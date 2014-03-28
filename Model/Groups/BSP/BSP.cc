#include <Model/Groups/BSP/BSP.h>
#include <Model/Groups/KDTree.h> //for the mailbox
#include <Model/Primitives/MeshTriangle.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/AffineTransformT.h>
#include <Core/Math/MinMax.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/Context.h>
#include <Core/Thread/Time.h>
#include <Interface/MantaInterface.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <iomanip>

// if defined, we'll add _realtive_ epsilons to plane distance (i.e., compare with t_p +/- 0.0001*t_p
#define SSE_REL_EPSILONS

using namespace Manta;

void BSP::preprocess(const PreprocessContext &context)
{
  if (mesh) {
    mesh->preprocess(context);
    mesh->computeBounds(context, bounds);

    // only rebuild if this is the "legitimate" preprocess and not a preprocess
    // manually called in order to find the bounds or something like that...
    // In this way we ensure that the initial build only occurs once.
    // Secondly, don't build if we already have it built, possibly because it
    // was loaded from file...
    if (context.isInitialized() && nodes.empty())
      rebuild(context.proc, context.numProcs);
  }

  if (context.proc == 0 && context.isInitialized())
    context.manta_interface->registerSerialPreRenderCallback(
      Callback::create(this, &BSP::update));
}

void BSP::setGroup(Group* new_group)
{
  mesh = dynamic_cast<Mesh*>(new_group);
}

void BSP::rebuild(int proc, int numProcs)
{
  if (proc != 0)
    return;

  double startTime = Time::currentSeconds();

  bounds.reset();

  //Something's wrong, let's bail. Since we reset the bounds right
  //above, this BSP should never get intersected.
  if (!mesh)
    return;

  PreprocessContext context;
  mesh->computeBounds(context, bounds);

  objects.clear();
  nodes.clear();
  nodes.reserve(mesh->size()*4);

  Point *vertexData = new Point[mesh->size()*3];
  for (size_t i=0; i < mesh->size(); ++i) {
    MeshTriangle *tri = mesh->get(i);
    vertexData[i*3+0] = tri->getVertex(0);
    vertexData[i*3+1] = tri->getVertex(1);
    vertexData[i*3+2] = tri->getVertex(2);
    vertices.push_back(&vertexData[i*3]);
  }

  int nextFreeNode = 1;

  //During traversal we do a very fast bounding box test to quickly
  //decide whether we need to actually intersect the BSP. So here we
  //place "fake" initial splitting planes to simulate the planes that
  //make up the bounding box test.
  vector<BuildSplitPlane> splitPlanes;

  cout <<"bounds are: "<<bounds[0] <<"  ,  " <<bounds[1]<<endl;
  cout <<"bounds SA:  "<<bounds.computeArea()<<endl;

  polytope.initialize(bounds);

  float lArea = polytope.getSurfaceArea();
  cout <<"root area is: "<<lArea<<endl;

  BSH bsh;
  bsh.build(mesh, bounds);
  cout <<"BSH built\n";
  build(0, nextFreeNode, splitPlanes, bsh, 1);

  delete[] vertexData;
  vertices.clear();

  float buildTime = Time::currentSeconds() - startTime;
  printf("BSP tree built in %f seconds\n", buildTime);
  printStats();

  if (proc == 0 && needToSaveFile)
    saveToFile(saveFileName);
}

void BSP::rebuildSubset(const vector<int> &subsetObjects, const BBox &bounds)
{
  isSubset = true;

  nodes.clear();
  nodes.reserve(subsetObjects.size()*4);

  objects = subsetObjects;
  this->bounds = bounds;

  Point *vertexData = new Point[mesh->size()*3];
  for (size_t i=0; i < mesh->size(); ++i) {
    MeshTriangle *tri = mesh->get(i);
    vertexData[i*3+0] = tri->getVertex(0);
    vertexData[i*3+1] = tri->getVertex(1);
    vertexData[i*3+2] = tri->getVertex(2);
    vertices.push_back(&vertexData[i*3]);
  }
  int nextFreeNode = 1;

  polytope.initialize(bounds);

  BSH bsh;
  bsh.buildSubset(mesh,subsetObjects, bounds);

  vector<BuildSplitPlane> splitPlanes;
  build(0, nextFreeNode, splitPlanes, bsh, 1);

  delete[] vertexData;
  vertices.clear();
}

#ifdef MANTA_SSE
// packet BSP traversal
//     Object *split_primitive = NULL;

typedef sse_t sse_arr[RayPacket::MaxSize/4];

#define false4() cmp4_lt(zero4(),zero4())
#endif

void BSP::intersect(const RenderContext& context, RayPacket& rays) const {
#ifndef MANTA_SSE
  intersectSingleRays(context, rays);
#else
  //Can only do packet traversal if we have constant origins. Furthermore,
  //it's only worth doing if there's more than one ray in the packet.
  if (!rays.getFlag(RayPacket::ConstantOrigin) || rays.end()-rays.begin()==1)
    intersectSingleRays(context, rays);
  else
    intersectPacket(context, rays);
#endif
}

