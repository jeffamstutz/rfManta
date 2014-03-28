#include <Model/Groups/KDTree.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Geometry/Vector.h>
#include <Core/Math/MinMax.h>
#include <Interface/Context.h>
#include <Core/Thread/Time.h>
#include <Interface/MantaInterface.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <algorithm>

#define DBG(a)

using namespace Manta;

// define RTSAH if you want to use RTSAH traversal order
#define RTSAH //note: this currently only works with single ray traversal.
              //Packet traversal will see no improvement if this is enabled.

static const float ISEC_COST = 2;
static const float TRAV_COST = 1.5;

#define MAX_TREE_HEIGHT 512

#ifdef _MSC_VER
#  ifdef MANTA_SSE
#    define KDTREE_MANTA_SSE MANTA_SSE
#    undef MANTA_SSE
#  endif
#endif

#ifdef MANTA_SSE
#define SSE
#endif

#define false4() cmp4_lt(zero4(),zero4())

namespace Manta {
  bool ClipTriangle(BBox &clipped,
                    const Vector &a,const Vector &b,const Vector&c,
                    const BBox &bounds);
};

void KDTree::preprocess(const PreprocessContext &context)
{
  if (currGroup) {
    currGroup->preprocess(context);
    currGroup->computeBounds(context, bounds);

    // only rebuild if this is the "legitimate" preprocess and not a preprocess
    // manually called in order to find the bounds or something like that...
    // In this way we ensure that the initial build only occurs once.
    // Secondly, don't build if we already have it built, possibly because it
    // was loaded from file...
    if (context.isInitialized() && nodes.empty())
      rebuild(context.proc, context.numProcs);
  }

#ifdef COLLECT_STATS
  if (context.proc == 0 && context.isInitialized())
    context.manta_interface->registerParallelPreRenderCallback(
                                Callback::create(this, &KDTree::updateStats));
#endif
}

void KDTree::setGroup(Group* new_group)
{
  currGroup = new_group;
}


void KDTree::rebuild(int proc, int numProcs)
{
  if (proc != 0)
    return;

  double startTime = Time::currentSeconds();

  bounds.reset();

  //Something's wrong, let's bail. Since we reset the bounds right
  //above, this KDTree should never get intersected.
  if (!currGroup)
    return;

  PreprocessContext context;
  currGroup->computeBounds(context, bounds);

  nodes.clear();

  Node emptyNode;
  nodes.push_back(emptyNode);

  mesh = dynamic_cast<Mesh*>(currGroup);

  vector<int> primitives;  primitives.reserve(currGroup->size());
  for (unsigned int i=0; i < currGroup->size(); ++i) {
    primitives.push_back(i);
  }

  cout <<"bounds are: "<<bounds[0] <<"  ,  " <<bounds[1]<<endl;
  //   cout <<"bounds SA:  "<<bounds.computeArea()<<endl;

  double totalCost = 0;

#ifdef COLLECT_STATS
  stats.resize(numProcs);
#endif

  build(0, primitives, bounds, totalCost);

  cout << "done building" << endl << flush;
  double buildTime = Time::currentSeconds() - startTime;
  printf("KDTree tree built in %f seconds\n", buildTime);
  printStats();
  cout <<"SAH cost of tree is: " <<totalCost<<endl;

  if (proc == 0 && needToSaveFile)
    saveToFile(saveFileName);
}

struct Event
{
  enum Type { tri_end, tri_planar, tri_begin };
  Real pos;
  Type type;
  int tri;

  Event(Type t, Real p, int i) : pos(p), type(t), tri(i) {};
};

bool operator<(const Event &a, const Event &b)
{
  return a.pos < b.pos || (a.pos == b.pos && a.type < b.type);
}


BBox clipTri(MeshTriangle *tri, const BBox &bounds)
{
  BBox bb;

  Vector a = tri->getVertex(0);
  Vector b = tri->getVertex(1);
  Vector c = tri->getVertex(2);
  bool clipped = ClipTriangle(bb,a,b,c,bounds);
  if (!clipped) {
    bb.reset();
  }
  return bb;
};