void BSP::intersectPacket(const RenderContext& context, RayPacket& rays) const
// void BSP::intersect_same_origin(const RenderContext& context, RayPacket& rays) const
{
#ifdef MANTA_SSE

//   rays.normalizeDirections();
//   rays.computeSigns();
  rays.computeInverseDirections();

  const int ray_begin = rays.begin();
  const int ray_end = rays.end();
  const int sse_begin = ray_begin / 4;
  const int sse_end   = (ray_end+3) / 4;

#ifdef COLLECT_STATS
  nTotalRays += rays.end() - rays.begin();
#endif

#undef MAILBOX
#ifdef MAILBOX
  Mailbox mailbox;
  mailbox.clear();
#endif
  assert(rays.getFlag(RayPacket::ConstantOrigin));

  sse_t floatStackMem[RayPacket::MaxSize*80*3];
  float *freeMem = (float *)floatStackMem;

  sse_t *t_n = (sse_t*)freeMem; freeMem += RayPacket::MaxSize;
  sse_t *t_f = (sse_t*)freeMem; freeMem += RayPacket::MaxSize;
  sse_arr t_p;

  // set all sse-blocks to full ray segment
  for (int i=sse_begin;i<sse_end;i++) {
    t_n[i] = set4(T_EPSILON);
    t_f[i] = load44(&rays.getMinT(i*4));
  }
  // for incomplete border sse-blocks, invalidate ray segment
  for (int i=4*sse_begin;i<ray_begin;i++) {
    ((float*)t_n)[i] = MAXT;
    ((float*)t_f)[i] = -MAXT;
  }
  for (int i=ray_end;i<4*sse_end;i++) {
    ((float*)t_n)[i] = MAXT;
    ((float*)t_f)[i] = -MAXT;
  }

  sse_t anyValid = false4();
  for (int i=sse_begin;i<sse_end;i++) {
MANTA_UNROLL(3)
    for (int k=0;k<3;k++) {
      const sse_t rcp_ki = oneOver(load44(&rays.getDirection(4*i,k)));
      const sse_t dist_b0k = mul4(sub4(set4(bounds[0][k]),
                                       load44(&rays.getOrigin(4*i,k))),rcp_ki);
      const sse_t dist_b1k = mul4(sub4(set4(bounds[1][k]),
                                       load44(&rays.getOrigin(4*i,k))),rcp_ki);
      t_n[i] = max4(t_n[i],min4(dist_b0k,dist_b1k));
      t_f[i] = min4(t_f[i],max4(dist_b0k,dist_b1k));
    }
    anyValid = or4(anyValid,cmp4_le(t_n[i],t_f[i]));
  }

  if (getmask4(anyValid) == 0)
    // no active ray
    return;

  const Vector shared_origin = rays.getOrigin(rays.begin());

  struct StackEntry
  {
    float *freeMem;
    sse_t *t_n;
    sse_t *t_f;
    int nodeID;
  };

  StackEntry stackBase[80];
  StackEntry *stackPtr = stackBase;

  int nodeID = 0;
  while (1) {
    const BSPNode &node = nodes[nodeID];
    if (node.isLeaf()) {
      for (size_t i=0; i<node.numPrimitives(); ++i) {
#ifdef MAILBOX
        if (mailbox.testAndMark(objects[node.objectsIndex+i]))
          continue;
#endif
        mesh->get(objects[node.objectsIndex+i])->intersect(context, rays);
#ifdef COLLECT_STATS
        ++nTriIntersects;
#endif
        //         objects[node.objectsIndex+i]->intersect(context, rays);
      }

#if 1
      {
        // early ray termination
        sse_t allDied = _mm_true;
MANTA_UNROLL(4)
        for (int i=sse_begin;i<sse_end;i++)
          {
            sse_t thisOneDied =// and4(cmp4_le(t_n[i],t_f[i]), // valid
                                     cmp4_le(load44(&rays.getMinT(i*4)),t_f[i]);
            allDied = and4(allDied,thisOneDied);
          }
        if (getmask4(allDied) == 0xf)
          {
            //(thiago) this isn't catching all the early outs.
            //cout << "ERT" << endl;
            return;
          }
      }
#endif

    pop_node:
      if (stackPtr <= stackBase)
        {
          return;
        }
      --stackPtr;

      freeMem = stackPtr->freeMem;
      t_n = stackPtr->t_n;
      t_f = stackPtr->t_f;
      //       val = stackPtr->val;

MANTA_UNROLL(4)
      for (int i=sse_begin;i<sse_end;i++)
        {
          t_f[i] = min4(t_f[i],load44(&rays.getMinT(i*4)));
          //           val[i] = cmp4_le(t_n[i],t_f[i]);
          //           val[i] = and4(val[i],cmp4_le(t_n[i],t_f[i]));
        }
      nodeID = stackPtr->nodeID;
    } else {
      // inner node
//       const SplitPlane &plane = node.split;

      int originSideNodeID;
      int otherSideNodeID;
      sse_t originSideFlag = cast4_i2f(set4i(0));
      sse_t otherSideFlag  = cast4_i2f(set4i(0));

      if (node.isKDTree()) {
#ifdef COLLECT_STATS
      nTraversals++;
      nKDTraversals++;
#endif
        const int planeDim =  node.kdtreePlaneDim();
#define ASSUME_CONSTANT_ORIGIN
        // NOTE: ASSUME CONSTANT ORIGIN RIGHT NOW!!!
        const float origin_in_plane_eq = node.d - shared_origin[planeDim];
        const int origin_halfspace_sign = (origin_in_plane_eq < 0.f);

        // Dot(shared_origin,node.normal)+node.d > 0.f);
        originSideNodeID = node.children +   origin_halfspace_sign;
        otherSideNodeID  = node.children + 1-origin_halfspace_sign;

#ifdef ASSUME_CONSTANT_ORIGIN
        const sse_t org_in_plane = set4(origin_in_plane_eq);
#endif
        MANTA_UNROLL(8)
        for (int i=sse_begin;i<sse_end;i++) {
          const sse_t rcp_dir = load44(&rays.getInverseDirection(4*i, planeDim));

#ifndef ASSUME_CONSTANT_ORIGIN
          const sse_t normal_dot_origin = dot4(org_x[i],org_y[i],org_z[i],
                                               normal_0, normal_1, normal_2);
          const sse_t negative_org_in_plane =
            sub4(set4(-node.d),normal_dot_origin);
#endif
          t_p[i] = mul4(org_in_plane, rcp_dir);
        }
      }
      else {
#ifdef COLLECT_STATS
    ++nTraversals;
#endif

#define ASSUME_CONSTANT_ORIGIN
        // NOTE: ASSUME CONSTANT ORIGIN RIGHT NOW!!!
        const float origin_in_plane_eq
          = shared_origin[0]*node.normal[0]
          + shared_origin[1]*node.normal[1]
          + shared_origin[2]*node.normal[2]
          + node.d;

        const int origin_halfspace_sign = (origin_in_plane_eq > 0.f);

        // Dot(shared_origin,node.normal)+node.d > 0.f);
        originSideNodeID = node.children +   origin_halfspace_sign;
        otherSideNodeID  = node.children + 1-origin_halfspace_sign;

#ifdef ASSUME_CONSTANT_ORIGIN
        const sse_t negative_org_in_plane = set4(-origin_in_plane_eq);
#endif
        const Vector normal = node.normal;
        const sse_t normal_0 = set4(normal[0]);
        const sse_t normal_1 = set4(normal[1]);
        const sse_t normal_2 = set4(normal[2]);

        MANTA_UNROLL(8)
        for (int i=sse_begin;i<sse_end;i++) {
          //         const float t_p = -(Dot(node.normal, org) + node.d) / normal_dot_direction;
          const sse_t normal_dot_direction = dot4(load44(&rays.getDirection(4*i,0)),
                                                  load44(&rays.getDirection(4*i,1)),
                                                  load44(&rays.getDirection(4*i,2)),
                                                  normal_0, normal_1, normal_2);
#ifndef ASSUME_CONSTANT_ORIGIN
          const sse_t normal_dot_origin = dot4(org_x[i],org_y[i],org_z[i],
                                               normal_0, normal_1, normal_2);
          const sse_t negative_org_in_plane =
            sub4(set4(-node.d),normal_dot_origin);
#endif
          t_p[i] = mul4(negative_org_in_plane, oneOver4(normal_dot_direction));
        }
      }

      const sse_t rel_err = set4(1.0001f);

      MANTA_UNROLL(8)
      for (int i=sse_begin;i<sse_end;i++) {
#ifdef SSE_REL_EPSILONS
        const sse_t thisIsValid = cmp4_le(t_n[i],mul4(t_f[i],rel_err));
#else
        const sse_t thisIsValid = cmp4_le(t_n[i],t_f[i]);
#endif
        const sse_t facing_away = cmp4_le(t_p[i],zero4());
        const sse_t org_in_plane = cmp4_eq(t_p[i],zero4());
#ifdef SSE_REL_EPSILONS
        const sse_t thisOnOrigin
          = or4(facing_away, // faces away (in which case it's always on org side) ...
                cmp4_le(t_n[i],mul4(t_p[i],rel_err)) // or it points to the plane but doesn't
                );
#else
        const sse_t thisOnOrigin
          = or4(facing_away, // faces away (in which case it's always on org side) ...
                cmp4_le(t_n[i],t_p[i]) // or it points to the plane but doesn't
                );
#endif
        originSideFlag = or4(originSideFlag,
                             and4(thisIsValid,
                                  thisOnOrigin));

#ifdef SSE_REL_EPSILONS
        const sse_t thisOnOther  = or4(org_in_plane,
                                       andnot4(facing_away,
                                               // _not_ facing away, i.e. facing towrad, _and_
                                               cmp4_ge(mul4(rel_err,
                                                            t_f[i]),
                                                       t_p[i]) // ray seg does not end before plane
                                               ));
#else
        const sse_t thisOnOther  = or4(org_in_plane,
                                       andnot4(facing_away,
                                               // _not_ facing away, i.e. facing towrad, _and_
                                               cmp4_ge(t_f[i],t_p[i]) // ray seg does not end before plane
                                               ));
#endif
//           val[i] = cmp4_le(t_n[i],t_f[i]);
//           originSideFlag = or4(originSideFlag, and4(val[i],thisOnOrigin));
//           otherSideFlag  = or4(otherSideFlag,  and4(val[i],thisOnOther));
        otherSideFlag  = or4(otherSideFlag,
                             and4(thisIsValid,
                                  thisOnOther));
      }
#if 0
      // FORCE traversing both children -- essetially tests if tree
      // contains all tris, and if tritests work ...
      const int originSide = 0xf;
      const int otherSide = 0xf;
#else
      const int originSide = getmask4(originSideFlag);
      const int otherSide = getmask4(otherSideFlag);
#endif
      if (originSide) {
        if (otherSide) {
          // both sides

          sse_t *const frnt_t_f = (sse_t*)freeMem; freeMem += RayPacket::MaxSize;
//           sse_t * frnt_val = (sse_t*)freeMem; freeMem += RayPacket::MaxSize;
          sse_t *const back_t_n = (sse_t*)freeMem; freeMem += RayPacket::MaxSize;
MANTA_UNROLL(4)
          for (int i=sse_begin;i<sse_end;i++) {

            /*
              - ray facing away: ray interval (n,f) unchanged on
                origin side, invalidated (in whatever way) on other
                side
              - ray facing towards plane: (n,min(f,d)) on front side,
                and (max(n,d),f) on back side that is, assuming all
                rays agree on what origin side and other side are ...

              inverted:
              - origin interval is: (n,f) for rays facing away, and
                (n,min(f,d)) for those pointing towards
              - other interval is (max(n,d),f) for those pointing
                towards, and (inf,f) for those pointing away
             */
            const sse_t facing_away = cmp4_le(t_p[i],zero4());
            const sse_t org_in_plane = cmp4_eq(t_p[i],zero4());
            frnt_t_f[i] = mask4(facing_away,t_f[i],min4(t_p[i],t_f[i]));
            back_t_n[i] = mask4(andnot4(org_in_plane,facing_away),
                                set4(1e20f),
                                max4(t_p[i],t_n[i]));
          }

//           stackPtr->val = back_val;
          stackPtr->freeMem = freeMem;
          stackPtr->t_n = back_t_n;
          stackPtr->t_f = t_f;
          stackPtr->nodeID = otherSideNodeID;
          stackPtr++;

          nodeID = originSideNodeID;
          t_f = frnt_t_f;
//           val = frnt_val;

        } else {
          // _only_ origin side
          nodeID = originSideNodeID;
        }
      } else {
        if (otherSide) {
          // _only_ other side
          nodeID = otherSideNodeID;
        } else {
          goto pop_node;
          exit(1);
        }
      }
    }
  }
#else
  cerr<<"oops, shouldn't have gotten to intersectPacket()\n";
#endif
}

void BSP::intersectSingleRays(const RenderContext& context, RayPacket& rays) const
{
  //we normalize the rays so that we get consistent distances when we
  //do the epsilon test on the t value. kdtrees don't need that test,
  //so they also don't need to normalized rays.
  rays.normalizeDirections();
  TrvStack trvStack[80];

  rays.computeInverseDirections();
  for(int r=rays.begin(); r<rays.end(); r++) {

    RayPacket singleRayPacket(rays, r, r+1);
    Vector origin = rays.getOrigin( r );
    Vector direction = rays.getDirection( r );
    Vector inverse_direction = rays.getInverseDirection( r );
    traverse(context,singleRayPacket,origin,direction,inverse_direction,trvStack);
  }
}

void BSP::traverse(const RenderContext &context, RayPacket &ray,
                   const Vector &org, const Vector &dir, const Vector &rcp, TrvStack *const stackBase)
  const
{
#ifdef COLLECT_STATS
  nTotalRays += ray.end() - ray.begin();
#endif

  float t_n = T_EPSILON;
  float t_f = ray.getMinT( ray.begin() );

  const int signs[3] = {dir[0] < 0, dir[1] < 0, dir[2] < 0};

  for (int k=0;k<3;k++) {
    if (signs[k]) {
      const float k0 = (bounds[1][k] - org[k]) * rcp[k];
      t_n = Max(t_n,k0);
      const float k1 = (bounds[0][k] - org[k]) * rcp[k];
      t_f = Min(t_f,k1);
    } else {
      const float k0 = (bounds[0][k] - org[k]) * rcp[k];
      t_n = Max(t_n,k0);
      const float k1 = (bounds[1][k] - org[k]) * rcp[k];
      t_f = Min(t_f,k1);
    }
    if (t_n > t_f)
      return;
  };
  TrvStack *stackPtr = stackBase;
  int nodeID = 0;

#ifdef BSP_INTERSECTION
  int depth = 0;
  int nearPlane = -1;
  int farPlane = -1;

#endif

  while (1) {
    const BSPNode &node = nodes[nodeID];
    if (node.isLeaf()) {
#ifdef COLLECT_STATS
      nLeafs++;
#endif
      for (size_t i=0; i<node.numPrimitives(); ++i) {
#ifdef BSP_INTERSECTION
        //Is the closest splitting plane that the ray intersected the
        //triangle face plane? If so, and we've also intersected the
        //planes that go through the 3 triangle edges, then we must
        //have intersected the triangle!

        /*
        //This is actually slower and results in some errors in the cylinder.
        if (node.numPrimitives() == 1 && ((node.triangleBounds & 0x1) == 0x1)) {

#ifdef COLLECT_STATS
          nBSPTriIntersects++;
#endif
          const MeshTriangle* const tri = mesh->get(objects[node.objectsIndex]);
          const Vector v0 = tri->getVertex(0);
          const Vector v1 = tri->getVertex(1);
          const Vector v2 = tri->getVertex(2);

          const Vector normal = Cross(v1 - v0, v2 - v0);
          const float normal_dot_direction = Dot(normal, dir);
          const float inv_dot = 1.f/normal_dot_direction;
          const float d = Dot(normal, -v0);
          const float t_p = -(Dot(normal, org) + d) * inv_dot; /// normal_dot_direction;

          if (t_p+T_EPSILON >= t_n && t_p <= t_f+T_EPSILON) {
            ray.hit(ray.begin(), t_p, mesh->materials[mesh->face_material[0]],
                    tri, (TexCoordMapper*)tri);
            return;
          }
        }
        */

        if (node.triangleBounds == 0x3) {
          if (nearPlane == node.getTriPlaneDepth()) {
#ifdef COLLECT_STATS
          nBSPTriIntersects++;
#endif
            const MeshTriangle* const tri = mesh->get(objects[node.objectsIndex]);
            ray.hit(ray.begin(), t_n, mesh->materials[mesh->face_material[objects[node.objectsIndex]]],
                    tri, (TexCoordMapper*)tri);
          }
          else if (farPlane == node.getTriPlaneDepth()) {
#ifdef COLLECT_STATS
            nBSPTriIntersects++;
#endif
            const MeshTriangle* const tri = mesh->get(objects[node.objectsIndex]);
            ray.hit(ray.begin(), t_f, mesh->materials[mesh->face_material[objects[node.objectsIndex]]],
                    tri, (TexCoordMapper*)tri);
          }
          else if (1) {
            //In an ideal world, if both the near and far planes don't
            //correspond with the triangle, then we know the ray
            //missed. But in practice there might be multiple planes
            //with the same hit point (a plane gets used twice or
            //numerical precision issues occur), so we still need to
            //check the misses.
#ifdef COLLECT_STATS
          nTriIntersects++;
#endif
//             const float oldT = ray.getMinT( ray.begin() );
          mesh->get(objects[node.objectsIndex+i])->intersect(context, ray);
//             const float newT = ray.getMinT( ray.begin() );
//             if (newT != oldT) {
//               //we got a hit that we didn't expect!!!!
//             }
          }
        }

        else {
#ifdef COLLECT_STATS
          nTriIntersects++;
#endif
          mesh->get(objects[node.objectsIndex+i])->intersect(context, ray);
        }
#else
#ifdef COLLECT_STATS
          nTriIntersects++;
#endif

          mesh->get(objects[node.objectsIndex+i])->intersect(context, ray);
#endif //BSP_INTERSECTION
      }
      if (stackPtr <= stackBase)
        return;
      --stackPtr;
      const float newT = ray.getMinT( ray.begin() );
      if (newT <= t_f)
        return;
      nodeID = stackPtr->nodeID;

      t_n = t_f;
#ifdef BSP_INTERSECTION
      nearPlane = farPlane;

      if (newT < stackPtr->t) {
        t_f = newT;
        //t_f does not correspond to a plane (it hit a primitive not
        //on a split)
        farPlane = -1;
      }
      else {
        t_f = stackPtr->t;
        farPlane = stackPtr->farPlane;
      }
      depth = stackPtr->depth+1;
#else
      t_f = Min(newT,stackPtr->t);
#endif //BSP_INTERSECTION
    }
    else if (node.isKDTree()) {
#ifdef COLLECT_STATS
      nTraversals++;
      nKDTraversals++;
#endif
      const int planeDim = node.kdtreePlaneDim();
      const float t_p = (node.d - org[planeDim]) * rcp[planeDim];

      const int frontChild = node.children+signs[planeDim];
      const int backChild  = node.children+1-signs[planeDim];

      //TODO: This T_EPSILON stuff is making the far/near plane
      //triangle intersection not work well. Should see if there's a
      //good way to fix this.  The problem is that this causes us to
      //have the incorrect far/near plane and we require that one of
      //these two planes be the triangle plane in order for an
      //intersection to occur.

      //A solution would be to keep a list of nearPlanes and farPlanes
      //that are close (within epsilon) to t_n and t_f.

      if (t_p < t_n - 1e-5f) {
        nodeID = backChild;
      }
      else if (t_p > t_f + 1e-5f) {
        nodeID = frontChild;
      }
      else {
        stackPtr->nodeID = backChild;
        stackPtr->t = t_f;
#ifdef BSP_INTERSECTION
        stackPtr->depth = depth;
        stackPtr->farPlane = farPlane;
        farPlane = depth;
#endif
        ++stackPtr;
        nodeID = frontChild;
        t_f = t_p;
      }
#ifdef BSP_INTERSECTION
      depth++;
#endif
    }
    else {
#ifdef COLLECT_STATS
    nTraversals++;
#endif
      const float normal_dot_direction = Dot(node.normal, dir);
      const float inv_dot = 1.f/normal_dot_direction;
      const float t_p = -(Dot(node.normal, org) + node.d) * inv_dot; /// normal_dot_direction;

      const int side = ((unsigned int &)normal_dot_direction) >> 31;
      const int frontChild = node.children+side; //normal_dot_direction > 0.f ? node.children : node.children+1;
      const int backChild  = node.children+1-side;//normal_dot_direction > 0.f ? node.children+1 : node.children;

      if (t_p < t_n - 1e-5f) {
        nodeID = backChild;
      }
      else if (t_p > t_f + 1e-5f) {
        nodeID = frontChild;
      }
      else {
        stackPtr->nodeID = backChild;
        stackPtr->t = t_f;
#ifdef BSP_INTERSECTION
        stackPtr->depth = depth;
        stackPtr->farPlane = farPlane;
        farPlane = depth;
#endif
        ++stackPtr;
        nodeID = frontChild;
        t_f = t_p;
      }
#ifdef BSP_INTERSECTION
      ++depth;
#endif
    }
  }
};