void KDTree::build(unsigned int nodeID,
                   vector<int> &primitiveID,
                   const BBox &bounds,
                   double &totalCost, int depth)
{
  DBG(cout << "build " << nodeID << "   #=" << primitiveID.size() << " bb=" << bounds[0] << ":" << bounds[1] << endl);
  int bestDim = -1;
  Real bestSplit = -1;
  Real bestCost = primitiveID.size() * ISEC_COST;
  bool bestCommonToTheLeft = false;
  Real bestlProb=-1;
  Real bestrProb=-1;

  vector<Event> event[3];
  // reserving space gives a large speedup.
  event[0].reserve(primitiveID.size()*2);
  event[1].reserve(primitiveID.size()*2);
  event[2].reserve(primitiveID.size()*2);

  // Is the node still legit sized? If it's super tiny or super deep most
  // likely something is wrong. Just create a leaf.  Also, if it's more than
  // MAX_TREE_HEIGHT deep than this will be too large for the traversal stack.
  if (bounds.computeArea() > T_EPSILON*T_EPSILON && depth < MAX_TREE_HEIGHT-1) {

    //generate split candidates for SAH
    for (unsigned int i=0;i<primitiveID.size();i++) {
      BBox box;
      if (mesh)
        box = clipTri(mesh->get(primitiveID[i]),bounds);
      else {
        PreprocessContext context;
        currGroup->get(primitiveID[i])->computeBounds(context, box);
        box.intersection(bounds);
      }

      // Did clipTri fail (box.isDefault) or is the box degenerate (no area)?
      if (box.isDefault() || box.computeArea()==0) {
        // One way this could happen is if the triangle is almost outside the
        // bounds, with only one or two vertices lying *on* (not in) the bounds.
        primitiveID.erase(primitiveID.begin()+i);
        i--;
        bestCost = primitiveID.size() * ISEC_COST;
        continue;
      }
      else {
        for (int k=0;k<3;k++) {
          if (box.getMin()[k] == box.getMax()[k])
            event[k].push_back(Event(Event::tri_planar,box.getMin()[k],i));
          else {
            event[k].push_back(Event(Event::tri_begin,box.getMin()[k],i));
            event[k].push_back(Event(Event::tri_end,  box.getMax()[k],i));
          }
        }
      }
    }
  }

  //   cout << __PRETTY_FUNCTION__ << endl;
  const Real boundsArea = bounds.computeArea();
  for (int k=0;k<3;k++) {
    BBox lBounds = bounds;
    BBox rBounds = bounds;

    DBG(cout << "sorting " << k << " " << event[k].size() << endl);
    std::sort(event[k].begin(),event[k].end());

    int Nl = 0;
    int Nr = primitiveID.size();
    int Np = 0;

    unsigned int seqStart = 0;
    while (seqStart < event[k].size()) {
      unsigned int idx = seqStart;

      int numEnding = 0;
      while (idx < event[k].size() &&
             event[k][idx].pos == event[k][seqStart].pos &&
             event[k][idx].type == Event::tri_end)
        { ++idx; ++numEnding; }

      int numPlanar = 0;
      while (idx < event[k].size() &&
             event[k][idx].pos == event[k][seqStart].pos &&
             event[k][idx].type == Event::tri_planar)
        { ++idx; ++numPlanar; }

      int numStarting = 0;
      while (idx < event[k].size() &&
             event[k][idx].pos == event[k][seqStart].pos &&
             event[k][idx].type == Event::tri_begin)
        { ++idx; ++numStarting; }

      DBG(cout << "event " << event[k][seqStart].pos << "@" << k << endl;
          cout << "  " << numStarting << " " << numPlanar << " " << numEnding << endl);
      lBounds[1][k] = event[k][seqStart].pos;
      rBounds[0][k] = event[k][seqStart].pos;

      Real lProb = lBounds.computeArea() / boundsArea;
      Real rProb = rBounds.computeArea() / boundsArea;

      Nr -= numEnding;
      Nr -= numPlanar;

      Np =  numPlanar;

      DBG(cout << "prob " << lProb << " " << rProb << endl;
          cout << "   N " << Nl << " " << Np << " " << Nr << endl);

      {
        // try putting common on the left
        Real cost = TRAV_COST + ISEC_COST * (lProb * (Nl+Np) + rProb * Nr);

        DBG(cout << cost << endl);
        if (cost < bestCost) {
          bestCost = cost;
          bestCommonToTheLeft = true;
          bestDim = k;
          bestSplit = event[k][seqStart].pos;
          bestlProb = lProb;
          bestrProb = rProb;
          DBG(cout << "new favorite " << bestDim << " " << bestSplit << endl);
        }
      }
      {
        // try putting common on the right
        Real cost = TRAV_COST + ISEC_COST * (lProb * Nl + rProb * (Nr+Np));
        DBG(cout << cost << endl);
        if (cost < bestCost) {
          bestCost = cost;
          bestCommonToTheLeft = false;
          bestDim = k;
          bestSplit = event[k][seqStart].pos;
          bestlProb = lProb;
          bestrProb = rProb;
          DBG(cout << "new favorite " << bestDim << " " << bestSplit << endl);
        }
      }

      Nl += numPlanar;
      Nl += numStarting;

      seqStart = idx;
    }
  }

  DBG(cout << "node " << nodeID << " bestSplit " << bestSplit << " dim " << bestDim << endl);

  if (bestDim == -1) {
    // no split found ...
    makeLeaf(nodeID,primitiveID);
    DBG(cout << "leaf : " << nodeID << " " << primitiveID.size() << endl);

    //compute actual cost
    totalCost += primitiveID.size()*ISEC_COST;
  }
  else {
    vector<int> lPrimitive;
    vector<int> rPrimitive;

    bool *is_l = new bool[primitiveID.size()];
    bool *is_r = new bool[primitiveID.size()];

    for (unsigned int i=0;i<primitiveID.size();i++)
      is_l[i] = is_r[i] = true;

    for (unsigned int i=0;i<event[bestDim].size();i++)
    {
      switch (event[bestDim][i].type) {
      case Event::tri_planar:
        if ((event[bestDim][i].pos == bestSplit && bestCommonToTheLeft) ||
            event[bestDim][i].pos <  bestSplit)
          is_r[event[bestDim][i].tri] = false;
        else
          is_l[event[bestDim][i].tri] = false;
        break;
      case Event::tri_begin:
        if (event[bestDim][i].pos >= bestSplit)
          is_l[event[bestDim][i].tri] = false;
        break;
      case Event::tri_end:
        if (event[bestDim][i].pos <= bestSplit)
          is_r[event[bestDim][i].tri] = false;
        break;
      };
    }
    for (unsigned int i=0;i<primitiveID.size();i++) {
      if (is_l[i]) lPrimitive.push_back(primitiveID[i]);
      if (is_r[i]) rPrimitive.push_back(primitiveID[i]);
    }
    delete[] is_l;
    delete[] is_r;

    primitiveID.clear();
    makeInner(nodeID,bestDim,bestSplit);
    BBox lBounds = bounds;
    BBox rBounds = bounds;
    lBounds[1][bestDim] = bestSplit;
    rBounds[0][bestDim] = bestSplit;

    double lCost = 0;
    double rCost = 0;
    build(nodes[nodeID].childIdx+0,lPrimitive,lBounds, lCost, depth+1);
    build(nodes[nodeID].childIdx+1,rPrimitive,rBounds, rCost, depth+1);
    totalCost += bestlProb * lCost + bestrProb * rCost;
  }
};