void BSP::setTriangleBounds(vector<BuildSplitPlane> &splitPlanes, int &nodeID,
                            int &nextFreeNode)
{
#ifdef BSP_INTERSECTION
  assert(nodes[nodeID].numPrimitives() == 1);
  const size_t triID = objects[nodes[nodeID].objectsIndex];

  //The node needs to be contained in the triangle. In otherwords, it
  //can be equal to the triangle, or it can be inside the triangle.
  //To be more precise, if one of the faces of the node is completly
  //contained within the triangle, then we can intersect that triangle
  //purely through the traversal.

  //This also means that it is ok to have very large nodes (high
  //Surface Area) as long as one of the faces corresponds to a
  //triangle, since any further subdividing to get empty nodes would
  //not result in any fewer triangle or node tests (we aren't doing
  //triangle tests after all).

  //TODO: modify SAH criteria so that these kinds of nodes have
  //priority. Perhaps make C_isec zero.

  unsigned int triangleBounds = 0;

#if 1
  Polygon triangle;
  triangle.push_back(vertices[triID][0]);
  triangle.push_back(vertices[triID][1]);
  triangle.push_back(vertices[triID][2]);
  for (size_t f=0; f < polytope.faces.size(); ++f) {
    float intersectedArea = 0;
    if (overlaps(triangle, polytope.faces[f], intersectedArea, true)) {
      if (almostEqualRelative(intersectedArea, getArea(polytope.faces[f]))) {
        for (size_t i=0; i < splitPlanes.size(); ++i) {
          bool pointsOnPlane[3];
          for (int k=0; k < 3; ++k)
            pointsOnPlane[k] = fabs(signedDistance(splitPlanes[i], vertices[triID][k])) < 1e-6;
          if (pointsOnPlane[0] && pointsOnPlane[1] && pointsOnPlane[2]) {
            nodes[nodeID].setTriPlaneDepth(i);
            nodes[nodeID].triangleBounds = 0x3;
            triangleBounds = 0xf;
//             cerr << nodes[nodeID].triangleBounds<<endl;
//             return;
          }
        }
//         cerr << nodes[nodeID].triangleBounds<<endl;
//         return;
      }
//       else
//         cout <<intersectedArea << "  " << getArea(polytope.faces[f])<< "  "
//              <<intersectedArea / getArea(polytope.faces[f])<<endl<<endl;
    }
  }
  //#else
  //find which splits correspond with the triangle edges and plane.
  for (size_t i=0; i < splitPlanes.size(); ++i) {
    bool pointsOnPlane[3];
    for (int k=0; k < 3; ++k)
      pointsOnPlane[k] = fabs(signedDistance(splitPlanes[i], vertices[triID][k])) < 1e-6;

    if (pointsOnPlane[0] && pointsOnPlane[1] && pointsOnPlane[2]) {
      //if all three edges are on the split plane, then the triangle
      //is on the plane.

      //This means during traversal we already calculated the t value.
      //If all the other sides have been tested and this is the
      //closest intersection point, then we know for sure we have an
      //intersection.
      if (true||(triangleBounds & 0x8) == 0) {
        nodes[nodeID].setTriPlaneDepth(i);
        triangleBounds |= 0x8;
      }
    }
    else if (pointsOnPlane[0] && pointsOnPlane[1]) {
      triangleBounds |= 0x4;
    }
    else if (pointsOnPlane[0] && pointsOnPlane[2]) {
      triangleBounds |= 0x2;
    }
    else if (pointsOnPlane[1] && pointsOnPlane[2]) {
      triangleBounds |= 0x1;
    }
  }

#endif

  unsigned int numSplits = splitPlanes.size();

  if (triangleBounds == 0xe ||
      triangleBounds == 0xd ||
      triangleBounds == 0xb ||
      triangleBounds == 0x7) {
    numSplits++;
    //We are missing just one split. It's definitely cheaper to just
    //add this extra node traversal than doing a full triangle test

    BuildSplitPlane plane;
    const Point triNormal = getTriangleNormal(vertices[triID][0],
                                              vertices[triID][1],
                                              vertices[triID][2]).normal();
    Point p1, p2, p3;
    if (triangleBounds == 0xe) {
      //missing 0x1 == vertices 1 and 2
      p1 = vertices[triID][1];
      p2 = vertices[triID][2];
      p3 = vertices[triID][0];
    }
    else if (triangleBounds == 0xd) {
      //missing 0x2 == vertices 0 and 2
      p1 = vertices[triID][0];
      p2 = vertices[triID][2];
      p3 = vertices[triID][1];
    }
    else if (triangleBounds == 0xb) {
      //missing 0x4 == vertices 0 and 1
      p1 = vertices[triID][0];
      p2 = vertices[triID][1];
      p3 = vertices[triID][2];
    }

    if (triangleBounds == 0x7) {
      //missing 0x8 == triangle autopartition
      plane.normal = triNormal;
      p3 = vertices[triID][0];
      plane.d = Dot(plane.normal, -p3);
      nodes[nodeID].setTriPlaneDepth(splitPlanes.size());
    }
    else {
      plane.normal = Cross(triNormal, p2-p1).normal();
      plane.d = Dot(plane.normal, -p1);
    }

    if (triangleBounds == 0xf)
      nodes[nodeID].triangleBounds = 0x3;
    else if (triangleBounds == 0x8)
      nodes[nodeID].triangleBounds = 0x2;
    else if (triangleBounds == 0x7)
      nodes[nodeID].triangleBounds = 0x1;
    else nodes[nodeID].triangleBounds = 0x0;

    int leftChild = nextFreeNode++;
    int rightChild = nextFreeNode++;
    nodes.resize(nextFreeNode);
    BSPNode leafNode = nodes[nodeID];
    nodes[nodeID].makeInternal(leftChild, plane.normal, plane.d);

    double p3_dist = signedDistance(plane, p3);
    if (p3_dist > 0) {
      swap(leftChild, rightChild);
    }

    nodes[leftChild] = leafNode;
    nodes[leftChild].triangleBounds = 0x3;

    nodes[rightChild].makeLeaf(objects.size(), 0);

    nodeID = leftChild;
  }

  return;

  if ((nodes[nodeID].triangleBounds == 0x3 ||
       nodes[nodeID].triangleBounds == 0x1) &&
      nodes[nodeID].getTriPlaneDepth() < (int) splitPlanes.size()-1) {
//     cout <<"hi\n";
    //add triangle autopartition
    BuildSplitPlane plane;
    const Point triNormal = getTriangleNormal(vertices[triID][0],
                                              vertices[triID][1],
                                              vertices[triID][2]).normal();
    plane.normal = triNormal;
    plane.d = Dot(plane.normal, -vertices[triID][0]);
    nodes[nodeID].setTriPlaneDepth(splitPlanes.size());

    int leftChild = nextFreeNode++;
    int rightChild = nextFreeNode++;
    nodes.resize(nextFreeNode);
    BSPNode leafNode = nodes[nodeID];
    nodes[nodeID].makeInternal(leftChild, plane.normal, plane.d);

    nodes[leftChild] = leafNode;
    nodes[leftChild].triangleBounds = 0x3;
    nodes[leftChild].setTriPlaneDepth(numSplits);

    nodes[rightChild].makeLeaf(objects.size(), 0);

    nodeID = leftChild;
  }
#endif
//   cerr << nodes[nodeID].triangleBounds<<endl;
}

void BSP::build(int nodeID, int &nextFreeNode,
                vector<BuildSplitPlane> &splitPlanes, BSH &primitives, int depth)
{
  nodes.resize(nextFreeNode);

  bool forceCreateLeaf = false;
  if (primitives.size()-primitives.numSplits() <=0){
  createLeaf:
    if (primitives.numSplits() > 0) {
      nodes[nodeID].makeLeaf(objects.size(),
                             primitives.size()-primitives.numSplits());
      BSH::Iterator iter = primitives.begin();
      objects.push_back(iter.get().originalTriID);
      for (iter.next(); iter != primitives.end(); iter.next()) {
        if (iter.get().originalTriID != objects.back())
          objects.push_back(iter.get().originalTriID);
      }
    }
    else {
      nodes[nodeID].makeLeaf(objects.size(), primitives.size());
      for (BSH::Iterator iter = primitives.begin();
           iter != primitives.end(); iter.next()) {
        objects.push_back(iter.get().originalTriID);
      }
    }

    //Doing this seems to improve performance for single ray
    //traversal. Packet traversal on the other hand does better with
    //shallow trees.
#ifdef BSP_INTERSECTION
    if (primitives.size()-primitives.numSplits() == 1) {
      setTriangleBounds(splitPlanes, nodeID, nextFreeNode);

      if (!forceCreateLeaf && nodes[nodeID].triangleBounds != 0x3) {
        objects.pop_back();
        goto createInteriorNode;
      }
    }
#endif

  }
  else {
#ifdef BSP_INTERSECTION
  createInteriorNode:
#endif

    Location coplanar_primitivesSide;
    BuildSplitPlane plane = getBuildSplitPlane(primitives, splitPlanes, coplanar_primitivesSide);
    if (plane.normal == Point(0,0,0)) {
      forceCreateLeaf = true;
      goto createLeaf;
    }

    splitPlanes.push_back(plane);

    BSH posPrimitives(primitives);
    primitives.split(splitPlanes.back(), posPrimitives, coplanar_primitivesSide);

    int leftChild = nextFreeNode++;
    int rightChild = nextFreeNode++;
    nodes[nodeID].makeInternal(leftChild, plane.normal, plane.d);

    Polytope side1, side2;
    polytope.split(splitPlanes.back(), side1, side2);

    polytope = side1;
    polytope.updateVertices();

    build(leftChild, nextFreeNode, splitPlanes, primitives, depth+1);

    polytope = side2;
    polytope.updateVertices();

    splitPlanes.resize(depth);
    splitPlanes.back().flip();

    build(rightChild, nextFreeNode, splitPlanes, posPrimitives, depth+1);
  }
}

BuildSplitPlane BSP::getBuildSplitPlane(BSH &primitives,
                              vector<BuildSplitPlane> &splitPlanes,
                              Location &coplanar_primitivesSide)
{
  //I am assuming we just have triangles!!!
//    return getRandomSplit(primitives, splitPlanes, coplanar_primitivesSide);
  return getSAHSplit(primitives, splitPlanes, coplanar_primitivesSide);
}

void BSP::printPrimitivesAsObj(const BSH &primitives) const
{
  for (BSH::Iterator iter = primitives.begin();
       iter != primitives.end(); iter.next())
    for (size_t k = 0; k < 3; ++k)
      cout << "v "<<iter.get().tri[k]<<endl;
      //cout << "v " << vertices[iter.get().originalTriID][k] <<"\n";
  for (size_t i = 0; i < primitives.size(); ++i)
    cout << "f " << i*3 +1<< " " << i*3+2 << " " <<i*3+3 <<endl;
}
void BSP::printOriginalPrimitivesAsObj(const BSH &primitives) const
{
  for (BSH::Iterator iter = primitives.begin();
       iter != primitives.end(); iter.next())
    for (size_t k = 0; k < 3; ++k)
      //cout << "v "<<iter.get().tri[k]<<endl;
      cout << "v " << vertices[iter.get().originalTriID][k] <<"\n";
  for (size_t i = 0; i < primitives.size(); ++i)
    cout << "f " << i*3 +1<< " " << i*3+2 << " " <<i*3+3 <<endl;
}


const float C_isec = 75;
const float C_KDTree_trav = 10;
const float C_BSP_trav = 20;

inline bool isKDTreeSplit(const BuildSplitPlane &splitPlane) {
  return (splitPlane.normal.minComponent() < -0.9999999 ||
          splitPlane.normal.maxComponent() >  0.9999999);
}