void KDTree::intersect(const RenderContext& context, RayPacket& rays) const
{

  rays.computeInverseDirections();
  const bool anyHit = rays.getFlag(RayPacket::AnyHit);

#if 0  // Set to 1 to trace rays individually instead of using packet traversal.

  TrvStack trvStack[MAX_TREE_HEIGHT];

  for(int r=rays.begin(); r<rays.end(); r++) {
    if (rays.rayIsMasked(r))
      continue;
    RayPacket singleRayPacket(rays, r, r+1);
    Vector origin = rays.getOrigin( r );
    Vector direction = rays.getDirection( r );
    Vector inverse_direction = rays.getInverseDirection( r );

    if (anyHit)
      traverse<true>(context,singleRayPacket,origin,direction,inverse_direction,trvStack);
    else
      traverse<false>(context,singleRayPacket,origin,direction,inverse_direction,trvStack);
  }
  return;
#endif

  rays.computeSigns();

  const int ray_begin = rays.begin();
  const int ray_end   = rays.end();

  if (!rays.getFlag(RayPacket::ConstantSigns)) {
    TrvStack trvStack[MAX_TREE_HEIGHT];
    for (int first=ray_begin; first < ray_end; ++first) {
      int last = first+1;
      for (; last < ray_end; ++last) {
        if (rays.getSign(first, 0) != rays.getSign(last, 0) ||
            rays.getSign(first, 1) != rays.getSign(last, 1) ||
            rays.getSign(first, 2) != rays.getSign(last, 2)) {
          break;
        }
      }

      const int sse_begin = (first + 3) & (~3);
      const int sse_end   = (last) & (~3);
      if (sse_begin >= sse_end) {
        // If the packet can't use SSE, don't use packet traversal,
        // TODO: for packets of size 3 and aligned the right way, see if we
        // could just traverse an sse packet with one invalid ray.
        for (; first < last; ++first) {
          if (rays.rayIsMasked(first))
            continue;

          RayPacket singleRayPacket(rays, first, first+1);
          Vector origin = rays.getOrigin( first );
          Vector direction = rays.getDirection( first );
          Vector inverse_direction = rays.getInverseDirection(first);
          if (anyHit)
            traverse<true>(context, singleRayPacket,origin, direction,
                           inverse_direction, trvStack);
          else
            traverse<false>(context, singleRayPacket, origin, direction,
                            inverse_direction, trvStack);
        }
        first = last-1;
      }
      else {
        RayPacket sameSignSubpacket(rays, first, last);
        sameSignSubpacket.setFlag(RayPacket::ConstantSigns);
        sameSignSubpacket.resetFlag(RayPacket::HaveCornerRays);
        intersect(context, sameSignSubpacket);
        first = last-1;
      }
    }
    return;
  }
  else if (ray_begin+1 == ray_end) {
    if (rays.rayIsMasked(ray_begin))
      return;
    TrvStack trvStack[MAX_TREE_HEIGHT];
    const Vector origin = rays.getOrigin( rays.begin() );
    const Vector direction = rays.getDirection( rays.begin() );
    const Vector inverse_direction = rays.getInverseDirection(rays.begin());
    if (anyHit)
      traverse<true>(context, rays, origin, direction, inverse_direction, trvStack);
    else
      traverse<false>(context,rays, origin, direction, inverse_direction, trvStack);
    return;
  }


  const bool COMMON_ORIGIN = rays.getFlag(RayPacket::ConstantOrigin);

#ifdef MAILBOX
  Mailbox mailbox;
  mailbox.clear();
#endif

#ifdef COLLECT_STATS
  stats[context.proc].nTotalRays += rays.end() - rays.begin();
  stats[context.proc].nTotalRaysInPacket += rays.end() - rays.begin();
  stats[context.proc].nTotalPackets++;
#endif

  MANTA_ALIGN(16) Real t_in[RayPacketData::MaxSize+1]; //last element is min of all t_in
  MANTA_ALIGN(16) Real t_out[RayPacketData::MaxSize+1];//last element is max of all t_out
  MANTA_ALIGN(16) int valid[RayPacketData::MaxSize];

#ifdef SSE
  const int sse_begin = (ray_begin + 3) & (~3);
  const int sse_end   = (ray_end) & (~3);

  sse_t anyValid = false4();

  const int sign_x = rays.getSign(ray_begin,0);
  const int sign_y = rays.getSign(ray_begin,1);
  const int sign_z = rays.getSign(ray_begin,2);
  const sse_t near_x = set4(bounds[sign_x][0]);
  const sse_t near_y = set4(bounds[sign_y][1]);
  const sse_t near_z = set4(bounds[sign_z][2]);
  const sse_t far_x = set4(bounds[1-sign_x][0]);
  const sse_t far_y = set4(bounds[1-sign_y][1]);
  const sse_t far_z = set4(bounds[1-sign_z][2]);

  int numRays = 0;
  if (ray_begin < sse_begin || sse_end < ray_end) {
    Vector origin;

    if (COMMON_ORIGIN)
      origin = rays.getOrigin(ray_begin);

    bool leading = true;
    if (sse_begin >=sse_end)
      leading = false;
    for (int ray = ray_begin; /*no test*/ ; ++ray) {
      if (leading && ray >= sse_begin) {
        leading = false;
        ray = sse_end;
      }
      if (!leading && ray >= ray_end)
        break;

      //do bounding box test

      if (!COMMON_ORIGIN)
        origin = rays.getOrigin( ray );
      const Vector inverse_direction = rays.getInverseDirection( ray );

      t_in[ray] = T_EPSILON;
      t_out[ray] = rays.getMinT(ray);

      for (int k=0;k<3;k++) {
        //TODO: Check to see if using rays.getOrigin(0,k) and
        //getInverseDirection(ray,k) are faster.
        Real t0 = (bounds.getMin()[k] - origin[k]) * inverse_direction[k];
        Real t1 = (bounds.getMax()[k] - origin[k]) * inverse_direction[k];

        if (t0 > t1) {
          if (t1 > t_in[ray])  t_in[ray]  = t1;
          if (t0 < t_out[ray]) t_out[ray] = t0;
        } else {
          if (t0 > t_in[ray])  t_in[ray]  = t0;
          if (t1 < t_out[ray]) t_out[ray] = t1;
        }
        if (t_in[ray] > t_out[ray])
          break;
      }
      valid[ray] = (t_in[ray] <= t_out[ray]);
      if (valid[ray])
        numRays++;
    }
  }

  sse_t org_x = load44(&rays.getOrigin(sse_begin,0));
  sse_t org_y = load44(&rays.getOrigin(sse_begin,1));
  sse_t org_z = load44(&rays.getOrigin(sse_begin,2));

MANTA_UNROLL(4)
  for (int ray = sse_begin; ray < sse_end; ray+=4) {
    if (!COMMON_ORIGIN) {
      org_x = load44(&rays.getOrigin(ray,0));
      org_y = load44(&rays.getOrigin(ray,1));
      org_z = load44(&rays.getOrigin(ray,2));
    }

    sse_t t0 = set4(T_EPSILON);
    sse_t t1 = load44(&rays.getMinT(ray));
    const sse_t t0x = mul4(sub4(near_x,org_x),load44(&rays.getInverseDirection(ray,0)));
    t0 = max4(t0,t0x);
    const sse_t t1x = mul4(sub4(far_x, org_x),load44(&rays.getInverseDirection(ray,0)));
    t1 = min4(t1,t1x);
    const sse_t t0y = mul4(sub4(near_y,org_y),load44(&rays.getInverseDirection(ray,1)));
    t0 = max4(t0,t0y);
    const sse_t t1y = mul4(sub4(far_y, org_y),load44(&rays.getInverseDirection(ray,1)));
    t1 = min4(t1,t1y);
    const sse_t t0z = mul4(sub4(near_z,org_z),load44(&rays.getInverseDirection(ray,2)));
    t0 = max4(t0,t0z);
    const sse_t t1z = mul4(sub4(far_z, org_z),load44(&rays.getInverseDirection(ray,2)));
    t1 = min4(t1,t1z);

    store44(&t_in[ray], t0);
    store44(&t_out[ray], t1);
    store44(reinterpret_cast<float*>(&valid[ray]), cmp4_le(t0,t1));
    anyValid = or4(anyValid,load44(reinterpret_cast<float*>(&valid[ray])));
  }

  //   cout << __PRETTY_FUNCTION__ << endl;
  if (getmask4(anyValid) || numRays > 0) {
    if (COMMON_ORIGIN)
      intersectNode<true>(0, context, rays, t_in, t_out, valid
#ifdef MAILBOX
                          , mailbox
#endif
                          );
    else
      intersectNode<false>(0, context, rays, t_in, t_out, valid
#ifdef MAILBOX
                          , mailbox
#endif
                          );

  }
#else
  int numRays = 0;
  //   cout << "new rays -> " << (ray_end - rays.begin()) << endl;
  for(int ray=ray_begin; ray<ray_end; ray++) {

    //do bounding box test
    Vector origin = rays.getOrigin( ray );
    Vector inverse_direction = rays.getInverseDirection( ray );

    t_in[ray] = T_EPSILON;
    t_out[ray] = rays.getMinT(ray);

    for (int k=0;k<3;k++) {
      Real t0 = (bounds.getMin()[k] - origin[k]) * inverse_direction[k];
      Real t1 = (bounds.getMax()[k] - origin[k]) * inverse_direction[k];

      //    cout << " k=" << k << " : " << t0 << " " << t1 << endl;
      // hopefully everything all right with nan's etc....
      if (t0 > t1) {
        if (t1 > t_in[ray])  t_in[ray]  = t1;
        if (t0 < t_out[ray]) t_out[ray] = t0;
      } else {
        if (t0 > t_in[ray])  t_in[ray]  = t0;
        if (t1 < t_out[ray]) t_out[ray] = t1;
      }
      if (t_in[ray] > t_out[ray])
        break;
    }
    valid[ray] = (t_in[ray] <= t_out[ray]);
    if (valid[ray])
      numRays++;
  }
  //   cout << "num rays " << numRays << endl;
  if (numRays > 0) {

    if (COMMON_ORIGIN)
      intersectNode<true>(0, context, rays, t_in, t_out, valid
#ifdef MAILBOX
                          , mailbox
#endif
                          );
    else
      intersectNode<false>(0, context, rays,
                           t_in,t_out,valid
#ifdef MAILBOX
                           , mailbox
#endif
                           );
  }
#endif
}