BuildSplitPlane BSP::getSAHSplit(BSH &primitives,
                                 vector<BuildSplitPlane> &splitPlanes,
                                 Location &coplanar_primitivesSide)
{
  float parentArea;
  parentArea = polytope.getSurfaceArea();
//     cout <<"parent Area: " << parentArea <<endl;

  SplitData bestSplits[2]; //first is for bsp splits, second for kd.
  bestSplits[0].cost = bestSplits[1].cost =
    (primitives.size()-primitives.numSplits())*C_isec;

  //if the node is really small, just stop. (is this really needed?)
  if (parentArea < 1e-10) {
    BuildSplitPlane plane;
    plane.normal = Point(0,0,0);
    plane.d = 0;
    return plane;
  }

  splitPlanes.resize(splitPlanes.size()+1);

  int previousTriID=-1;
  const int lastPass = 16;

  set<BuildSplitPlane> splitPlaneSet;

  for (BSH::Iterator iter = primitives.begin();
       iter!=primitives.end(); iter.next()) {

    SplitData splitdata;
    splitdata.splitPrimitive = iter.get().originalTriID;

    for (int pass=0; pass < lastPass; ++pass) {
      Polygon splitPoints;

      splitdata.splitType = pass;

      //set the split to use the triangle face.
      splitdata.split = iter.get().plane;

      if (pass == 0 && iter.get().originalTriID != previousTriID) {
        //try triangle plane In theory, the originalTri normal's and
        //the current tri normal are the same. In practice though, the
        //current tri normal might have more error since the triangle
        //is smaller, so we use the original normal.

        splitPoints.push_back(vertices[iter.get().originalTriID][0]);
        splitPoints.push_back(vertices[iter.get().originalTriID][1]);
        splitPoints.push_back(vertices[iter.get().originalTriID][2]);
      }
      else if (pass > 0 && pass < 4  && iter.get().originalTriID != previousTriID
               ) {
        //use plane on triangle edge that is orthogonal to triangle
        //normal Edges that are on the bounding polytope (and
        //obviously those outside it), are useless.
        Point p1, p2;
        switch ((pass-1)%3) {
        case 0:
#if 1
          p1 = vertices[iter.get().originalTriID][0];//iter.get().tri[0];
          p2 = vertices[iter.get().originalTriID][1];//iter.get().tri[1];
          break;
        case 1:
          p1 = vertices[iter.get().originalTriID][1];//iter.get().tri[1];
          p2 = vertices[iter.get().originalTriID][2];//iter.get().tri[2];
          break;
        case 2:
          p1 = vertices[iter.get().originalTriID][2];//iter.get().tri[2];
          p2 = vertices[iter.get().originalTriID][0];//iter.get().tri[0];
          break;
#else
          p1 = iter.get().tri[0];
          p2 = iter.get().tri[1];
          break;
        case 1:
          p1 = iter.get().tri[1];
          p2 = iter.get().tri[2];
          break;
        case 2:
          p1 = iter.get().tri[2];
          p2 = iter.get().tri[0];
          break;

#endif
        }

        splitdata.split.normal = Cross(splitdata.split.normal, p2 - p1).normal();

#ifndef _MSC_VER // drand48 not available, and since this code isn't even
                 // executed, we can safely comment it out.
        if (pass > 3) {
          AffineTransformT<double> transform;
          transform.initWithRotation(p2-p1, (.5-drand48())*2.5);
          splitdata.split.normal = transform.multiply_vector(splitdata.split.normal).normal();
        }
#endif
        splitdata.split.d = Dot(splitdata.split.normal, -p1);

        splitPoints.push_back(p1);
        splitPoints.push_back(p2);
      }
      else if (pass > 6 && pass < 16) {
        const int k = (pass-7)%3;
        const int v = (pass-7)/3;

#if 1
        //We are only interested in spliting planes that don't split
        //the triangle (i.e. use those only on the triangle boundary).
        const double split = vertices[iter.get().originalTriID][v][k];
        if ((split - vertices[iter.get().originalTriID][(v+1)%3][k])*
            (split - vertices[iter.get().originalTriID][(v+2)%3][k]) < 0)
#else
        const double split = iter.get().tri[v][k];
        if ((split - iter.get().tri[(v+1)%3][k])*
            (split - iter.get().tri[(v+2)%3][k]) < 0)
#endif
          continue;

        splitdata.split.d = -split;
        splitdata.split.normal = Point(0,0,0); //set it all to 0
        splitdata.split.normal[k] = 1;         //now set one of them to 1
      }
      else
        continue;

      //Check to see if we've already tried this split. If so, don't
      //bother testing it again.
      if (splitPlaneSet.insert(splitdata.split).second == false)
        continue;

      //set to false to not use getBuildSplitPlane
      if (false && !(pass == 0 && iter.get().fixedPlane) ) //are we using the fixed plane?
        splitdata.split = getBestSplitPlane(primitives, splitdata.split,
                                               splitPoints);

      splitPlanes.back() = splitdata.split;

      bool success = getSurfaceAreas(splitdata);
      if (!success) {
        //Either something went bad or the split was useless (cut off
        //only a vertex or an edge of the polytope).
        continue;
      }
      calculateSAH(parentArea, splitdata, primitives, polytope);

//       printf("%d - cost: %f  areas: %f  %f   counts: %d %d   probabilities are %f %f  \t%f\n",
//              pass, splitdata.cost, splitdata.lArea, splitdata.rArea, splitdata.nLeft, splitdata.nRight,  splitdata.lArea/parentArea, splitdata.rArea/parentArea,
//              (splitdata.lArea+splitdata.rArea)/parentArea);


      if (polytope.debugPrint) {
        cout <<"\n\ndebug print:\n";
        printPrimitivesAsObj(primitives);
        cout <<endl;
        Polytope side1, side2;
        Polytope poly;
        poly.initialize(bounds);
        poly.printAsObj(primitives.size()*3);
        for (size_t i=6; i < splitPlanes.size(); ++i) {
          cout <<"\nsplit " << i<<endl;
          side1.clear();
          side2.clear();
          poly.split(splitPlanes[i], side1, side2);
          side1.printAsObj(primitives.size()*3);
          side2.printAsObj(primitives.size()*3);
          cout <<endl;
          cout << "plane " << splitPlanes[i].normal<< "  " <<splitPlanes[i].d<<endl;
          poly = side1;
        }
        cout <<"\n\ninput mesh:\n";
        printPrimitivesAsObj(primitives);

        exit(1);
      }

      if (splitdata.lArea*.98 > parentArea || splitdata.rArea*.98 > parentArea) {
          //This should never happen. If it does, the surface area
          //calculation got screwed up. If we use this split plane, bad
          //things might later happen.
#ifdef BUILD_DEBUG
          printf( "%f+%f = %f\n", splitdata.lArea/parentArea, splitdata.rArea/parentArea, (splitdata.lArea+splitdata.rArea)/parentArea);
          printf("%d Split has cost %f and #left/right are: %d %d\n",
                 iter.get().originalTriID, splitdata.cost, splitdata.nLeft, splitdata.nRight);

          cout.precision(20);
          Polytope side1, side2;
          polytope.split(splitdata.split, side1, side2);
          polytope.printAsObj(primitives.size()*3);
          cout << "plane " << splitdata.split.normal<< "  " <<splitdata.split.d<<endl;
          side1.printAsObj(primitives.size()*3);
          side2.printAsObj(primitives.size()*3);
          cout << "parentArea: " <<parentArea<<endl;
          cout << "side1 area: " <<lArea<<endl;
          cout << "side2 area: " <<rArea<<endl;

          Polytope poly;
          poly.initialize(bounds);
          poly.printAsObj(primitives.size()*3);
          for (size_t i=6; i < splitPlanes.size(); ++i) {
            cout <<"\nsplit " << i<<endl;
            side1.clear();
            side2.clear();
            poly.split(splitPlanes[i], side1, side2);
            cout<<"area: "<<side1.getSurfaceArea() << " and is flat()="<<side1.isFlat()<<endl;
            side1.printAsObj(primitives.size()*3);
            cout<<"area: "<<side2.getSurfaceArea() << " and is flat()="<<side2.isFlat()<<endl;
            side2.printAsObj(primitives.size()*3);
            cout <<endl;
            cout << "plane " << splitPlanes[i].normal<< "  " <<splitPlanes[i].d<<endl;

            poly = side1;
          }

        cout <<"\n\ninput mesh:\n";
        printPrimitivesAsObj(primitives);
        if (parentArea > 1e-7) //smaller than this and there's
                               //probably not much we can do. (maybe compute convex hull??)
          exit(1);
#endif
          continue;
        }

      if (splitdata.lArea + splitdata.rArea <= parentArea*0.99) {
        //it is legitimate (but still useless) for one area to be 1
        //and another 0 (split along polytope edge for instance).
        if (splitdata.lArea < parentArea*.99 && splitdata.rArea < parentArea*.99) {
            //This should never happen. If it does, the surface area
            //calculation got screwed up. If we use this split plane, bad
            //things might later happen.
          cout.precision(20);
          printf( "%f+%f = %f   (area too small)\n", splitdata.lArea/parentArea, splitdata.rArea/parentArea, (splitdata.lArea+splitdata.rArea)/parentArea);
          printf("%d Split has cost %f and #left/right are: %d %d\n",
                 iter.get().originalTriID, splitdata.cost, splitdata.nLeft, splitdata.nRight);

#ifdef BUILD_DEBUG
          Polytope side1, side2;
          Polytope poly;
          poly.initialize(bounds);

          for (size_t i=6; i < splitPlanes.size(); ++i) {
            cout <<"\nsplit " << i<<endl;
            side1.clear();
            side2.clear();
            poly.split(splitPlanes[i], side1, side2);
            side1.printAsObj(primitives.size()*3);
            side2.printAsObj(primitives.size()*3);
            cout <<endl;
            cout << "plane " << splitPlanes[i].normal<< "  " <<splitPlanes[i].d<<endl;
            poly = side1;
          }

        cout <<"\n\ndebug print:\n";
        printPrimitivesAsObj(primitives);
        cout <<endl;

        if (parentArea > 1e-7)
          exit(1);
#endif
        continue;
          }
      }


      if (isKDTreeSplit(splitdata.split)) {
        if ( splitdata.cost < bestSplits[1].cost ||
             (splitdata.cost == bestSplits[1].cost &&
              bestSplits[1].splitPrimitive < splitdata.splitPrimitive))
          bestSplits[1] = splitdata;
      }
      else {
        if (splitdata.cost < bestSplits[0].cost ||
            (splitdata.cost == bestSplits[0].cost &&
             bestSplits[0].splitPrimitive < splitdata.splitPrimitive))
          bestSplits[0] = splitdata;
      }
    }
    previousTriID = splitdata.splitPrimitive;
  }


//   if (bestSplits[0].splitPrimitive >= 0)
//     printf("\nSplit has cost %f and #left/right are: %d %d and child probabilities are %f %f. C_bsp=%f\n",
//            bestSplits[0].cost, bestSplits[0].nLeft, bestSplits[0].nRight, bestSplits[0].lArea/parentArea, bestSplits[0].rArea/parentArea, C_BSP_trav);
//   if (bestSplits[1].splitPrimitive >= 0)
//     printf("Split has cost %f and #left/right are: %d %d and child probabilities are %f %f\n",
//            bestSplits[1].cost, bestSplits[1].nLeft, bestSplits[1].nRight, bestSplits[1].lArea/parentArea, bestSplits[1].rArea/parentArea);


  //Now that we've tested all the split candidates, we need to figure
  //out whether the best kdtree split is better than the best bsp
  //split.

  //First lets check whether we can use the varying BSP cost.
  if (bestSplits[1].splitPrimitive >= 0) {
    //We found a valid kd-tree split, so lets use the varying BSP cost.

    const float C_trav = .1*C_isec*(primitives.size() - 2) + C_KDTree_trav;
    const float lProb = bestSplits[0].lArea / parentArea;
    const float rProb = bestSplits[0].rArea / parentArea;

    bestSplits[0].cost = (bestSplits[0].nLeft*lProb +
                          bestSplits[0].nRight*rProb)*C_isec + C_trav;
  }
  else {
    bestSplits[0].cost += C_BSP_trav;
    if (bestSplits[0].cost >= (primitives.size()-primitives.numSplits())*C_isec)
      bestSplits[0].splitPrimitive = -1;
  }

  SplitData bestSplit;
  if (bestSplits[0].cost <= bestSplits[1].cost)
    bestSplit = bestSplits[0];
  else
    bestSplit = bestSplits[1];

  splitPlanes.resize(splitPlanes.size()-1);

  if (primitives.size()/(primitives.size()-primitives.numSplits()) >= 100) {
    cout <<primitives.numSplits()<< " split triangles " <<primitives.size()<<endl;
    //I don't know why this is happening. But if it does, just get out.
    if (primitives.size()/(primitives.size()-primitives.numSplits()) >= 600) {
      bestSplit.split.normal = Point(0,0,0);
      return bestSplit.split;
    }
  }

  if (bestSplit.splitPrimitive < 0) {
//     cout <<"found nothing!"<<endl;

    if (primitives.size() - primitives.numSplits() >= 16) {
      cout <<primitives.size() - primitives.numSplits()
           <<"too many primitives in leaf!\n";
//       cout.precision(10);
//       printPrimitivesAsObj(primitives);

//       Polytope side1, side2;
//       polytope.split(splitdata.split, side1, side2);
//       polytope.printAsObj(primitives.size()*3);
//       side1.printAsObj(primitives.size()*3);
//       side2.printAsObj(primitives.size()*3);
      //exit(1);
    }

    bestSplit.split.normal = Point(0,0,0);
    return bestSplit.split;
  }

  coplanar_primitivesSide = bestSplit.coplanar_side;

  //axis aligned splits need to have positive normals.
  if (isKDTreeSplit(bestSplit.split) &&              //is it a kd-tree split?
      bestSplit.split.normal.maxComponent() < 0.1) { //is it negative?
    bestSplit.split.normal*=-1;
    bestSplit.split.d*=-1;
    if (coplanar_primitivesSide == negativeSide)
      coplanar_primitivesSide = positiveSide;
    else if (coplanar_primitivesSide == positiveSide)
      coplanar_primitivesSide = negativeSide;
  }

//   printf("\n%f,%f,%f Split has cost %f and #left/right are: %d %d and child probabilities are %f %f\n",
//          bestSplit.split.normal[0],bestSplit.split.normal[1],bestSplit.split.normal[2], bestSplit.cost, bestSplit.nLeft, bestSplit.nRight, bestSplit.lArea/parentArea, bestSplit.rArea/parentArea);

  return bestSplit.split;
  }