#ifdef MAILBOX
template<bool COMMON_ORIGIN>
void KDTree::intersectNode(unsigned int nodeID, const RenderContext& context,
                           RayPacket& rays,
                           const Real *const  t_in,
                           const Real *const  t_out,
                           const int  *const  valid
                           , Mailbox &mailbox
                           ) const
#else
template<bool COMMON_ORIGIN>
void KDTree::intersectNode(unsigned int nodeID, const RenderContext& context,
                           RayPacket& rays,
                           const Real *const  t_in,
                           const Real *const  t_out,
                           const int  *const  valid
                           ) const
#endif
{
#ifdef COLLECT_STATS
  ++stats[context.proc].nTraversals;
#endif

  const Node &node = nodes[nodeID];
  if (node.isLeaf) {

#ifdef COLLECT_STATS
    ++stats[context.proc].nLeavesVisited;
    if (node.numPrimitives==0)
      ++stats[context.proc].nEmptyLeavesVisited;
#endif

    //     cout << "LEAF " << node.numPrimitives << endl;
    int primOffset = node.childIdx;
    for (int i=0; i<node.numPrimitives; ++i) {
      //       printf("testing object from node %d\n", nodeID);
      const int triID = itemList[primOffset+i];
#ifdef MAILBOX
      if (mailbox.testAndMark(triID))
        continue;
#endif

      currGroup->get(triID)->intersect(context, rays);
#ifdef COLLECT_STATS
      ++stats[context.proc].nIntersects;
#endif
    }
  } else {

    const int ray_begin = rays.begin();
    const int ray_end   = rays.end();

#ifdef SSE
    const int sse_begin = (ray_begin + 3) & (~3);
    const int sse_end   = (ray_end) & (~3);
#endif

    const int sign = rays.getSign(ray_begin,node.planeDim);

    int front=0;
    int back=0;
    MANTA_ALIGN(16) Real t_plane[RayPacketData::MaxSize];
#ifdef SSE
    sse_t frontMask = false4();
    sse_t backMask = frontMask;

    const sse_t planePos4 = set4(node.planePos);
    if (ray_begin < sse_begin || sse_end < ray_end) {
      bool leading = true;
      if (sse_begin >=sse_end)
        leading = false;

      Real origin = rays.getOrigin(ray_begin, node.planeDim);
      for (int ray = ray_begin; /*no test*/ ; ++ray) {
        if (leading && ray >= sse_begin) {
          leading = false;
          ray = sse_end;
        }
        if (!leading && ray >= ray_end)
          break;

        if (valid[ray]) {
          if (!COMMON_ORIGIN)
            origin = rays.getOrigin(ray, node.planeDim);
          t_plane[ray]
            = (node.planePos - origin) * rays.getInverseDirection(ray,node.planeDim);

          if (t_out[ray] < t_plane[ray])
            front++;
          else if (t_in[ray] > t_plane[ray])
            back++;
          else {
            front++;
            back++;
          }
        }
      }
    }

    sse_t org_k = load44(&rays.getOrigin(sse_begin,node.planeDim));
    sse_t plane_org = sub4(planePos4, org_k);

MANTA_UNROLL(4)
    for (int ray = sse_begin; ray < sse_end; ray+=4) {
      if (!COMMON_ORIGIN) {
        org_k = load44(&rays.getOrigin(ray,node.planeDim));
        plane_org = sub4(planePos4, org_k);
      }
      const sse_t rcp = load44(&rays.getInverseDirection(ray,node.planeDim));
      const sse_t tp = mul4(plane_org,rcp);
      store44(&t_plane[ray], tp);

      const sse_t frontOnly = cmp4_lt(load44(&t_out[ray]),tp);
      backMask = or4(backMask, andnot4(frontOnly,
                                       load44(reinterpret_cast<const float*>(&valid[ray]))));
      const sse_t backOnly = cmp4_gt(load44(&t_in[ray]),tp);
      frontMask = or4(frontMask, andnot4(backOnly,
                                         load44(reinterpret_cast<const float*>(&valid[ray]))));
    }

    front += getmask4(frontMask);
    back += getmask4(backMask);
#else
    int both = 0;
    Real origin = rays.getOrigin(ray_begin, node.planeDim);
    for(int ray=ray_begin; ray<ray_end; ray++) {
      if (valid[ray]) {
        if (!COMMON_ORIGIN)
          origin = rays.getOrigin(ray, node.planeDim);
        t_plane[ray]
          = (node.planePos - origin) * rays.getInverseDirection(ray,node.planeDim);
        if (t_out[ray] < t_plane[ray])
          front++;
        else if (t_in[ray] > t_plane[ray])
          back++;
        else {
          both++;
        }
      }
    }
    front+=both;
    back+=both;
#endif

    const unsigned int backChildID = node.childIdx+1-sign;
    const unsigned int frontChildID = node.childIdx+sign;

    if (front == 0) {
#ifdef MAILBOX
      intersectNode<COMMON_ORIGIN>(backChildID,context,rays,t_in,t_out,valid, mailbox);
#else
      intersectNode<COMMON_ORIGIN>(backChildID,context,rays,t_in,t_out,valid);
#endif
    } else if (back == 0) {
#ifdef MAILBOX
      intersectNode<COMMON_ORIGIN>(frontChildID,context,rays,t_in,t_out,valid, mailbox);
#else
      intersectNode<COMMON_ORIGIN>(frontChildID,context,rays,t_in,t_out,valid);
#endif
    } else {
      // We need to intersect both nodes

      int packetValid = 0; // valid if > 0

      MANTA_ALIGN(16) Real new_t_in [RayPacketData::MaxSize+1];
      MANTA_ALIGN(16) Real new_t_out[RayPacketData::MaxSize+1];
      MANTA_ALIGN(16) int  new_valid[RayPacketData::MaxSize];

#ifdef SSE
      if (ray_begin < sse_begin || sse_end < ray_end) {
        bool leading = true;
        if (sse_begin >=sse_end)
          leading = false;

        for (int ray = ray_begin; /*no test*/ ; ++ray) {
          if (leading && ray >= sse_begin) {
            leading = false;
            ray = sse_end;
          }
          if (!leading && ray >= ray_end)
            break;

          if (!valid[ray]) {
            new_valid[ray] = false;
          } else {
            if (t_in[ray] > t_plane[ray]) {
              new_valid[ray] = false;
            }
            else {
              new_valid[ray] = true;
              new_t_out[ray] = Min(t_out[ray], t_plane[ray]);
            }
          }
        }
      }
MANTA_UNROLL(4)
      for (int ray = sse_begin; ray < sse_end; ray+=4) {
        //no need to do t_in here
        const sse_t nto = min4(load44(&t_out[ray]),load44(&t_plane[ray]));
        store44(&new_t_out[ray], nto);
        store44(reinterpret_cast<float*>(&new_valid[ray]),
                and4(load44(reinterpret_cast<const float*>(&valid[ray])),
                     cmp4_le(load44(&t_in[ray]),nto)));
      }
#else

      for(int ray=ray_begin; ray<ray_end; ray++) {
        if (!valid[ray]) {
          // ray does not traverse either node
          new_valid[ray] = false;
        } else {
          if (t_in[ray] > t_plane[ray]) {
            // ray does not traverse front node, but it does the back.
            new_valid[ray] = false;
          }
          else {
            // ray traverses front node and possibly also the back.
            new_valid[ray] = true;
            new_t_out[ray] = Min(t_out[ray], t_plane[ray]);
          }
        }
      }
#endif //SSE

      intersectNode<COMMON_ORIGIN>(frontChildID,context,rays,
                    t_in,new_t_out,new_valid
#ifdef MAILBOX
                    , mailbox
#endif
                    );

#ifdef SSE
      if (ray_begin < sse_begin || sse_end < ray_end) {
        bool leading = true;
        if (sse_begin >=sse_end)
          leading = false;

        for (int ray = ray_begin; /*no test*/ ; ++ray) {
          if (leading && ray >= sse_begin) {
            leading = false;
            ray = sse_end;
          }
          if (!leading && ray >= ray_end)
            break;

          if (!valid[ray]) {
            new_valid[ray] = false;
          } else {
            if (t_in[ray] > t_plane[ray]) {
              new_t_in[ray] = t_in[ray];
              // might may have changed in prev isec step:
              new_t_out[ray] = t_out[ray];
              new_valid[ray] = true;
            } else if (t_out[ray] < t_plane[ray]) {
              new_valid[ray] = false;
            } else {
              new_t_in[ray] = t_plane[ray]; //t_in[ray];
              new_t_out[ray] = Min(t_out[ray],rays.getMinT(ray));
              new_valid[ray] = (new_t_in[ray]<=new_t_out[ray]);
            }
            packetValid += new_valid[ray];
          }
        }
      }

      backMask = false4();
MANTA_UNROLL(4)
      for (int ray = sse_begin; ray < sse_end; ray+=4) {
        const sse_t nti = max4(load44(&t_in[ray]),load44(&t_plane[ray]));
        store44(&new_t_in[ray], nti);
        const sse_t nto = min4(load44(&t_out[ray]),load44(&rays.getMinT(ray)));
        store44(&new_t_out[ray], nto);
        const sse_t nvi = and4(load44(reinterpret_cast<const float*>(&valid[ray])),
                               cmp4_le(nti,nto));
        store44(reinterpret_cast<float*>(&new_valid[ray]), nvi);

        backMask = or4(backMask,load44(reinterpret_cast<float*>(&new_valid[ray])));
      }
      packetValid += getmask4(backMask);

#else

      // now, clip to back plane ...
      for(int ray=ray_begin; ray<ray_end; ray++) {
        if (!valid[ray]) {
          // ray does not traverse either node
          new_valid[ray] = false;
        } else {
          if (t_in[ray] > t_plane[ray]) {
            // ray does not traverse front node, but it does the back.
            new_t_in[ray] = t_in[ray];
            // minT might may have changed in prev isec step due to hitting
            // triangle that straddles nodes
            new_t_out[ray] = t_out[ray];//Min(t_out[ray], subpacket.getMinT(ray));
            new_valid[ray] = true;//(new_t_in[ray] <= new_t_out[ray]);
          } else if (t_out[ray] < t_plane[ray]) {
            // ray does not traverse back node, but it does the front.
            new_valid[ray] = false;
          } else {
            new_t_in[ray] = t_plane[ray]; //t_in[ray];
            new_t_out[ray] = Min(t_out[ray],rays.getMinT(ray));
            new_valid[ray] = (new_t_in[ray]<=new_t_out[ray]);
          }
          packetValid += new_valid[ray];
        }
      }

#endif //SSE

      if (packetValid) {
        intersectNode<COMMON_ORIGIN>(backChildID,context,rays,
                      new_t_in,new_t_out,new_valid
#ifdef MAILBOX
                      , mailbox
#endif
                      );
      }
    }
  }
}