void BSP::calculateSAH(float parentArea, SplitData &splitdata,
                        const BSH &primitives, const Polytope &polytope)
{
  int nLeft = 0;
  int nRight = 0;
  int nEither = 0;
  primitives.countLocations(splitdata.split, nLeft, nRight, nEither);

  //TODO: optimize this to remove all the useless computations.

  const float lProb = splitdata.lArea / parentArea;
  const float rProb = splitdata.rArea / parentArea;

  const float C_trav = isKDTreeSplit(splitdata.split) ? C_KDTree_trav : 0;
  //if it's a BSP split, we'll add in the traversal cost later.

  float leftCost, rightCost;
    leftCost = ((nLeft+nEither)*lProb + nRight*rProb)*C_isec + C_trav;
    rightCost = (nLeft*lProb + (nRight+nEither)*rProb)*C_isec + C_trav;

  if (leftCost < rightCost) {
    nLeft+=nEither;
    splitdata.cost = leftCost;
    splitdata.coplanar_side = negativeSide;
  }
  else {
    nRight+=nEither;
    splitdata.cost = rightCost;
    splitdata.coplanar_side = positiveSide;
  }

  splitdata.nLeft = nLeft;
  splitdata.nRight = nRight;
}

bool BSP::getSurfaceAreas(SplitData &splitdata)
{
  Polytope side1, side2;
  polytope.split(splitdata.split, side1, side2);
  splitdata.lArea = side1.getSurfaceArea();
  splitdata.rArea = side2.getSurfaceArea();

  if (splitdata.lArea <= 0 || splitdata.rArea <= 0) {
    return false;
  }
  return true;
}


 BuildSplitPlane BSP::getBestSplitPlane(const BSH &primitives,
                                        BuildSplitPlane plane,
                                        const vector<Point> &initialVertices)
 {
   //some splits, like axis aligned splits, don't do this.
   if(initialVertices.empty())
     return plane;

   //static to reduce (de)allocations. Not thread-safe.
   //static Polygon points;
   Polygon points; //Can't do static Polygon due to getBestSplitPlane being called recursively!
   points = initialVertices;

   if (initialVertices.size() < 3) {
//      primitives.pointsNearPlaneDist(points, plane);

     //Also want the plane to exactly go through the vertices of the
     //bounding polytope
     for (size_t i=0; i < polytope.vertices.size(); ++i) {

       Point normal2 = Cross(points[1]-points[0],
                             polytope.vertices[i]-points[0]);
       if (normal2.length2() < 1e-12)
         continue;
       normal2.normalize();
       double costheta = 1-fabs(Dot(normal2, plane.normal));

       if( costheta < 0.001 ) {
         points.push_back(polytope.vertices[i]);
         break;
       }
     }

     if (points.size() >= 3)
       radialSort(points);
     if (points.size() < 3) {
       Point closePoint = initialVertices[0];
       double minCostheta = .2;

       primitives.pointNearPlaneAngle(points, plane, closePoint, minCostheta);
       if (closePoint == points[0])
         return plane;
       points.push_back(closePoint);
       if (points.size() >= 3)
         radialSort(points);
       if (points.size() < 3)
         return plane;
     }

     //Now that we have 3 points, lets try
     plane.normal = getOrderedFaceNormal(points);
     const Point center = getCenter(points);
     plane.d = Dot(plane.normal, -center);
   }

   primitives.pointsNearPlaneDist(points, plane);
   const double MAX_DIST = 2.5e-6; //TODO: ideally, make this a
                                   //function of distance from vertex
                                   //to triangle that defined plane.

   //Also want the plane to exactly go through the vertices of the
   //bounding polytope
   for (size_t i=0; i < polytope.vertices.size(); ++i) {
       double p_dist = signedDistance(plane, polytope.vertices[i]);
//        double errorScalar = distanceAlongPlane(plane, initialVertices,
//                                                     polytope.vertices[i]);
       if (fabs(p_dist) <= 1*MAX_DIST) {
//          if (p_dist != 0)
//            cout << "got some at: " <<p_dist<<endl;
         points.push_back(polytope.vertices[i]);
       }
   }

   if (points.size() >= 3)
     radialSort(points);

   if (points.size() < 3) {
     return plane;
   }

   BuildSplitPlane optimizedPlane;
   optimizedPlane.normal = getOrderedFaceNormal(points);

   //we just want a small correction
   double costheta = fabs(Dot(optimizedPlane.normal, plane.normal));
   if( (costheta < 0.99 && initialVertices.size()>=3) ||
       (costheta < 0.5 && initialVertices.size() < 3)) {
#ifdef BUILD_DEBUG
     if (costheta > .9) {
       cout << "corrected too much!\n"
            <<plane.normal <<endl
            <<optimizedPlane.normal<<endl;
       cout<<initialVertices.size()<< " and angle: " <<costheta<<endl;
     // exit(0);
     }
#endif
     return plane;
   }

   //Is it better to place the center at the initialVertices or using
   //all the vertices that were used to calculate the optimized
   //normal?
   Point center = getCenter(points);//getCenter(initialVertices);
   optimizedPlane.d = Dot(optimizedPlane.normal, -center);

   //TODO: check if some points are still far away and if so, remove them
   //and try again.

   return optimizedPlane;
 }