// single ray traversal
template<bool anyHit>
void KDTree::traverse(const RenderContext &context, RayPacket &ray,
                      const Vector &org, const Vector &dir,
                      const Vector &rcp, TrvStack *const stackBase)
  const
{
#ifdef COLLECT_STATS
  stats[context.proc].nTotalRays += ray.end() - ray.begin();
  stats[context.proc].nTotalRaysInPacket += ray.end() - ray.begin();
  stats[context.proc].nTotalPackets++;
#endif
  float t_n = T_EPSILON;
  float t_f = ray.getMinT( ray.begin() );

  const bool signs[3] = {dir[0] < 0, dir[1] < 0, dir[2] < 0};

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
  unsigned int nodeID = 0;

  const int MAX_TRANSPARENT_HITS = 50;
  Real transparentHits[MAX_TRANSPARENT_HITS];
  int transparentHitsSize=0;

  while (1) {
    const Node &node = nodes[nodeID];
    if (node.isLeaf) {

#ifdef COLLECT_STATS
      ++stats[context.proc].nLeavesVisited;
      if (node.numPrimitives==0)
        ++stats[context.proc].nEmptyLeavesVisited;
#endif

      for (int i=0; i<node.numPrimitives; ++i) {
        currGroup->get(itemList[node.childIdx+i])->intersect(context, ray);

#ifdef COLLECT_STATS
        stats[context.proc].nIntersects += node.numPrimitives;
#endif
#if 1
        if (anyHit && ray.wasHit(ray.begin())) {
          int count;
          for (count = transparentHitsSize-1; count >= 0; --count) {
            if ( Abs(ray.getMinT(ray.begin()) - transparentHits[count]) < T_EPSILON)
              break;
          }
          if (count < 0) {
            transparentHits[transparentHitsSize++] = ray.getMinT(ray.begin());
            ray.getHitMaterial(ray.begin())->attenuateShadows(context, ray);
          }
          if (ray.getColor(ray.begin()) == Color::black())
            return;
          ray.resetHit(ray.begin()); // don't need to reset to original light
                                     // distance since t_f handles that for us.
          if (transparentHitsSize >= MAX_TRANSPARENT_HITS)
            return;
        }
#endif

      }

      if (stackPtr <= stackBase)
        return;

      --stackPtr;

      const float newT = ray.getMinT( ray.begin() );

      if (anyHit) {
        if (ray.wasHit(ray.begin()))
          return;
      }
      else if (newT <= t_f)
        return;

      nodeID = stackPtr->nodeID;

      if (anyHit)
        t_n = stackPtr->t_n;
      else
        t_n = t_f;

      t_f = Min(newT,stackPtr->t);
    }
    else {

#ifdef COLLECT_STATS
      stats[context.proc].nTraversals++;
#endif
      const int planeDim = node.planeDim;
      const float t_p = (node.planePos - org[planeDim]) * rcp[planeDim];

      const int frontChild = node.childIdx+signs[planeDim];
      const int backChild  = node.childIdx+1-signs[planeDim];

      if (t_p < t_n) {
        nodeID = backChild;
      }
      else if (t_p > t_f) {
        nodeID = frontChild;
      }
      else {
#ifdef RTSAH
        //cost ordered traversal
        if (!anyHit ||
            (static_cast<bool>(node.isLeftCheaper) == !signs[planeDim])) {
#endif
          stackPtr->nodeID = backChild;
          stackPtr->t = t_f;
          if (anyHit)
            stackPtr->t_n = t_p;

          ++stackPtr;

          nodeID = frontChild;
          t_f = t_p;
#ifdef RTSAH
        }
        else {
          //traverse the backchild first
          stackPtr->nodeID = frontChild;
          stackPtr->t = t_p;
          stackPtr->t_n = t_n;
          ++stackPtr;
          nodeID = backChild;
          t_n = t_p;
        }
#endif
      }
    }
  }
};

/**
 * Test whether any of the rays in a ray packet intersects this
 * bounding box.
 *
 * @param  rays[in]  the set of rays to check.
 * @return  true if any rays touch, false if all of them miss.
 */
bool KDTree::intersectBounds(const RenderContext& context, RayPacket& rays) const
{
  throw InternalError("hopefully done need this function");
  return false;
}


bool KDTree::buildFromFile(const string &file)
{
  double startTime = Time::currentSeconds();

  bounds.reset();

  //Something's wrong, let's bail. Since we reset the bounds right
  //above, this KDTree should never get intersected.
  if (!currGroup)
    return false;

  PreprocessContext context;
  currGroup->computeBounds(context, bounds);

  nodes.clear();

  mesh = dynamic_cast<Mesh*>(currGroup);

#if 0
  ifstream in(file.c_str());
  if (!in) return false;
  unsigned int itemListSize;
  in >> itemListSize;

  for (unsigned int i=0; i < itemListSize; ++i) {
    int item;
    in >> item;
    if (!in) return false;
    itemList.push_back(item);
  }

  while (in.good()) {
    Node n;
    int i;
    unsigned int u;

    in >> i;
    n.isLeaf = i;

    if (in.eof())
      break;

    if (n.isLeaf) {
      in >> n.numPrimitives;
      in >> u;
      n.childIdx = u;
    }
    else {
      int type;
      in >> type;

      if (type != 0)
        return false; //can only read KDTree format.

      float f;
      in >> f;
      n.planePos = f;


      in >> u;
      n.planeDim = u;
      in >> u;
      n.childIdx = u;
    }
    nodes.push_back(n);
  }
#else
  ifstream in(file.c_str(), ios::binary | ios::in);
  if (!in)
    return false;

  unsigned int itemList_size;
  in.read((char*)&itemList_size, sizeof(itemList_size));
  itemList.resize(itemList_size);

  in.read((char*) &itemList[0], sizeof(itemList[0])*itemList.size());

  unsigned int node_size;
  in.read((char*)&node_size, sizeof(node_size));
  nodes.resize(node_size);

  in.read((char*) &nodes[0], sizeof(Node)*nodes.size());
#endif

  in.close();

  double buildTime = Time::currentSeconds() - startTime;
  printf("KDTree tree loaded in %f seconds\n", buildTime);
  printStats();

  return true;
}