bool BSP::buildFromFile(const string &file)
{
  double startTime = Time::currentSeconds();

  bounds.reset();

  //Something's wrong, let's bail. Since we reset the bounds right
  //above, this KDTree should never get intersected.
  if (!mesh)
    return false;

  PreprocessContext context;
  mesh->computeBounds(context, bounds);

  bool isAscii = false;
  bool isKDTree = false;
  if (!strncmp(file.c_str()+file.length()-8, ".kdtreer", 8) ||
      !strncmp(file.c_str()+file.length()-7, ".kdtree", 7)) {
    isAscii = isKDTree = true;
  }
  if (!strncmp(file.c_str()+file.length()-4, ".bsp", 4)) {
    isAscii = true;
  }

  if (isAscii) {
    objects.clear();
    nodes.clear();
    nodes.reserve(mesh->size()*4);

    ifstream in(file.c_str());
    if (!in) return false;
    unsigned int itemListSize;
    in >> itemListSize;

    for (unsigned int i=0; i < itemListSize; ++i) {
      int item;
      in >> item;
      if (!in) return false;
      objects.push_back(item);
    }

    while (in.good()) {
      BSPNode node;
      int i;

      in >> i;
      int isLeaf = i;

      if (in.eof())
        break;

      if (isLeaf) {
        int nObjects;
        unsigned int children;
        in >> nObjects;
        in >> children;
        node.makeLeaf(children, nObjects);
        if (!isKDTree) {
          int triangleBounds = 0;
          int triPlaneDepth = 0;
          in >> triangleBounds;
          in >> triPlaneDepth;
#ifdef BSP_INTERSECTION
          node.triangleBounds = triangleBounds;
          node.setTriPlaneDepth(triPlaneDepth);
#endif
        }
      }
      else {
        int type;
        in >> type;

        float d;
        in >> d;

        Vector normal(0,0,0);
        if (type == 0) { //kdtree format
          d*=-1;

          unsigned int planeDim;
          in >> planeDim;
          normal[planeDim] = 1;
        }
        else {
          in >> normal;
        }
        unsigned int children;
        in >> children;

        node.makeInternal(children, normal, d);
      }
      nodes.push_back(node);
    }
    in.close();
  }
  else {
    ifstream in(file.c_str(), ios::binary | ios::in);

    unsigned int objects_size;
    in.read((char*)&objects_size, sizeof(objects_size));
    objects.resize(objects_size);

    in.read((char*) &objects[0], sizeof(objects[0])*objects.size());

    unsigned int nodes_size;
    in.read((char*)&nodes_size, sizeof(nodes_size));
    nodes.resize(nodes_size);

    in.read((char*) &nodes[0], sizeof(BSPNode)*nodes.size());
    in.close();
  }

  cout << "done building" << endl << flush;
  float buildTime = Time::currentSeconds() - startTime;
  printf("BSP tree built in %f seconds\n", buildTime);
  printStats();

  return true;
}


bool BSP::saveToFile(const string &file)
{
  if (nodes.empty()) {
    needToSaveFile = true;
    saveFileName = file;
    return true; // We support it, so we return true now.  Hopefully it'll end
                 // up working when this actually occurs...
  }
  needToSaveFile = false;

#if 0
  //ascii write (human readable, but big files and inaccurate!)
  ofstream out(file.c_str());
  if (!out) return false;

  out << objects.size() << endl;
  for (unsigned int i=0; i < objects.size(); ++i) {
    out << objects[i] <<" ";
  }

  out<<endl;

  for (unsigned int i=0; i < nodes.size(); ++i) {
    out << nodes[i].isLeaf() << " ";
    if (nodes[i].isLeaf()) {
      out << nodes[i].numPrimitives() << " " <<  nodes[i].objectsIndex << " "
          <<nodes[i].triangleBounds << " " <<nodes[i].triPlaneDepth<<endl;
    }
    else {
      if (nodes[i].isKDTree())
        out  << 0 << " " << nodes[i].d << " " << nodes[i].kdtreePlaneDim() << " " <<  nodes[i].children<<endl;
      else
        out  << 1 << " " << nodes[i].d << " " << nodes[i].normal << " " <<  nodes[i].children<<endl;
    }
  }
#else
  ofstream out(file.c_str(), ios::out | ios::binary);
  if (!out) return false;

  const unsigned int objects_size = objects.size();
  const unsigned int nodes_size = nodes.size();
  out.write((char*) &objects_size, sizeof(objects_size));
  out.write((char*) &objects[0], sizeof(objects[0])*objects.size());
  out.write((char*) &nodes_size, sizeof(nodes_size));
  out.write((char*) &nodes[0], sizeof(BSPNode)*nodes.size());
#endif
  out.close();
  return true;
}


void BSP::printStats()
{
  TreeStats stats = {0};
  collectTreeStats(0, 0, stats);

  printf("BSP tree made from %d primitives has: \n", (int)mesh->size());
  printf(" - %d nodes\n", (int)nodes.size());
  printf(" - %d primitive references\n", (int)objects.size());
  printf(" - %d max depth\n", stats.maxDepth);
  printf(" - %d max objects in a leaf\n", stats.maxObjectsInLeaf);

  for (int i=0; i<stats.MAX; ++i)
    if (stats.childrenLeafHist[i] > 0)
      printf(" - %d leaves with %d children\n", stats.childrenLeafHist[i], i);

  float totalKD = 0;
  float totalBSP = 0;
  for (int i=0; i<stats.MAX; ++i)
    if (stats.splitType[i][0] + stats.splitType[i][1] > 0) {
      totalBSP += stats.splitType[i][0];
      totalKD += stats.splitType[i][1];
      float totalSplits = stats.splitType[i][0] + stats.splitType[i][1];
      printf(" - %d, %d\t (%3.0f%% %3.0f%%) bsp/kdtree splitType\n",
             stats.splitType[i][0], stats.splitType[i][1],
             100*(stats.splitType[i][0] / totalSplits),
             100*(stats.splitType[i][1] / totalSplits));
    }

  printf(" - %3.1f%% nodes kdtree\n", 100*totalKD / (totalKD+totalBSP));
}

void BSP::collectTreeStats(int nodeID, int depth, TreeStats &stats)
{
  if (depth >= stats.MAX) {
    cout << "Tree is too deep!\n";
    exit(1);
  }

  const BSPNode &node = nodes[nodeID];
  if (node.isLeaf()) {
    if ((int)node.numPrimitives() >= stats.MAX) {
      cerr << "There are too many primitives in the leaf! " << node.numPrimitives()<<endl;
      return;
    }

    stats.maxDepth = Max(stats.maxDepth, depth);
    stats.maxObjectsInLeaf = Max((unsigned int)stats.maxObjectsInLeaf,
                                         node.numPrimitives());
    stats.childrenLeafHist[node.numPrimitives()]++;
  }
  else {
    if (node.isKDTree())
      stats.splitType[depth][1]++;
    else
      stats.splitType[depth][0]++;
    collectTreeStats(node.children, depth+1, stats);
    collectTreeStats(node.children+1, depth+1, stats);
  }
}