bool KDTree::saveToFile(const string &file)
{
  if (nodes.empty()) {
    needToSaveFile = true;
    saveFileName = file;
    return true; // We support it, so we return true now.  Hopefully it'll end
                 // up working when this actually occurs...
  }
  needToSaveFile = false;

#if 0
  ofstream out(file.c_str());
  if (!out) return false;

  out << itemList.size() << endl;
  for (unsigned int i=0; i < itemList.size(); ++i) {
    out << itemList[i] <<" ";
  }

  out<<endl;

  for (unsigned int i=0; i < nodes.size(); ++i) {
    out << nodes[i].isLeaf << " ";
    if (nodes[i].isLeaf) {
      out << nodes[i].numPrimitives << " " <<  nodes[i].childIdx <<endl;
    }
    else {
      out << 0 << " " << nodes[i].planePos << " " << nodes[i].planeDim << " " <<  nodes[i].childIdx<<endl;
    }
  }
#else
  ofstream out(file.c_str(), ios::out | ios::binary);
  if (!out) return false;

  const unsigned int itemList_size = itemList.size();
  const unsigned int node_size = nodes.size();
  out.write((char*) &itemList_size, sizeof(itemList_size));
  out.write((char*) &itemList[0], sizeof(itemList[0])*itemList_size);
  out.write((char*) &node_size, sizeof(node_size));
  out.write((char*) &nodes[0], sizeof(Node)*node_size);
#endif
  out.close();
  return true;
}

void KDTree::printStats()
{
  TreeStats treeStats = {0};
  collectTreeStats(0, 0, treeStats);

#ifdef RTSAH
  computeTraversalCost();
#endif

  printf("KDtree made from %d primitives has: \n", (int) currGroup->size());
  printf(" - %d nodes\n", (int)nodes.size());
  printf(" - %d primitive references\n", (int)itemList.size());
  printf(" - %d max depth\n", treeStats.maxDepth);
  printf(" - %d max objects in a leaf\n", treeStats.maxObjectsInLeaf);

  for (int i=0; i<1024; ++i)
    if (treeStats.childrenLeafHist[i] > 0)
      printf(" - %d leaves with %d children\n", treeStats.childrenLeafHist[i], i);
}

void KDTree::collectTreeStats(unsigned int nodeID, int depth, TreeStats &stats)
{
  const Node &node = nodes[nodeID];
  if (node.isLeaf) {
    stats.maxDepth = Max(stats.maxDepth, depth);
    stats.maxObjectsInLeaf = Max(stats.maxObjectsInLeaf,
                                 node.numPrimitives);
    stats.childrenLeafHist[std::min(node.numPrimitives,1023)]++;
  }
  else {
    collectTreeStats(node.childIdx+0, depth+1, stats);
    collectTreeStats(node.childIdx+1, depth+1, stats);
  }
}

void KDTree::computeTraversalCost()
{
  double start = Time::currentSeconds();
  float cost = computeSubTreeTraversalCost(0, bounds, bounds.computeArea())[0];
  double end = Time::currentSeconds();

  cout << "KDTree traversal cost: " << cost << " computed in "
       <<(end-start)*1000<<"ms"<<endl;
}

VectorT<float, 2> KDTree::computeSubTreeTraversalCost(unsigned int nodeID,
                                                      const BBox& nodeBounds,
                                                      float nodeSA)
{
#ifdef RTSAH
  Node &node = nodes[nodeID];
  if (node.isLeaf) {
    float immediateCost = node.numPrimitives * ISEC_COST + TRAV_COST;
    float futureCost = node.numPrimitives==0 ? 1 : 0;
    for (int i=0; i < node.numPrimitives; ++i) {
      if (mesh) {
        const MeshTriangle* tri = mesh->get(itemList[node.childIdx+i]);
        if (tri->getMaterial()->canAttenuateShadows()) {
          //futureCost += 1.0/node.numPrimitives;
          futureCost = 1;
          break;
        }
      }
    }

    return VectorT<float, 2> (immediateCost, futureCost);
  }
  else { // internal node
    const float inv_nodeSA = 1.0f / nodeSA;

    BBox lBounds, rBounds;
    lBounds = rBounds = nodeBounds;
    lBounds[1][nodes[nodeID].planeDim] = nodes[nodeID].planePos;
    rBounds[0][nodes[nodeID].planeDim] = nodes[nodeID].planePos;

    const float l_SA = lBounds.computeArea();
    const float r_SA = rBounds.computeArea();
    const float Pl = l_SA * inv_nodeSA;
    const float Pr = r_SA * inv_nodeSA;
    const float Pjl = 1-Pr;
    const float Pjr = 1-Pl;
    const float Plr = 1-(Pjl+Pjr);

    VectorT<float, 2> Cl = computeSubTreeTraversalCost(node.childIdx+0, lBounds, l_SA);
    VectorT<float, 2> Cr = computeSubTreeTraversalCost(node.childIdx+1, rBounds, r_SA);

    VectorT<float, 2> cost = Pjl*Cl + Pjr*Cr;

    cost[0] += TRAV_COST;

    // Handle the lr case where ray pierces both and a ray may enter one child
    // and then have to enter the other child if it only hit empty leaves
    VectorT<float, 2> bothCl(Cl[0], 0);
    VectorT<float, 2> bothCr(Cr[0], 0);
    bothCl += Cl[1]*Cr;
    bothCr += Cr[1]*Cl;

    // Note: bothCl[1] == bothCr[1] == Cl[1]*Cr[1] at this point.

    VectorT<float, 2> minBothC = Min(bothCl, bothCr);
#if 0
    // If we randomly picked which child to enter first (or always pick the
    // front child, as in a front to back traversal), then we would get the
    // following RTSAH.
    minBothC = .5*(bothCl+bothCr);
#endif

    cost += Plr*(minBothC);

    // During traversal, we only check which node is cheaper when we have a ray that
    // is traversing both nodes.  For this reason we only want to factor in the cost
    // for the ray hitting both nodes.

    node.isLeftCheaper = bothCl[0] < bothCr[0];

    return cost;
  }
#endif
}
