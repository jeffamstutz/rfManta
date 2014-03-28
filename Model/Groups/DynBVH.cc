#include <Core/Exceptions/InternalError.h>
#include <Core/Math/MiscMath.h>
#include <Core/Thread/Time.h>
#include <Core/Util/Preprocessor.h>
#include <Core/Util/UpdateGraph.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Core/Persistent/stdRTTI.h>
#include <DynBVH_Parameters.h>
#include <Interface/Context.h>
#include <Interface/Task.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/MantaInterface.h>
#include <Model/Primitives/MeshTriangle.h>
#include <Model/Groups/DynBVH.h>
#include <algorithm>
#include <float.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdio.h>
#include <stdlib.h>

using namespace Manta;
using std::cerr;

#define TEST_MASKS 1
// these constants control the SAH cost model
const float BVH_C_isec = 1.f;
const float BVH_C_trav = 1.f;
// NOTE(boulos): In Ingo's paper he says 16 works better than 8, but
// I'm not sure for which models yet. For bunny it makes no
// difference.
const int BVH_num_samples = 16;

// Lazy build has a slight rendering overhead and a more significant memory
// overhead.  But it allows for substantially faster builds.

#define LONGEST_AXIS 0
#define USE_PARALLEL_BUILD 1

// RTSAH requires a fully built tree, so it does not work with lazy builds.
// Getting RTSAH to work with lazy builds would be interesting future work.
// For instance, it would be much cheaper to intersect a fully built subtree
// than one that needs to be built, so build cost could be factored in.
#if USE_LAZY_BUILD == 0
// define RTSAH if you want to use RTSAH traversal order
# define RTSAH
#endif


#include <Model/Groups/Mesh.h>
void WriteMeshSorted(Group* group, std::vector<int> ids, const char* filename) {
  FILE* output = fopen(filename, "w");
  Mesh* mesh = dynamic_cast<Mesh*>(group);
  // Write out the verts
  std::vector<Vector>& verts = mesh->vertices;
  for (size_t i = 0; i < verts.size(); i++) {
    fprintf(output, "v %f %f %f\n", verts[i][0], verts[i][1], verts[i][2]);
  }
  fprintf(output, "\n");
  // Write out the faces but in the order the BVH produced
  std::vector<unsigned int>& indices = mesh->vertex_indices;
  for (size_t i = 0; i < ids.size(); i++) {
    int which_tri = ids[i];
    int index = 3 * which_tri;
    fprintf(output, "f %d %d %d\n",
            indices[index + 0] + 1,
            indices[index + 1] + 1,
            indices[index + 2] + 1);
  }
  fprintf(output, "\n");
  fclose(output);
}

void
DynBVH::printNode(int nodeID, int depth) const {
  BVHNode& node = nodes[nodeID];

  std::string indent;
  for (int i = 0; i < depth; i++) {
    indent += "  ";
  }

  cerr << indent << "NodeID = " << nodeID;

  if (node.isLeaf()) {
    cerr << " LEAF w/ " << node.children << " children.\n";
  } else {
    cerr << " INTR w/ bounds " << node.bounds << endl;
    printNode(node.child+0, depth+1);
    printNode(node.child+1, depth+1);
  }
}

DynBVH::~DynBVH()
{
  //  cerr << MANTA_FUNC << " called.\n";
}

void DynBVH::intersect(const RenderContext& context, RayPacket& rays) const
{
  bool debugFlag = rays.getFlag(RayPacket::DebugPacket);
  if (debugFlag) {
    //cerr << __func__ << " called\n";
    cerr << MANTA_FUNC << " called\n";
    cerr << "Rays are : \n" << rays << endl;
  }

#ifdef COLLECT_STATS
  stats[context.proc].nTotalRays += rays.end() - rays.begin();
  stats[context.proc].nTotalRaysInPacket += rays.end() - rays.begin();
  stats[context.proc].nTotalPackets++;
#endif


  rays.computeInverseDirections();
#if USE_DYNBVH_PORTS
  templatedTraverse(context, rays);
#else
  rays.computeSigns();

  // compute IntervalArithmetic Data
  IAData ia_data;
  for (int axis = 0; axis < 3; axis++ ) {
      ia_data.min_rcp[axis]     =  std::numeric_limits<float>::max();
      ia_data.max_rcp[axis]     = -std::numeric_limits<float>::max();
      ia_data.min_org[axis]     =  std::numeric_limits<float>::max();
      ia_data.max_org[axis]     = -std::numeric_limits<float>::max();
      ia_data.min_org_rcp[axis] =  std::numeric_limits<float>::max();
      ia_data.max_org_rcp[axis] = -std::numeric_limits<float>::max();
  }

#ifdef MANTA_SSE
  int b = (rays.begin() + 3) & ~3;
  int e = (rays.end()) & ~3;
  if (b >= e) {
    for (int ray = rays.begin(); ray < rays.end(); ray++ ) {
#if TEST_MASKS
      if (rays.rayIsMasked(ray)) continue;
#endif
      for (int axis = 0; axis < 3; axis++) {
        const Real new_rcp     = rays.getInverseDirection(ray, axis);
        const Real new_org     = rays.getOrigin(ray,axis);
        const Real new_org_rcp = new_org * new_rcp;

        ia_data.min_rcp[axis] = (ia_data.min_rcp[axis] < new_rcp) ? ia_data.min_rcp[axis] : new_rcp;
        ia_data.max_rcp[axis] = (ia_data.max_rcp[axis] < new_rcp) ? new_rcp : ia_data.max_rcp[axis];

        ia_data.min_org[axis] = (ia_data.min_org[axis] < new_org) ? ia_data.min_org[axis] : new_org;
        ia_data.max_org[axis] = (ia_data.max_org[axis] < new_org) ? new_org : ia_data.max_org[axis];

        ia_data.min_org_rcp[axis] = (ia_data.min_org_rcp[axis] < new_org_rcp) ?
          ia_data.min_org_rcp[axis] : new_org_rcp;
        ia_data.max_org_rcp[axis] = (ia_data.max_org_rcp[axis] < new_org_rcp) ?
          new_org_rcp : ia_data.max_org_rcp[axis];
      }
    }
  } else {
    for (int ray = rays.begin(); ray < b; ray++) {
#if TEST_MASKS
      if (rays.rayIsMasked(ray)) continue;
#endif
      for (int axis = 0; axis < 3; axis++) {
        const Real new_rcp     = rays.getInverseDirection(ray, axis);
        const Real new_org     = rays.getOrigin(ray,axis);
        const Real new_org_rcp = new_org * new_rcp;

        ia_data.min_rcp[axis] = (ia_data.min_rcp[axis] < new_rcp) ? ia_data.min_rcp[axis] : new_rcp;
        ia_data.max_rcp[axis] = (ia_data.max_rcp[axis] < new_rcp) ? new_rcp : ia_data.max_rcp[axis];

        ia_data.min_org[axis] = (ia_data.min_org[axis] < new_org) ? ia_data.min_org[axis] : new_org;
        ia_data.max_org[axis] = (ia_data.max_org[axis] < new_org) ? new_org : ia_data.max_org[axis];

        ia_data.min_org_rcp[axis] = (ia_data.min_org_rcp[axis] < new_org_rcp) ?
          ia_data.min_org_rcp[axis] : new_org_rcp;
        ia_data.max_org_rcp[axis] = (ia_data.max_org_rcp[axis] < new_org_rcp) ?
          new_org_rcp : ia_data.max_org_rcp[axis];
      }
    }
    __m128 min_rcp[3];
    __m128 max_rcp[3];
    __m128 min_org[3];
    __m128 max_org[3];
    __m128 min_org_rcp[3];
    __m128 max_org_rcp[3];
    // Copy current values
    for (int axis = 0; axis < 3; axis++) {
      min_rcp[axis] = _mm_set1_ps(ia_data.min_rcp[axis]);
      max_rcp[axis] = _mm_set1_ps(ia_data.max_rcp[axis]);
      min_org[axis] = _mm_set1_ps(ia_data.min_org[axis]);
      max_org[axis] = _mm_set1_ps(ia_data.max_org[axis]);
      min_org_rcp[axis] = _mm_set1_ps(ia_data.min_org_rcp[axis]);
      max_org_rcp[axis] = _mm_set1_ps(ia_data.max_org_rcp[axis]);
    }
    // Loop over all rays over all 3 axes (note change in order for
    // cache friendly walk down the directions and origins)
    for (int axis = 0; axis < 3; axis++) {
      for (int ray = b; ray < e; ray += 4) {
        __m128 new_rcp = _mm_load_ps(&(rays.data->inverseDirection[axis][ray]));
        __m128 new_org = _mm_load_ps(&(rays.data->origin[axis][ray]));
        __m128 new_org_rcp = _mm_mul_ps(new_org, new_rcp);

        min_rcp[axis] = _mm_min_ps(min_rcp[axis], new_rcp);
        max_rcp[axis] = _mm_max_ps(max_rcp[axis], new_rcp);
        min_org[axis] = _mm_min_ps(min_org[axis], new_org);
        max_org[axis] = _mm_max_ps(max_org[axis], new_org);
        min_org_rcp[axis] = _mm_min_ps(min_org_rcp[axis], new_org_rcp);
        max_org_rcp[axis] = _mm_max_ps(max_org_rcp[axis], new_org_rcp);
      }
    }
    // Copy the results back out
    for (int axis = 0; axis < 3; axis++) {
      ia_data.min_rcp[axis] = min4f(min_rcp[axis]);
      ia_data.max_rcp[axis] = max4f(max_rcp[axis]);

      ia_data.min_org[axis] = min4f(min_org[axis]);
      ia_data.max_org[axis] = max4f(max_org[axis]);

      ia_data.min_org_rcp[axis] = min4f(min_org_rcp[axis]);
      ia_data.max_org_rcp[axis] = max4f(max_org_rcp[axis]);
    }
    for (int ray = e; ray < rays.end(); ray++) {
#if TEST_MASKS
      if (rays.rayIsMasked(ray)) continue;
#endif
      for (int axis = 0; axis < 3; axis++) {
        const Real new_rcp     = rays.getInverseDirection(ray, axis);
        const Real new_org     = rays.getOrigin(ray,axis);
        const Real new_org_rcp = new_org * new_rcp;

        ia_data.min_rcp[axis] = (ia_data.min_rcp[axis] < new_rcp) ? ia_data.min_rcp[axis] : new_rcp;
        ia_data.max_rcp[axis] = (ia_data.max_rcp[axis] < new_rcp) ? new_rcp : ia_data.max_rcp[axis];

        ia_data.min_org[axis] = (ia_data.min_org[axis] < new_org) ? ia_data.min_org[axis] : new_org;
        ia_data.max_org[axis] = (ia_data.max_org[axis] < new_org) ? new_org : ia_data.max_org[axis];

        ia_data.min_org_rcp[axis] = (ia_data.min_org_rcp[axis] < new_org_rcp) ?
          ia_data.min_org_rcp[axis] : new_org_rcp;
        ia_data.max_org_rcp[axis] = (ia_data.max_org_rcp[axis] < new_org_rcp) ?
          new_org_rcp : ia_data.max_org_rcp[axis];
      }
    }
  }
#else
  for (int ray = rays.begin(); ray < rays.end(); ray++ ) {
#if TEST_MASKS
    if (rays.rayIsMasked(ray)) continue;
#endif
    for (int axis = 0; axis < 3; axis++) {
      const Real new_rcp     = rays.getInverseDirection(ray, axis);
      const Real new_org     = rays.getOrigin(ray,axis);
      const Real new_org_rcp = new_org * new_rcp;

      ia_data.min_rcp[axis] = (ia_data.min_rcp[axis] < new_rcp) ? ia_data.min_rcp[axis] : new_rcp;
      ia_data.max_rcp[axis] = (ia_data.max_rcp[axis] < new_rcp) ? new_rcp : ia_data.max_rcp[axis];

      ia_data.min_org[axis] = (ia_data.min_org[axis] < new_org) ? ia_data.min_org[axis] : new_org;
      ia_data.max_org[axis] = (ia_data.max_org[axis] < new_org) ? new_org : ia_data.max_org[axis];

      ia_data.min_org_rcp[axis] = (ia_data.min_org_rcp[axis] < new_org_rcp) ?
        ia_data.min_org_rcp[axis] : new_org_rcp;
      ia_data.max_org_rcp[axis] = (ia_data.max_org_rcp[axis] < new_org_rcp) ?
        new_org_rcp : ia_data.max_org_rcp[axis];
    }
  }
#endif


  intersectNode(0,context,rays, ia_data);
#endif // dynbvh port or not
}

void DynBVH::intersectNode(int nodeID, const RenderContext& context,
                           RayPacket& rays, const IAData& ia_data) const
{
#if USE_LAZY_BUILD
  lazyBuild(context, nodeID);
#endif
  const BVHNode& node = nodes[nodeID];
  int firstActive = firstIntersects(node.bounds, rays, ia_data);

#ifdef COLLECT_STATS
  stats[context.proc].nTraversals += (rays.end()-rays.begin());
#endif

  if (firstActive != rays.end()) {
    if (node.isLeaf()) {
#ifdef COLLECT_STATS
      stats[context.proc].nLeavesVisited += (rays.end()-rays.begin());
#endif

      // we already know that one of the rays from begin to end hits the
      // object, so lastActive is going to find something we need not create a
      // subpacket to help it stop early

      // actually if we're just one ray it'll look more than it needs to since
      // it will reintersect the firstActive ray...
      int lastActive = lastIntersects(node.bounds, rays);

      // build a subpacket from firstActive to lastActive (inclusive, hence +1)
      RayPacket subpacket(rays, firstActive, lastActive+1);

      for (int i = 0; i < node.children; i++ ) {
#ifdef COLLECT_STATS
        stats[context.proc].nIntersects += (subpacket.end()-subpacket.begin());
#endif

        const int object_id = object_ids[node.child+i];
#if 0
        currGroup->get(object_id)->intersect(context, subpacket);
#else
        const bool anyHit = subpacket.getFlag(RayPacket::AnyHit);

        if (anyHit) {

          // Save previous t values/hit points.
          Real t[RayPacket::MaxSize];
          for (int r = subpacket.begin(); r < subpacket.end(); ++r)
            t[r] = subpacket.getMinT(r);

          currGroup->get(object_id)->intersect(context, subpacket);

          bool somethingTerminated = false;

          // This block of code is only required for attenuating materials.
          // For occluding materials this introduces a (very) small performance
          // hit and could be safely commented out.
          for (int i = subpacket.begin(); i < subpacket.end(); ++i) {
            if (!subpacket.wasHit(i) || subpacket.rayIsMasked(i))
              continue;
            int end = i+1;
            const Material* hit_matl = subpacket.getHitMaterial(i);
            while (end < subpacket.end() && subpacket.wasHit(end) &&
                   subpacket.getHitMaterial(end) == hit_matl) {
              end++;
            }

            RayPacket shadingPacket(subpacket, i, end);
            hit_matl->attenuateShadows(context, shadingPacket);
            for (int j=i; j < end; ++j) {
              if (shadingPacket.getColor(j) != Color::black()) {
                shadingPacket.resetHit(j, t[j]);
              }
              else {
                somethingTerminated = true;
                // Do not allow anything else to count as hit.  Doing so could
                // replace a previously found occluder with a closer
                // transparent material and result in a lost early ray
                // termination optimization.  If there are no transparent
                // materials, then this is superfluous work.
                shadingPacket.overrideMinT(j, T_EPSILON); // T_EPSILON should be small enough.
              }
            }
            i=end-1;
          }

          // Shrink packet. Get rid of terminated (fully occluded) rays on the
          // ends of the packet.
          if (somethingTerminated) {
            int start, end;
            for (start = subpacket.begin(); start < subpacket.end(); ++start) {
              if (!subpacket.wasHit(start) && !subpacket.rayIsMasked(start))
                break;
            }
            for (end = subpacket.end()-1; end > start; --end) {
              if (!subpacket.wasHit(end) && !subpacket.rayIsMasked(end))
                break;
            }
            subpacket.resize(start, end+1);
            if (end < start) {
              break;
            }
          }
        }
        else {
          currGroup->get(object_id)->intersect(context, subpacket);
        }
#endif
      }
    }
    else {
      // make a subpacket from the (possibly) new firstActive to the current end
      RayPacket subpacket(rays, firstActive, rays.end());
      // recurse
#ifdef RTSAH
      const bool anyHit = subpacket.getFlag(RayPacket::AnyHit);
      int front_son;
      if (anyHit) {
        front_son = node.isLeftCheaper ? 0 : 1;
      }
      else {
        front_son = subpacket.getDirection(subpacket.begin(),
                                           static_cast<int>(node.axis)) > 0 ? 0 : 1;
      }
#else
      int front_son = subpacket.getDirection(subpacket.begin(),
                                             static_cast<int>(node.axis)) > 0 ? 0 : 1;
#endif
      intersectNode(node.child+front_son, context, subpacket, ia_data);
      intersectNode(node.child+1-front_son, context, subpacket, ia_data);
    }
  }
}

// return the first index (between [rays.begin(),rays.end()]) which hits the box
int DynBVH::firstIntersects(const BBox& box, const RayPacket& rays, const IAData& ia_data)
{

  const bool anyHit = rays.getFlag(RayPacket::AnyHit);

  int rayBegin = rays.begin();
  int rayEnd = rays.end();

  for (; rayBegin < rayEnd; rayBegin++) {
#if TEST_MASKS
    if (rays.rayIsMasked(rayBegin)) continue;
#endif
    if (anyHit && rays.wasHit(rayBegin)) continue;
    break;
  }

#ifdef MANTA_SSE
  int b = (rayBegin + 3) & (~3);
  int e = rayEnd & (~3);
  if (b >=e) {
    for (int i = rayBegin; i < rayEnd; i++) {
#if TEST_MASKS
      if (rays.rayIsMasked(i)) continue;
#endif
      if (anyHit && rays.wasHit(i)) continue;

      __m128 t00 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[0][0]), _mm_set_ss(rays.getOrigin(i, 0))),
                              _mm_set_ss(rays.getInverseDirection(i, 0)));
      __m128 t01 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[1][0]), _mm_set_ss(rays.getOrigin(i, 0))),
                              _mm_set_ss(rays.getInverseDirection(i, 0)));
      __m128 tmin0 = _mm_max_ss(_mm_min_ss(t00, t01), _mm_set_ss(T_EPSILON));
      __m128 tmax0 = _mm_min_ss(_mm_max_ss(t00, t01), _mm_set_ss(rays.getMinT(i)));

      __m128 t10 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[0][1]), _mm_set_ss(rays.getOrigin(i, 1))),
                              _mm_set_ss(rays.getInverseDirection(i, 1)));
      __m128 t11 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[1][1]), _mm_set_ss(rays.getOrigin(i, 1))),
                              _mm_set_ss(rays.getInverseDirection(i, 1)));
      __m128 tmin1 = _mm_max_ss(_mm_min_ss(t10, t11), tmin0);
      __m128 tmax1 = _mm_min_ss(_mm_max_ss(t10, t11), tmax0);

      __m128 t20 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[0][2]), _mm_set_ss(rays.getOrigin(i, 2))),
                              _mm_set_ss(rays.getInverseDirection(i, 2)));
      __m128 t21 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[1][2]), _mm_set_ss(rays.getOrigin(i, 2))),
                              _mm_set_ss(rays.getInverseDirection(i, 2)));
      __m128 tmin2 = _mm_max_ss(_mm_min_ss(t20, t21), tmin1);
      __m128 tmax2 = _mm_min_ss(_mm_max_ss(t20, t21), tmax1);

      float tmin, tmax;
      _mm_store_ss(&tmin, tmin2);
      _mm_store_ss(&tmax, tmax2);
      if(tmin <= tmax){
        return i;
      }
    }
  } else {
    int i = rayBegin;
    {
      __m128 t00 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[0][0]), _mm_set_ss(rays.getOrigin(i, 0))),
                              _mm_set_ss(rays.getInverseDirection(i, 0)));
      __m128 t01 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[1][0]), _mm_set_ss(rays.getOrigin(i, 0))),
                              _mm_set_ss(rays.getInverseDirection(i, 0)));
      __m128 tmin0 = _mm_max_ss(_mm_min_ss(t00, t01), _mm_set_ss(T_EPSILON));
      __m128 tmax0 = _mm_min_ss(_mm_max_ss(t00, t01), _mm_set_ss(rays.getMinT(i)));

      __m128 t10 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[0][1]), _mm_set_ss(rays.getOrigin(i, 1))),
                              _mm_set_ss(rays.getInverseDirection(i, 1)));
      __m128 t11 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[1][1]), _mm_set_ss(rays.getOrigin(i, 1))),
                              _mm_set_ss(rays.getInverseDirection(i, 1)));
      __m128 tmin1 = _mm_max_ss(_mm_min_ss(t10, t11), tmin0);
      __m128 tmax1 = _mm_min_ss(_mm_max_ss(t10, t11), tmax0);

      __m128 t20 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[0][2]), _mm_set_ss(rays.getOrigin(i, 2))),
                              _mm_set_ss(rays.getInverseDirection(i, 2)));
      __m128 t21 = _mm_mul_ss(_mm_sub_ss(_mm_set_ss(box[1][2]), _mm_set_ss(rays.getOrigin(i, 2))),
                              _mm_set_ss(rays.getInverseDirection(i, 2)));
      __m128 tmin2 = _mm_max_ss(_mm_min_ss(t20, t21), tmin1);
      __m128 tmax2 = _mm_min_ss(_mm_max_ss(t20, t21), tmax1);

      float tmin, tmax;
      _mm_store_ss(&tmin, tmin2);
      _mm_store_ss(&tmax, tmax2);
      if(tmin <= tmax){
        return i;
      }
    }

    // try a frustum miss
    {
      float tmin_frustum = T_EPSILON;
      float tmax_frustum = FLT_MAX;

      for (int axis = 0; axis < 3; axis++) {
        // the subtraction is really (boxIA + -orgIA)
        // or boxIA + [-max_org, -min_org]
        // [box_min, box_max] + [-max_org, -min_org]
        float a = box[0][axis]-ia_data.max_org[axis];
        float b = box[1][axis]-ia_data.min_org[axis];

        // now for multiplication
        float ar0 = a * ia_data.min_rcp[axis];
        float ar1 = a * ia_data.max_rcp[axis];
        float br0 = b * ia_data.min_rcp[axis];
        float br1 = b * ia_data.max_rcp[axis];

        // [a,b] is valid intersection interval so a is min
        // and b is max t-value

        //a = std::min(ar0, std::min(ar1, std::min(br0, br1)));
        a = (br0 < br1) ? br0 : br1;
        a = (a   < ar1) ?   a : ar1;
        a = (a   < ar0) ?   a : ar0;

        //b = std::max(ar0, std::max(ar1, std::max(br0, br1)));
        b = (br0 < br1) ? br1 : br0;
        b = (b   < ar1) ? ar1 : b;
        b = (b   < ar0) ? ar0 : b;

        tmin_frustum = (tmin_frustum < a) ? a : tmin_frustum;
        tmax_frustum = (tmax_frustum > b) ? b : tmax_frustum;
      }

      // frustum exit
      if (tmin_frustum > tmax_frustum) {
        return rayEnd;
      }
    }

    if (i < b) i++; // Avoid redoing first ray, but only if we won't do it in SIMD

    // Scalar pickup loop
    for (; i < b; i++) {
#if TEST_MASKS
      if (rays.rayIsMasked(i)) continue;
#endif
      if (anyHit && rays.wasHit(i)) continue;
      float tmin = T_EPSILON;
      float tmax = rays.getMinT(i);

      for (int c = 0; c < 3; c++) {
        float t0 = (box[0][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);
        float t1 = (box[1][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);

        float near = (t0 < t1) ? t0 : t1;
        float far  = (t0 < t1) ? t1 : t0;
        tmin = (tmin < near) ? near : tmin; // max of tmin, near
        tmax = (far <  tmax) ? far : tmax;  // min of tmax, far
      }
      if (tmin <= tmax) {  // valid intersection
        return i;
      }
    }

    RayPacketData* data = rays.data;
    __m128 box_x0 = _mm_set1_ps(box[0][0]);
    __m128 box_x1 = _mm_set1_ps(box[1][0]);

    __m128 box_y0 = _mm_set1_ps(box[0][1]);
    __m128 box_y1 = _mm_set1_ps(box[1][1]);

    __m128 box_z0 = _mm_set1_ps(box[0][2]);
    __m128 box_z1 = _mm_set1_ps(box[1][2]);

    for(;i<e;i+=4) {
#if TEST_MASKS
      if (rays.groupIsMasked(i)) continue;
#endif
      // TODO; check if adding to valid_intersect whether groupIsMasked is faster.
      __m128 valid_intersect;
      if (anyHit) {
          valid_intersect = rays.wereNotHitSSE(i);
          if (getmask4(valid_intersect)==0) continue;
      }
      else
        valid_intersect = _mm_true;

      __m128 x0 = _mm_mul_ps(_mm_sub_ps(box_x0, _mm_load_ps(&data->origin[0][i])),
                             _mm_load_ps(&data->inverseDirection[0][i]));
      __m128 x1 = _mm_mul_ps(_mm_sub_ps(box_x1, _mm_load_ps(&data->origin[0][i])),
                             _mm_load_ps(&data->inverseDirection[0][i]));

      __m128 xmin = _mm_min_ps(x0,x1);
      __m128 xmax = _mm_max_ps(x0,x1);

      __m128 y0 = _mm_mul_ps(_mm_sub_ps(box_y0, _mm_load_ps(&data->origin[1][i])),
                             _mm_load_ps(&data->inverseDirection[1][i]));
      __m128 y1 = _mm_mul_ps(_mm_sub_ps(box_y1, _mm_load_ps(&data->origin[1][i])),
                             _mm_load_ps(&data->inverseDirection[1][i]));

      __m128 ymin = _mm_min_ps(y0,y1);
      __m128 ymax = _mm_max_ps(y0,y1);

      __m128 z0 = _mm_mul_ps(_mm_sub_ps(box_z0, _mm_load_ps(&data->origin[2][i])),
                             _mm_load_ps(&data->inverseDirection[2][i]));
      __m128 z1 = _mm_mul_ps(_mm_sub_ps(box_z1, _mm_load_ps(&data->origin[2][i])),
                             _mm_load_ps(&data->inverseDirection[2][i]));

      __m128 zmin = _mm_min_ps(z0,z1);
      __m128 zmax = _mm_max_ps(z0,z1);

      __m128 maximum_minimum = _mm_max_ps(xmin,_mm_max_ps(ymin,_mm_max_ps(zmin, _mm_set1_ps(T_EPSILON))));
      __m128 minimum_maximum = _mm_min_ps(xmax,_mm_min_ps(ymax,_mm_min_ps(zmax,_mm_load_ps(&data->minT[i]))));
      valid_intersect = and4(valid_intersect, _mm_cmple_ps(maximum_minimum,minimum_maximum));
      const int mask = _mm_movemask_ps(valid_intersect);

      if (mask) {
        // Breaking simd alignment in order to return the most exact packet
        // interval gives a performance benefit when ray packets are not
        // coherent wrt the current bvh bounds.  This clearly gives a benefit
        // for path tracing/ambient occlusion like packets, but also for ray
        // casting/hard shadows with high res meshes (small triangles compared
        // to packet size).  The penalty when this is not the case is small and
        // in those cases the frame rate is already really high, so we optimize
        // for the more useful case.
        if ( (mask&0x1) && !rays.rayIsMasked(i+0))
          return i;
        else if ( (mask&0x2) && !rays.rayIsMasked(i+1))
          return i+1;
        else if ( (mask&0x4) && !rays.rayIsMasked(i+2))
          return i+2;
        else if ( (mask&0x8) && !rays.rayIsMasked(i+3))
          return i+3;
      }
    }

    for(;i<rayEnd;i++) {
#if TEST_MASKS
      if (rays.rayIsMasked(i)) continue;
#endif
      if (anyHit && rays.wasHit(i)) continue;

      float tmin = T_EPSILON;
      float tmax = rays.getMinT(i);

      for (int c = 0; c < 3; c++) {
        float t0 = (box[0][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);
        float t1 = (box[1][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);

        float near = (t0 < t1) ? t0 : t1;
        float far  = (t0 < t1) ? t1 : t0;
        tmin = (tmin < near) ? near : tmin; // max of tmin, near
        tmax = (far <  tmax) ? far : tmax;  // min of tmax, far
      }
      if (tmin <= tmax) {  // valid intersection
        return i;
      }
    }
  }
  return rayEnd;
#else // NOT MANTA_SSE (scalar case)

  for (int i = rayBegin; i < rayEnd; i++) {

    // Note, the frustum test will always occur since the rayBegin is never
    // masked or anyhit hit.
#if TEST_MASKS
    if (rays.rayIsMasked(i)) continue;
#endif
    if (anyHit && rays.wasHit(i)) continue;

    float tmin = T_EPSILON;
    float tmax = rays.getMinT(i);

    for (int c = 0; c < 3; c++) {
      float t0 = (box[0][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);
      float t1 = (box[1][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);

      float near = (t0 < t1) ? t0 : t1;
      float far  = (t0 < t1) ? t1 : t0;
      tmin = (tmin < near) ? near : tmin; // max of tmin, near
      tmax = (far <  tmax) ? far : tmax;  // min of tmax, far
    }
    if (tmin <= tmax) {  // valid intersection
      return i;
    }

    if (i == rayBegin) {
      // try a frustum miss
      float tmin_frustum = 1e-5;
      float tmax_frustum = FLT_MAX;

      for (int axis = 0; axis < 3; axis++) {
        // the subtraction is really (boxIA + -orgIA)
        // or boxIA + [-max_org, -min_org]
        // [box_min, box_max] + [-max_org, -min_org]
        float a = box[0][axis]-ia_data.max_org[axis];
        float b = box[1][axis]-ia_data.min_org[axis];

        // now for multiplication
        float ar0 = a * ia_data.min_rcp[axis];
        float ar1 = a * ia_data.max_rcp[axis];
        float br0 = b * ia_data.min_rcp[axis];
        float br1 = b * ia_data.max_rcp[axis];

        // [a,b] is valid intersection interval so a is min
        // and b is max t-value

        //a = std::min(ar0, std::min(ar1, std::min(br0, br1)));
        a = (br0 < br1) ? br0 : br1;
        a = (a   < ar1) ?   a : ar1;
        a = (a   < ar0) ?   a : ar0;

        //b = std::max(ar0, std::max(ar1, std::max(br0, br1)));
        b = (br0 < br1) ? br1 : br0;
        b = (b   < ar1) ? ar1 : b;
        b = (b   < ar0) ? ar0 : b;

        tmin_frustum = (tmin_frustum < a) ? a : tmin_frustum;
        tmax_frustum = (tmax_frustum > b) ? b : tmax_frustum;
      }

      // frustum exit
      if (tmin_frustum > tmax_frustum) {
        return rayEnd;
      }
    }
  }
  return rayEnd;
#endif
}

// return the last index which hits the box
int DynBVH::lastIntersects(const BBox& box, const RayPacket& rays)
{
  const bool anyHit = rays.getFlag(RayPacket::AnyHit);

  int rayBegin = rays.begin();
  int rayEnd = rays.end();

  for (; rayBegin < rayEnd; rayEnd--) {
#if TEST_MASKS
    if (rays.rayIsMasked(rayEnd-1)) continue;
#endif
    if (anyHit && rays.wasHit(rayEnd-1)) continue;
    break;
  }

#ifdef MANTA_SSE
  int last_simd = rayEnd & (~3);
  int first_simd = (rayBegin + 3) & (~3);
  if (first_simd >= last_simd) {
    for (int i = rayEnd - 1; i > rayBegin; i--) {
#if TEST_MASKS
      if (rays.rayIsMasked(i)) continue;
#endif
      if (anyHit && rays.wasHit(i)) continue;

      float tmin = T_EPSILON;
      float tmax = rays.getMinT(i);

      for (int c = 0; c < 3; c++) {
        float t0 = (box[0][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);
        float t1 = (box[1][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);

        float near = (t0 < t1) ? t0 : t1;
        float far  = (t0 < t1) ? t1 : t0;
        tmin = (tmin < near) ? near : tmin; // max of tmin, near
        tmax = (far <  tmax) ? far : tmax;  // min of tmax, far
      }
      if (tmin <= tmax) {  // valid intersection
        return i;
      }
    }
  } else {
    int i = rayEnd;
    for (; i > last_simd;) {
      i--;
#if TEST_MASKS
      if (rays.rayIsMasked(i)) continue;
#endif
      if (anyHit && rays.wasHit(i)) continue;

      float tmin = T_EPSILON;
      float tmax = rays.getMinT(i);

      for (int c = 0; c < 3; c++) {
        float t0 = (box[0][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);
        float t1 = (box[1][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);

        float near = (t0 < t1) ? t0 : t1;
        float far  = (t0 < t1) ? t1 : t0;
        tmin = (tmin < near) ? near : tmin; // max of tmin, near
        tmax = (far <  tmax) ? far : tmax;  // min of tmax, far
      }
      if (tmin <= tmax) {  // valid intersection
        return i;
      }
    }

    RayPacketData* data = rays.data;
    __m128 box_x0 = _mm_set1_ps(box[0][0]);
    __m128 box_x1 = _mm_set1_ps(box[1][0]);

    __m128 box_y0 = _mm_set1_ps(box[0][1]);
    __m128 box_y1 = _mm_set1_ps(box[1][1]);

    __m128 box_z0 = _mm_set1_ps(box[0][2]);
    __m128 box_z1 = _mm_set1_ps(box[1][2]);

    for(;i>first_simd;) {
      i -= 4;
#if TEST_MASKS
      if (rays.groupIsMasked(i)) continue;
#endif
      __m128 valid_intersect;
      if (anyHit) {
          valid_intersect = rays.wereNotHitSSE(i);
          if (getmask4(valid_intersect)==0) continue;
      }
      else
        valid_intersect = _mm_true;

      __m128 x0 = _mm_mul_ps(_mm_sub_ps(box_x0, _mm_load_ps(&data->origin[0][i])),
                             _mm_load_ps(&data->inverseDirection[0][i]));
      __m128 x1 = _mm_mul_ps(_mm_sub_ps(box_x1, _mm_load_ps(&data->origin[0][i])),
                             _mm_load_ps(&data->inverseDirection[0][i]));

      __m128 xmin = _mm_min_ps(x0,x1);
      __m128 xmax = _mm_max_ps(x0,x1);

      __m128 y0 = _mm_mul_ps(_mm_sub_ps(box_y0, _mm_load_ps(&data->origin[1][i])),
                             _mm_load_ps(&data->inverseDirection[1][i]));
      __m128 y1 = _mm_mul_ps(_mm_sub_ps(box_y1, _mm_load_ps(&data->origin[1][i])),
                             _mm_load_ps(&data->inverseDirection[1][i]));

      __m128 ymin = _mm_min_ps(y0,y1);
      __m128 ymax = _mm_max_ps(y0,y1);

      __m128 z0 = _mm_mul_ps(_mm_sub_ps(box_z0, _mm_load_ps(&data->origin[2][i])),
                             _mm_load_ps(&data->inverseDirection[2][i]));
      __m128 z1 = _mm_mul_ps(_mm_sub_ps(box_z1, _mm_load_ps(&data->origin[2][i])),
                             _mm_load_ps(&data->inverseDirection[2][i]));

      __m128 zmin = _mm_min_ps(z0,z1);
      __m128 zmax = _mm_max_ps(z0,z1);

      __m128 maximum_minimum = _mm_max_ps(xmin,_mm_max_ps(ymin,_mm_max_ps(zmin, _mm_set1_ps(T_EPSILON))));
      __m128 minimum_maximum = _mm_min_ps(xmax,_mm_min_ps(ymax,_mm_min_ps(zmax,_mm_load_ps(&data->minT[i]))));
      valid_intersect = and4(valid_intersect, _mm_cmple_ps(maximum_minimum,minimum_maximum));

      const int mask = _mm_movemask_ps(valid_intersect);

      if (mask) {
        if ((mask&0x8) && !rays.rayIsMasked(i+3))
          return i+3;
        else if ( (mask&0x4) && !rays.rayIsMasked(i+2))
          return i+2;
        else if ( (mask&0x2) && !rays.rayIsMasked(i+1))
          return i+1;
        else if ( (mask&0x1) && !rays.rayIsMasked(i))
          return i;
      }
    }

    for (; i > rayBegin;) {
      i--;
      //if (rays.rayIsMasked(i)) continue;
      if (anyHit && rays.wasHit(i)) continue;

      float tmin = T_EPSILON;
      float tmax = rays.getMinT(i);

      for (int c = 0; c < 3; c++) {
        float t0 = (box[0][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);
        float t1 = (box[1][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);

        float near = (t0 < t1) ? t0 : t1;
        float far  = (t0 < t1) ? t1 : t0;
        tmin = (tmin < near) ? near : tmin; // max of tmin, near
        tmax = (far <  tmax) ? far : tmax;  // min of tmax, far
      }
      if (tmin <= tmax) {  // valid intersection
        return i;
      }
    }
  }
#else
  for (int i = rayEnd - 1; i > rayBegin; i--) {
    //if (rays.rayIsMasked(i)) continue;
      if (anyHit && rays.wasHit(i)) continue;

    float tmin = T_EPSILON;
    float tmax = rays.getMinT(i);

    for (int c = 0; c < 3; c++) {
      float t0 = (box[0][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);
      float t1 = (box[1][c] - rays.getOrigin(i,c)) * rays.getInverseDirection(i,c);

      float near = (t0 < t1) ? t0 : t1;
      float far  = (t0 < t1) ? t1 : t0;
      tmin = (tmin < near) ? near : tmin; // max of tmin, near
      tmax = (far <  tmax) ? far : tmax;  // min of tmax, far
    }
    if (tmin <= tmax) {  // valid intersection
      return i;
    }
  }
#endif
  return rayBegin;
}

void DynBVH::setGroup(Group* new_group) {
  if (new_group != currGroup)
    group_changed = true;
  currGroup = new_group;
  mesh = dynamic_cast<Mesh*>(new_group);
}

Group* DynBVH::getGroup() const {
  return currGroup;
}

void DynBVH::groupDirty()
{
  group_changed = true;
}

void DynBVH::addToUpdateGraph(ObjectUpdateGraph* graph,
                              ObjectUpdateGraphNode* parent) {
  cerr << MANTA_FUNC << endl;
  ObjectUpdateGraphNode* node = graph->insert(this, parent);
  getGroup()->addToUpdateGraph(graph, node);
}

void DynBVH::preprocess(const PreprocessContext& context)
{
  //cerr << MANTA_FUNC << endl;
  // First preprocess the underlying group (so for example the
  // positions of underlying WaldTriangles are correct)
  if (currGroup) {
    currGroup->preprocess(context);

    // Call rebuild (may call update underneath)
    if (context.isInitialized()) {

      // We need to set largeSubtreeSize before calling rebuild in case rebuild
      // just calls update (loaded from file for instance).
      if (!group_changed) {
        if (context.proc == 0) {
          largeSubtreeSize = nodes.size() / (20 * context.numProcs);
#if TREE_ROT
          subtree_size.resize(nodes.size());
#endif
          computeSubTreeSizes(0);
        }
        context.done();
      }

      rebuild(context.proc, context.numProcs);

    //printNode(0, 0);
    }

    // NOTE(boulos): We allow rebuild to set the group_changed flag so
    // that you're not required to call preprocess in order to do an
    // update/rebuild.
  }

#ifdef COLLECT_STATS
  if (context.proc == 0 && context.isInitialized())
    context.manta_interface->registerParallelPreRenderCallback(
                                Callback::create(this, &DynBVH::updateStats));
#endif

}

typedef Callback_1Data_2Arg<DynBVH, Task*, int, UpdateContext> BVHUpdateTask;
typedef Callback_1Data_2Arg<DynBVH, TaskList*, int, Task*> BVHUpdateReduction;

typedef Callback_1Data_3Arg<DynBVH, Task*, int, int, UpdateContext> BVHPreprocessTask;

typedef Callback_1Data_4Arg<DynBVH, Task*, int, int, int, UpdateContext> BVHBuildTask;
typedef Callback_1Data_2Arg<DynBVH, TaskList*, int, Task*> BVHBuildReduction;
typedef Callback_1Data_5Arg<DynBVH, TaskList*, Task*, int, int, int, UpdateContext> BVHSAHReduction;


void DynBVH::beginParallelPreprocess(UpdateContext context) {
  cerr << "Computing bounds for all primitives (" << currGroup->size() << ")" << endl;
  // Allocate all the necessary spots first
  allocate();
  const unsigned int kNumPrimsPerTask = 1024;
  unsigned int num_tasks = std::max(size_t(1), currGroup->size() / kNumPrimsPerTask);
  if (TaskListMemory) delete[] TaskListMemory;
  TaskListMemory = new char[1 * sizeof(TaskList)];
  CurTaskList = 0;
  if (TaskMemory) delete[] TaskMemory;
  TaskMemory = new char[num_tasks * sizeof(Task)];
  CurTask = 0;
  TaskList* new_list = new (TaskListMemory) TaskList();

  if (FourArgCallbackMemory) delete[] FourArgCallbackMemory;
  FourArgCallbackMemory = new char[num_tasks * sizeof(BVHPreprocessTask)];
  CurFourArgCallback = 0;

  for (unsigned int i = 0; i < num_tasks; i++) {
    int begin = i * kNumPrimsPerTask;
    int end   = begin + kNumPrimsPerTask;
    if (i == num_tasks - 1) {
      end = currGroup->size();
    }
    Task* child = new (TaskMemory + sizeof(Task) * i) Task
      (new (FourArgCallbackMemory + i*sizeof(BVHPreprocessTask))BVHPreprocessTask
       (this,
        &DynBVH::parallelPreprocess,
        begin,
        end,
        context));
    new_list->push_back(child);
  }
  new_list->setReduction(
    Callback::create(this,
                     &DynBVH::finishParallelPreprocess,
                     context));

  // Ask the work queue to update my BVH
  context.insertWork(new_list);
}

void DynBVH::parallelPreprocess(Task* task, int objectBegin, int objectEnd, UpdateContext context) {
  PreprocessContext preprocess_context(context.manta_interface,
                                       context.proc,
                                       context.num_procs,
                                       NULL);
  BBox overall_bounds;
  for ( int i = objectBegin; i < objectEnd; i++ ) {
    object_ids[i] = i;
    currGroup->get(i)->computeBounds(preprocess_context, obj_bounds[i]);
    obj_centroids[i] = obj_bounds[i].center();
    overall_bounds.extendByBox(obj_bounds[i]);
  }
  BBox* output = (BBox*)task->scratchpad_data;
  *output = overall_bounds;
  task->finished();
}

void DynBVH::finishParallelPreprocess(TaskList* tasklist, UpdateContext context) {
  // reduce the individual bounds into one bound and set node 0
  TaskList& list = *tasklist;
  BBox overall_bounds;
  for (size_t i = 0; i < list.tasks.size(); i++) {
    BBox* task_bounds = (BBox*)list.tasks[i]->scratchpad_data;
    overall_bounds.extendByBox(*task_bounds);
  }
  nodes[0].bounds = overall_bounds;
  beginParallelBuild(context);
}

void DynBVH::beginParallelBuild(UpdateContext context) {
  cerr << "Doing parallel BVH build (for " << currGroup->size() << " primitives)" << endl;
  num_nodes.set(1);
  nextFree.set(1);

  int num_possible_nodes = (2*currGroup->size()) + 1;
  if (TaskListMemory) delete[] TaskListMemory;
  TaskListMemory = new char[(num_possible_nodes) * sizeof(TaskList)];
  CurTaskList = 0;
  if (TaskMemory) delete[] TaskMemory;
  TaskMemory = new char[2 * (num_possible_nodes) * sizeof(Task)];
  CurTask = 0;

  if (FourArgCallbackMemory) delete[] FourArgCallbackMemory;
  FourArgCallbackMemory = new char[3 * (num_possible_nodes) * sizeof(BVHBuildTask)];
  CurFourArgCallback = 0;

  if (FiveArgCallbackMemory) delete[] FiveArgCallbackMemory;
  FiveArgCallbackMemory = new char[3 * (num_possible_nodes) * sizeof(BVHSAHReduction)];

  TaskList* new_list = new (TaskListMemory + sizeof(TaskList)*CurTaskList) TaskList();

  Task* build_tree =
    new (TaskMemory + sizeof(Task) * CurTask) Task(
      new (FourArgCallbackMemory) BVHBuildTask
        (this,
         &DynBVH::parallelTopDownBuild,
         0,
         0,
         currGroup->size(),
         context));

  CurTask++;
  CurTaskList++;
  CurFourArgCallback++;

  new_list->push_back(build_tree);
  //I (thiago) commented this out since I removed the parallel update code.
  //Not running this might break things!  Of course, this parallel build
  //already doesn't really work...
//   new_list->setReduction(
//     Callback::create(this,
//                      &DynBVH::beginParallelUpdate,
//                      context));

  // Ask the work queue to update my BVH
  context.insertWork(new_list);
}

void DynBVH::splitBuild(Task* task, int nodeID, int objectBegin, int objectEnd, UpdateContext context) {
  int* vals = (int*)task->scratchpad_data;
  int bestAxis = vals[0];
  int split = vals[1];
  BVHNode& node = nodes[nodeID];

  if (bestAxis == -1) {
    // make leaf
    node.makeLeaf(objectBegin, objectEnd-objectBegin);

    std::sort(object_ids.begin()+objectBegin,object_ids.begin()+objectEnd);

    node.bounds.reset();
    for (int i = objectBegin; i < objectEnd; i++) {
      node.bounds.extendByBox(obj_bounds[object_ids[i]]);
    }

    num_nodes++;
    task->finished();
  } else {
    // make internal node
    // allocate two spots
    int my_spot = (nextFree += 2);
    node.makeInternal(my_spot, bestAxis);
    num_nodes++;

    // Make two subtasks to build the children
    int left_son = node.child;
    int right_son = left_son + 1;

    // Recurse into two subtrees
    taskpool_mutex.lock();

    size_t tasklist_id = CurTaskList;
    size_t left_task_id = CurTask;
    size_t right_task_id = CurTask + 1;
    size_t callback_id = CurFourArgCallback;

    CurTaskList++;
    CurTask += 2;
    CurFourArgCallback += 3;

    taskpool_mutex.unlock();

    TaskList* children = new (TaskListMemory + sizeof(TaskList) * tasklist_id) TaskList();
    Task* left_child = new (TaskMemory + sizeof(Task) * left_task_id) Task
      (
       new (FourArgCallbackMemory + callback_id * sizeof(BVHBuildTask) )BVHBuildTask
       (this,
        &DynBVH::parallelTopDownBuild,
        left_son,
        objectBegin,
        split,
        context));

    children->push_back(left_child);

    Task* right_child = new (TaskMemory + sizeof(Task) * right_task_id) Task
      (
       new (FourArgCallbackMemory + (callback_id+1)*sizeof(BVHBuildTask))BVHBuildTask
       (this,
        &DynBVH::parallelTopDownBuild,
        right_son,
        split,
        objectEnd,
        context));

    children->push_back(right_child);
    // Insert new TaskList for the BVH Object* in UpdateGraph

    // When the children finish, call reduction
    children->setReduction(
                           new (FourArgCallbackMemory + (callback_id+2) *sizeof(BVHBuildTask))BVHBuildReduction
                           (this,
                            &DynBVH::parallelBuildReduction,
                            nodeID,
                            task));

    context.insertWork(children);
  }
}

// We know that there is "lots of work" (objectEnd - objectBegin is
// sort of big), so no need to check for small cases here
void DynBVH::parallelApproximateSAH(Task* task, int nodeID, int objectBegin, int objectEnd, UpdateContext context) {
  // First kick off a parallel bounds computation
  const int kBinningObjects = 1024;
  int num_objects = objectEnd - objectBegin;
  int num_tasks = std::max(1, num_objects / kBinningObjects);

  taskpool_mutex.lock();
  size_t tasklist_id = CurTaskList;
  size_t first_task = CurTask;
  size_t callback_id = CurFourArgCallback;
  size_t reduction_id = CurFiveArgCallback;

  CurTaskList++;
  CurTask += num_tasks;
  CurFourArgCallback += num_tasks;
  CurFiveArgCallback++;
  taskpool_mutex.unlock();

  TaskList* children = new (TaskListMemory + sizeof(TaskList) * tasklist_id) TaskList();
  for (int i = 0; i < num_tasks; i++) {
    int begin = objectBegin + i * kBinningObjects;
    int end   = begin + kBinningObjects;
    if (i == num_tasks - 1) {
      end = objectEnd;
    }
    Task* child = new (TaskMemory + sizeof(Task) * (first_task + i)) Task
      (new (FourArgCallbackMemory + (callback_id+i)*sizeof(BVHBuildTask))BVHBuildTask
       (this,
        &DynBVH::parallelComputeBounds,
        nodeID,
        begin,
        end,
        context));
    children->push_back(child);
  }

  children->setReduction(new (FiveArgCallbackMemory + reduction_id * sizeof(BVHSAHReduction))BVHSAHReduction
                         (this,
                          &DynBVH::parallelComputeBoundsReduction,
                          task,
                          nodeID,
                          objectBegin,
                          objectEnd,
                          context));

  context.insertWork(children);
  // Then compute bin entries in parallel (sweep during reduction?)

  // If a split has been deemed necessary (very likely), segment the object ids in parallel
}

void DynBVH::parallelComputeBounds(Task* task, int nodeID, int objectBegin, int objectEnd, UpdateContext context) {
  BBox exact_bounds;
  BBox centroid_bounds;
  for (int i = objectBegin; i < objectEnd; i++) {
    exact_bounds.extendByBox(obj_bounds[object_ids[i]]);
    centroid_bounds.extendByPoint(obj_centroids[object_ids[i]]);
  }
  BBox* boxes = (BBox*)task->scratchpad_data;
  boxes[0] = exact_bounds;
  boxes[1] = centroid_bounds;

  //cerr << MANTA_FUNC << " NodeID " << nodeID << " has bounds " << result << endl;
  task->finished();
}

void DynBVH::parallelComputeBoundsReduction(TaskList* tasklist,
                                            Task* task,
                                            int nodeID,
                                            int objectBegin,
                                            int objectEnd,
                                            UpdateContext context) {
  // Merge the bounds
  TaskList& list = *tasklist;
  BBox overall_bounds;
  BBox centroid_bounds;
  for (size_t i = 0; i < list.tasks.size(); i++) {
    BBox* task_bounds = (BBox*)list.tasks[i]->scratchpad_data;
    overall_bounds.extendByBox(task_bounds[0]);
    centroid_bounds.extendByBox(task_bounds[1]);
  }
  //cerr << MANTA_FUNC << " NodeID " << nodeID << " has bounds " << overall_bounds << endl;

  // Now kick off a parallel bin computation
  const int kBinningObjects = 1024;
  int num_objects = objectEnd - objectBegin;
  int num_tasks = std::max(1, num_objects / kBinningObjects);

  taskpool_mutex.lock();
  size_t tasklist_id = CurTaskList;
  size_t first_task = CurTask;
  size_t callback_id = CurFourArgCallback;
  size_t reduction_id = CurFiveArgCallback;

  CurTaskList++;
  CurTask += num_tasks;
  CurFourArgCallback += num_tasks;
  CurFiveArgCallback++;
  taskpool_mutex.unlock();

  TaskList* children = new (TaskListMemory + sizeof(TaskList) * tasklist_id) TaskList();
  for (int i = 0; i < num_tasks; i++) {
    int begin = objectBegin + i * kBinningObjects;
    int end   = begin + kBinningObjects;
    if (i == num_tasks - 1) {
      end = objectEnd;
    }
    Task* child = new (TaskMemory + sizeof(Task) * (first_task + i)) Task
      (new (FourArgCallbackMemory + (callback_id+i)*sizeof(BVHBuildTask))BVHBuildTask
       (this,
        &DynBVH::parallelComputeBins,
        nodeID,
        begin,
        end,
        context));
    BBox* task_bounds = (BBox*)child->scratchpad_data;
    task_bounds[0] = overall_bounds;
    task_bounds[1] = centroid_bounds;
    children->push_back(child);
  }

  children->setReduction(new (FiveArgCallbackMemory + reduction_id * sizeof(BVHSAHReduction))BVHSAHReduction
                         (this,
                          &DynBVH::parallelComputeBinsReduction,
                          task,
                          nodeID,
                          objectBegin,
                          objectEnd,
                          context));

  context.insertWork(children);
}

void DynBVH::parallelComputeBins(Task* task, int nodeID, int objectBegin, int objectEnd, UpdateContext context) {
  // The overall bounds is in the scratch pad
  BBox* boxes = (BBox*)(task->scratchpad_data);
  // NOTE(boulos): Interestingly the binning only needs the centroid
  //bounds while the cost determination needs the overall bounds...
  //BBox& overall_bounds = boxes[0];
  BBox& centroid_bounds = boxes[1];
  //cerr << MANTA_FUNC << " NodeID " << nodeID << " has bounds " << overall_bounds << endl;
  struct SampleBin {
    SampleBin() { count = 0; }
    SampleBin(const SampleBin& copy) : bounds(copy.bounds), count(copy.count) {
    }
    BBox bounds;
    int count;
  };

  const int num_samples = BVH_num_samples;
  SampleBin bins[3][num_samples];

  Vector min_point = centroid_bounds.getMin();
  Vector max_point = centroid_bounds.getMax();
  Vector width = max_point - min_point;
  Vector scale((num_samples/width[0]) * .999f,
               (num_samples/width[1]) * .999f,
               (num_samples/width[2]) * .999f);

  for (int i = objectBegin; i < objectEnd; i++) {
    BBox& obj_box = obj_bounds[object_ids[i]];
    Vector& obj_centroid = obj_centroids[object_ids[i]];
    for (int axis = 0; axis < 3; axis++) {
      // Sample bin is where this position would fall to the left
      int which_bin = static_cast<int>((obj_centroid[axis] - min_point[axis]) * scale[axis]);
      which_bin = Clamp(which_bin, 0, num_samples-1);
      bins[axis][which_bin].count++;
      bins[axis][which_bin].bounds.extendByBox(obj_box);
    }
  }

  SampleBin* output = (SampleBin*)task->scratchpad_data;
  for (int i = 0; i < 3; i++) {
    for (int s = 0; s < num_samples; s++) {
      *output++ = bins[i][s];
    }
  }
  BBox* centroid_output = (BBox*)output;
  *centroid_output = centroid_bounds;

  task->finished();
}

int DynBVH::partitionObjects(int objBegin, int objEnd, int axis, float position) const {
  //cerr << MANTA_FUNC << " begin = " << objBegin << ", end = " << objEnd << endl;
  int first = objBegin;
  int last  = objEnd;
  --first;
  while (1) {
    for (++first; first < last; first++) {
      if (obj_centroids[object_ids[first]][axis] >= position)
        break;
    }

    for(--last; first < last; --last) {
      if (obj_centroids[object_ids[last]][axis] < position)
        break;
    }

    if (first < last) {
      // Swap first and last
      int temp = object_ids[first];
      object_ids[first] = object_ids[last];
      object_ids[last]  = temp;
    } else {
      return first;
    }
  }
  // To quiet warnings
  return -1;
}

void DynBVH::parallelComputeBinsReduction(TaskList* tasklist,
                                          Task* task,
                                          int nodeID,
                                          int objectBegin,
                                          int objectEnd,
                                          UpdateContext context) {
  const int num_samples = BVH_num_samples;
  struct SampleBin {
    SampleBin() { count = 0; }
    BBox bounds;
    int count;
  };
  SampleBin bins[3][num_samples];
  int num_objects = objectEnd - objectBegin;

  // Merge the bins
  TaskList& list = *tasklist;
  BBox overall_bounds;
  BBox centroid_bounds;
  for (size_t i = 0; i < list.tasks.size(); i++) {
    SampleBin* task_bins = (SampleBin*)list.tasks[i]->scratchpad_data;
    for (int axis = 0; axis < 3; axis++) {
      for (int sample = 0; sample < num_samples; sample++) {
        SampleBin& task_bin = *(task_bins + axis * num_samples + sample);
        bins[axis][sample].count += task_bin.count;
        bins[axis][sample].bounds.extendByBox(task_bin.bounds);
        overall_bounds.extendByBox(task_bin.bounds);
      }
    }
    if (i == 0) {
      // NOTE(boulos): Every task has the centroid bounds immediately following the SampleBins
      BBox* box = (BBox*)(task_bins + 1);
      centroid_bounds = *box;
    }
  }


  Vector min_point = centroid_bounds.getMin();
  Vector max_point = centroid_bounds.getMax();
  Vector width = max_point - min_point;
  // Search for best cost
  BVHCostEval best_cost;
  best_cost.cost = num_objects * overall_bounds.computeArea();
  best_cost.axis = -1;
  best_cost.position = FLT_MAX;
  best_cost.event = -1;

  for (int axis = 0; axis < 3; axis++) {
    float left_areas[num_samples];
    float right_areas[num_samples];
    int left_counts[num_samples];
    int right_counts[num_samples];
    BBox left_box, right_box;
    int num_left = 0;
    int num_right = num_objects;

    // Sweep from left to right, aggregating area and counts
    for (int i = 0; i < num_samples; i++) {
      num_left += bins[axis][i].count;
      num_right -= bins[axis][i].count;
      left_box.extendByBox(bins[axis][i].bounds);
      left_counts[i] = num_left;
      right_counts[i] = num_right;
      left_areas[i]  = left_box.computeArea();
    }

    // Sweep from right to left, aggregating areas
    for (int i = num_samples - 1; i >= 0; i--) {
      if (i == num_samples - 1)
        right_areas[i] = FLT_MAX;
      else
        right_areas[i] = right_box.computeArea();

      right_box.extendByBox(bins[axis][i].bounds);
    }

    // NOTE(boulos): The last bin is pointless (since it indicates leaf)
    for (int i = 0; i < num_samples - 1; i++) {
      float cost = (left_areas[i] * left_counts[i] +
                    right_areas[i] * right_counts[i]);
      if (cost < best_cost.cost) {
        // Found new best cost
        best_cost.cost = cost;
        best_cost.axis = axis;
        // NOTE(boulos): The position we want is the end of the bin, not the start
        float sample_position =
          (min_point[axis] + width[axis]*(static_cast<float>(i+1)/num_samples));
        best_cost.position = sample_position;
        best_cost.num_left = left_counts[i];
        best_cost.num_right = right_counts[i];
        best_cost.event = -1; // Unused
      }
    }
  }

  int* vals = (int*)task->scratchpad_data;
  vals[0] = best_cost.axis;
  vals[1] = 0;
  if (best_cost.axis == -1) {
    // no need for parallel split
    splitBuild(task, nodeID, objectBegin, objectEnd, context);
    return;
  }

  // Otherwise, we need to kick off a parallel partition of the object
  // ids based on the axis and position

  // TODO(boulos): Actually do this in parallel

  // write out object ids [objBegin,objEnd) in appropriate order
  int middle = partitionObjects(objectBegin, objectEnd, best_cost.axis, best_cost.position);
  if (middle == objectBegin || middle == objectEnd) {
    // Splitting didn't find a valid split, split in the middle unless
    // we have too few
    const int kMinObjects = 3;
    if (num_objects < kMinObjects) {
      vals[0] = -1;
    } else {
      middle = objectBegin + num_objects / 2;
    }
  }
  vals[1] = middle;
  splitBuild(task, nodeID, objectBegin, objectEnd, context);
}

void DynBVH::parallelTopDownBuild(Task* task, int nodeID, int objectBegin, int objectEnd, UpdateContext context) {
  //cerr << MANTA_FUNC << "(nodeId = " << nodeID << ", objectBegin = " << objectBegin << ", objectEnd = " << objectEnd << ")\n";
  int num_objects = objectEnd - objectBegin;
  const int kSmallObjects = 4096;
  PreprocessContext preprocess_context(context.manta_interface,
                                       context.proc,
                                       context.num_procs,
                                       NULL);
  if (num_objects < kSmallObjects) {
    // Just have this thread do the rest of the build
    build(nodeID, objectBegin, objectEnd);
    task->finished();
  } else {

    if (objectEnd <= objectBegin) {
      throw InternalError("Tried building BVH over invalid range");
    }

    //num_prims_beneath[nodeID] = num_objects;

    // We know that there are more than kSmallObjects, but how many
    // are necessary for parallel sorting?
    const int kParallelSortThreshold = kSmallObjects * 2;

    if (num_objects > kParallelSortThreshold) {
      // do split in parallel
      parallelApproximateSAH(task, nodeID, objectBegin, objectEnd, context);
    } else {
      int bestAxis = -1;
      int split = -1;
      // TODO (thiago): Finish porting my partition* methods to here or simply
      // get rid of parallelTopDownBuild.  Clearly commenting out these
      // functions breaks this build...
#if USE_APPROXIMATE_BUILD
      //split = partitionApproxSAH(preprocess_context, nodeID, objectBegin, objectEnd, bestAxis);
#else
      //split = partitionSAH(preprocess_context, nodeID, objectBegin, objectEnd, bestAxis);
#endif

      int* vals = (int*)task->scratchpad_data;
      vals[0] = bestAxis;
      vals[1] = split;
      splitBuild(task, nodeID, objectBegin, objectEnd, context);
    }
  }
}

void DynBVH::parallelBuildReduction(TaskList* list, int node_id, Task* task) {
  task->finished();
}

void DynBVH::update(int proc, int numProcs) {
  PreprocessContext context;
  parallelUpdateBounds(context, proc, numProcs);
  // TODO(boulos): Wait until everyone has gone through update to
  // disable group_changed (requires another barrier)
  if (proc == 0)
    group_changed = false;
}

void DynBVH::computeTraversalCost()
{
  double start = Time::currentSeconds();
  float cost = computeSubTreeTraversalCost(0)[0];
  double end = Time::currentSeconds();
  if (print_info) {
    cout << "BVH tree cost: " << cost << " computed in "
         <<(end-start)*1000<<"ms"<<endl;
  }
}

VectorT<float, 2> DynBVH::computeSubTreeTraversalCost(unsigned int nodeID)
{
  const float ISEC_COST = 3;
  const float TRAV_COST = 1;

  BVHNode &node = nodes[nodeID];
  if (node.isLeaf()) {
    float immediateCost = node.children * ISEC_COST + TRAV_COST;
    float futureCost = 0;
    for (int i=0; i < node.children; ++i) {
      if (mesh) {
        const int object_id = object_ids[node.child+i];
        const MeshTriangle* tri = mesh->get(object_id);
        if (tri->getMaterial()->canAttenuateShadows()) {
          futureCost += 1.0/node.children;
//           futureCost = 1; // This seems to give the same result.
//           break;
        }
      }
    }
    return VectorT<float, 2> (immediateCost, futureCost);
  }
  else { // internal node

#if 1
    // Use true probs.
    float Plr;
    float Pjl;
    float Pjr;

    ttp.computeProbs(nodes[nodeID].bounds,
                     nodes[node.child+0].bounds,
                     nodes[node.child+1].bounds, Plr, Pjl, Pjr);

    const float Pl = Pjl+Plr;
    const float Pr = Pjr+Plr;
#else
    // Use the BSP cost hack.
    const float rootSA = nodes[nodeID].bounds.computeArea();
    const float inv_rootSA = 1.0f / rootSA;
    const float Pl = nodes[node.child+0].bounds.computeArea() * inv_rootSA;
    const float Pr = nodes[node.child+1].bounds.computeArea() * inv_rootSA;

    // These calculations only apply to kd-trees, but we use them anyway.
    const float Plr = (Pl + Pr) - 1;
    const float Pjl = Pl - Plr;
    const float Pjr = Pr - Plr;
#endif

    VectorT<float, 2> Cl = computeSubTreeTraversalCost(node.child+0);
    VectorT<float, 2> Cr = computeSubTreeTraversalCost(node.child+1);

    VectorT<float, 2> cost;

     // Handle the lr case where ray pierces both and a ray may enter one child
     // and then have to enter the other child if it only hit empty leaves
     VectorT<float, 2> bothCl(Cl[0], 0);
     VectorT<float, 2> bothCr(Cr[0], 0);
     bothCl += Cl[1]*Cr;
     bothCr += Cr[1]*Cl;

     // Note: bothCl[1] == bothCr[1] == Cl[1]*Cr[1] at this point.

     VectorT<float, 2> minBothC = Min(bothCl, bothCr);

     // The BVH specific high quality optimized metric
     const float Pe = 1-Pjr-Pjl-Plr;
#if 0
     // code used in the RTSAH paper
     const float lCost = Pl*Cl[0] + Pjr*(Cr[0]+TRAV_COST) + Plr*Cl[1]*(Cr[0]+TRAV_COST) + Pe*TRAV_COST + TRAV_COST;
     const float rCost = Pr*Cr[0] + Pjl*(Cl[0]+TRAV_COST) + Plr*Cr[1]*(Cl[0]+TRAV_COST) + Pe*TRAV_COST + TRAV_COST;
#else
     // This is the corrected formula (original RTSAH paper is wrong).
     // TODO: Verify it really is better before getting rid of the old code.
     const float lCost = Pl*Cl[0] + (Pjr + Plr*Cl[1])*Cr[0] + (1 + Pl*Cl[1]+(1-Pl))*TRAV_COST;
     const float rCost = Pr*Cr[0] + (Pjl + Plr*Cr[1])*Cl[0] + (1 + Pr*Cr[1]+(1-Pr))*TRAV_COST;
#endif
     cost[0] = Min(lCost, rCost);
     cost[1] = Pjl*Cl[1] + Pjr*Cr[1] + Plr*Cl[1]*Cr[1] + Pe;
     cost[1] = Clamp(cost[1], 0.0f, 1.0f);  // Clean up numerical precision issues.

     node.isLeftCheaper = lCost < rCost;
     return cost;
  }
}

void DynBVH::allocate() const {
 if(2*currGroup->size() > nodes.size()) {
    nodes.resize(2*currGroup->size());
#if TREE_ROT
    costs.resize(2*currGroup->size());
    subtree_size.resize(2*currGroup->size());
#endif
    object_ids.resize(currGroup->size());

#if USE_LAZY_BUILD
    build_records.resize(2*currGroup->size());
#endif
    // TODO(boulos): Free these after construction? or keep around
    // for rebuild?
    obj_bounds.resize(currGroup->size());
    obj_centroids.resize(currGroup->size());
  }
}

void DynBVH::rebuild(int proc, int numProcs)
{
  if (!group_changed) {
    update(proc, numProcs);
    return;
  }

#if USE_LAZY_BUILD == 0
  if (print_info && proc == 0)
    cerr << "\nDynBVH::preprocess START (" << currGroup->size() << " objects)\n";
#endif
  double startTime = Time::currentSeconds();

  if (proc == 0) {
    allocate();
    nodes[0].bounds.reset();
  }
  barrier.wait(numProcs);

  // Note: We can't change group_changed until all the threads have passed the
  // if (!group_changed) line up above.  So this has to occur after the barrier.
  if (proc == 0)
    group_changed = false;


  PreprocessContext context;
  PreprocessContext serial_context;
  context.proc = proc;
  context.numProcs = numProcs;

  computeBounds(context, nodes[0].bounds);


  size_t start = proc*currGroup->size()/numProcs;
  size_t end = (proc+1)*currGroup->size()/numProcs;
  for (size_t i=start; i < end; ++i) {
    object_ids[i] = i;
    obj_bounds[i].reset();
    currGroup->get(i)->computeBounds(serial_context, obj_bounds[i]);
    obj_centroids[i] = obj_bounds[i].center();
  }


#if USE_LAZY_BUILD
  {
    if (proc == 0) {
      build_records[0].objectBegin = 0;
      build_records[0].objectEnd   = currGroup->size();
    }

    size_t start = proc*nodes.size()/numProcs;
    size_t end = (proc+1)*nodes.size()/numProcs;
    for (size_t i=start; i < end; ++i) {
      nodes[i].setUninitialized();
    }

    return;
  }
#endif

  // Need to make sure all threads have computed their bounds and other data
  // before doing build.
  barrier.wait(numProcs);

  if (proc > 0) return;

  num_nodes.set(0);
  nextFree.set(1);

  double build_start = Time::currentSeconds();

  build(0, 0, currGroup->size());

#if !TREE_ROT
  nodes.resize(num_nodes);
#endif

  largeSubtreeSize = nodes.size() / (20 * numProcs);
#if TREE_ROT
  subtree_size.resize(nodes.size());
#endif
  computeSubTreeSizes(0);

  double endTime = Time::currentSeconds();
  if (print_info) {
    cerr << "\nDynBVH build time: Total ("<<endTime-startTime<<")\n"
         << "object_ids initialization ("<<build_start-startTime<<")\n"
         << "build ("<<endTime-build_start<<")\n"
         << "num_nodes = " << num_nodes << "\n"
         << "BBox = ("<<nodes[0].bounds.getMin()<<", "<<nodes[0].bounds.getMax()<<")\n\n";

  }

#ifdef RTSAH
  computeTraversalCost();
#endif

#if TREE_ROT
  computeCost<true>(0);
#endif

  if (proc == 0 && needToSaveFile)
    saveToFile(saveFileName);
}

void DynBVH::build(int nodeID, int objectBegin, int objectEnd,
                   const bool useApproximateBuild, const bool nodeAlreadyExists)
{
  //cerr << MANTA_FUNC << " begin = " << objectBegin << " , end = " << objectEnd << endl;
  if (objectEnd <= objectBegin) {
    throw InternalError("Tried building BVH over invalid range");
  }

  BVHNode& node = nodes[nodeID];

  PartitionData partition;
#if USE_APPROXIMATE_BUILD
  partition = partitionApproxSAH(nodeID, objectBegin, objectEnd);
#else
  if (useApproximateBuild)
    partition = partitionApproxSAH(nodeID, objectBegin, objectEnd);
  else
    partition = partitionSAH(nodeID, objectBegin, objectEnd);
#endif

  if (partition.axis == -1) {
    if (nodeAlreadyExists)
      return;
    // make leaf
    node.makeLeaf(objectBegin, objectEnd-objectBegin);

#if TREE_ROT
    // need to set the cost of the new leaf node since leaves aren't visited
    // during rotations where costs are normally updated
    costs[nodeID] = node.children * BVH_C_isec;
    subtree_size[nodeID] = 1;
#endif

    std::sort(object_ids.begin()+objectBegin,object_ids.begin()+objectEnd);
    num_nodes++;
  } else {
    // make internal node
    // allocate two spots for child nodes
    int my_spot = (nextFree += 2);
    node.makeInternal(my_spot, partition.axis);

    if (!nodeAlreadyExists)
      num_nodes++;

    nodes[node.child+0].bounds = partition.bounds_l;
    nodes[node.child+1].bounds = partition.bounds_r;

    build(node.child+0, objectBegin, partition.split, useApproximateBuild);
    build(node.child+1, partition.split, objectEnd, useApproximateBuild);
#if TREE_ROT
    subtree_size[nodeID] = 1 + subtree_size[node.child+0] + subtree_size[node.child+1];
#endif
  }
}

void DynBVH::lazyBuild(const RenderContext& context, const int nodeID) const
{
  BVHNode& node = nodes[nodeID];
  // Is the node already built?
  // Note: As long as 32Byte BVH nodes are aligned to 32B boundaries, then if
  // the node is seen as initialized, the rest of the node data will also be
  // valid since all the data is brought in one pass.  If the node is not
  // aligned, there is a very small chance that we could end up using
  // uninitialized BVH data in traversal.  This is, however, very unlikely to
  // happen.
  if (!node.isUninitialized()) return;

  // No, it's not, so let's try to build it.
  const unsigned int mutex_id = (nodeID%kNumLazyBuildMutexes);

  lazybuild_mutex[mutex_id].lock();

  // Now that we called lock, a memory fence has a occurred and we can check to
  // see if the node really is still uninitialized (before this, this thread
  // could have been using old node data).
  if (!node.isUninitialized()) {
    lazybuild_mutex[mutex_id].unlock();
    return;
  }

  // Check to see if another thread is already building the node.
  if (std::find(nodesBeingBuilt[mutex_id].v.begin(), nodesBeingBuilt[mutex_id].v.end(),
                nodeID) != nodesBeingBuilt[mutex_id].v.end()) {
    // Another thread is already building the node, so we just need to wait
    // until the node is built.
    while (node.isUninitialized()) {
      lazybuild_cond[mutex_id].wait(lazybuild_mutex[mutex_id]);
      // if thread gets awakened from a broadcast, need to check if it is
      // because some other thread finished building the node this thread
      // needed.
    }

    lazybuild_mutex[mutex_id].unlock();
    return;
  }
  else {
    nodesBeingBuilt[mutex_id].v.push_back(nodeID);
    lazybuild_mutex[mutex_id].unlock();
  }

  int objectBegin = build_records[nodeID].objectBegin;
  int objectEnd   = build_records[nodeID].objectEnd;

  if (nodeID == 0) {
    num_nodes.set(1);
    nextFree.set(1);
  }

  PartitionData partition;
#if USE_APPROXIMATE_BUILD
  partition = partitionApproxSAH(nodeID, objectBegin, objectEnd);
#else
  partition = partitionSAH(nodeID, objectBegin, objectEnd);
#endif

  if (partition.axis == -1) {
    // make leaf
    // Can't assign axis just yet or we could get a race condition.  So we
    // makeLeaf manually.
    //node.makeLeaf(objectBegin, objectEnd-objectBegin);
    node.child = objectBegin;
    node.children = objectEnd-objectBegin;
    nodes[nodeID].isLargeSubtree = false;

#if USE_APPROXIMATE_BUILD != 0
    // Sort object ids so that memory access is faster during tracing.  Since
    // approx build is about fast builds, let's only do this sort for high
    // quality builds.
    std::sort(object_ids.begin()+objectBegin,object_ids.begin()+objectEnd);
#endif
    num_nodes++;
  } else {
    // make internal node
    // allocate two spots
    int my_spot = (nextFree += 2);
    node.child = my_spot;//node.makeInternal(my_spot, 3);
    node.children = 0;
    num_nodes++;

    nodes[node.child+0].bounds = partition.bounds_l;
    nodes[node.child+1].bounds = partition.bounds_r;

    build_records[node.child+0].objectBegin = objectBegin;
    build_records[node.child+0].objectEnd   = partition.split;

    build_records[node.child+1].objectBegin = partition.split;
    build_records[node.child+1].objectEnd   = objectEnd;

    // guess subtree size will be max size.
    nodes[nodeID].isLargeSubtree = static_cast<unsigned int>(2*(objectEnd-objectBegin)) > largeSubtreeSize;
  }

  // mutex needed for erase, but also operates as a memory fence so we know the
  // node is ready.
  lazybuild_mutex[mutex_id].lock();

  node.axis = partition.axis == -1? 0 : partition.axis;
  vector<int>::iterator eraseLoc = std::find(nodesBeingBuilt[mutex_id].v.begin(),
                                             nodesBeingBuilt[mutex_id].v.end(),
                                             nodeID);
  *eraseLoc = nodesBeingBuilt[mutex_id].v.back();
  nodesBeingBuilt[mutex_id].v.pop_back();

  lazybuild_cond[mutex_id].conditionBroadcast();
  lazybuild_mutex[mutex_id].unlock();
}

unsigned int DynBVH::computeSubTreeSizes(size_t nodeID) {
  if (nodes[nodeID].isUninitialized())
    return 0;

  if ( nodes[nodeID].isLeaf() ) {
    nodes[nodeID].isLargeSubtree = false;
#if TREE_ROT
    subtree_size[nodeID] = 1;
#endif
    return 1;
  }
  else {
    const size_t leftID = nodes[nodeID].child;
    const size_t rightID = nodes[nodeID].child + 1;
    const unsigned int leftSize = computeSubTreeSizes(leftID);
    const unsigned int rightSize = computeSubTreeSizes(rightID);
    const unsigned int size = leftSize + rightSize + 1;
    nodes[nodeID].isLargeSubtree = size > largeSubtreeSize;

#if TREE_ROT
    subtree_size[nodeID] = size;
#endif

    return size;
  }
}

void DynBVH::seedSubtrees(const int nodeID)
{
  if (nodes[nodeID].isLargeSubtree) {
    const BVHNode& node = nodes[nodeID];
    seedSubtrees(node.child+0);
    seedSubtrees(node.child+1);
  }
  else {
    subtreeListMutex.lock();
    subtreeList.push_back(nodeID);
    subtreeListMutex.unlock();
  }
}

void DynBVH::parallelUpdateBounds(const PreprocessContext& context,
                                  int proc, int numProcs)
{
#if USE_LAZY_BUILD
  if (nodes[0].isUninitialized())
    return;
#endif

  // It is faster (about 17% in one measurement) to compute the bounds all at
  // once here instead of on demand in updateBounds method where memory
  // accesses are more random (two triangles might be far apart in memory).
  const size_t startObj = proc*currGroup->size()/numProcs;
  const size_t endObj = (proc+1)*currGroup->size()/numProcs;
  for (size_t i=startObj; i < endObj; ++i) {
    obj_bounds[i].reset();
    currGroup->get(i)->computeBounds(context, obj_bounds[i]);
#if TREE_ROT
    obj_centroids[i] = obj_bounds[i].center();
#endif
  }
  barrier.wait(numProcs);

  if (numProcs == 1) {
    updateBounds<false>(0);
  } else {

    if (proc == 0) {
      seedSubtrees(0);
      subtreeListFilled = true;
    }

    while(true) {
      subtreeListMutex.lock();
      if (subtreeList.empty()) {
        subtreeListMutex.unlock();
        if (subtreeListFilled)
          break;
        else
          continue;
      }

      const unsigned int nodeID = subtreeList.back();
      subtreeList.pop_back();
      subtreeListMutex.unlock();
      updateBounds<false>(nodeID);
    }

    barrier.wait(numProcs);

    if (proc == 0) {
      subtreeListFilled = false;
      updateBounds<true>(0);
    }
  }
}

template <bool bottomSubtreesDone>
void DynBVH::updateBounds(int ID)
{
  // TODO: If a recomputed RTSAH is desired after each update, then that should
  // be folded into this in order to minimize memory accesses.  Also, the
  // BSP approximation should probably be used since that is much faster.

  BVHNode& node = nodes[ID];

#if USE_LAZY_BUILD
  if (node.isUninitialized()) {
    const int objectBegin = build_records[ID].objectBegin;
    const int objectEnd   = build_records[ID].objectEnd;

    node.bounds.reset();
    for (int i = objectBegin; i < objectEnd; i++) {
      const int obj_id = object_ids[i];
      node.bounds.extendByBox(obj_bounds[obj_id]);
    }
    return;
  }
#endif

  if (node.isLeaf()) {
    node.bounds.reset();
    for (int i = 0; i < node.children; i++ ) {
      const int obj_id = object_ids[node.child + i];
      node.bounds.extendByBox(obj_bounds[obj_id]);
    }
    node.axis = 0;
  } else {
    int left_son = node.child;
    int right_son = left_son + 1;
    if (bottomSubtreesDone) {
      if (nodes[left_son].isLargeSubtree)
        updateBounds<bottomSubtreesDone>(left_son);
      if (nodes[right_son].isLargeSubtree)
        updateBounds<bottomSubtreesDone>(right_son);
    }
    else {
      updateBounds<bottomSubtreesDone>(left_son);
      updateBounds<bottomSubtreesDone>(right_son);
    }

    node.bounds.reset();
    const BVHNode& left_node = nodes[left_son];
    const BVHNode& right_node = nodes[right_son];

    node.bounds.extendByBox(left_node.bounds);
    node.bounds.extendByBox(right_node.bounds);

#if TREE_ROT
    rotateNode( ID );
#endif
  }
}

DynBVH::PartitionData DynBVH::partitionApproxSAH(int nodeID, int objBegin,
                                                   int objEnd) const {
  // TODO(thiago): check whether returning a structure instead of through an
  // argument is expensive.
  PartitionData partition;

  int num_objects = objEnd - objBegin;
  if ( num_objects == 1 ) {
    partition.axis = -1;
    partition.split = -1;
    return partition;
  }

  if (num_objects == 2) {
    return partition2Objs(nodeID, objBegin);
  }

  BBox centroid_bounds;
  for (int i = objBegin; i < objEnd; i++) {
    centroid_bounds.extendByPoint(obj_centroids[object_ids[i]]);
  }

  const float inv_parent_area = 1.0f / nodes[nodeID].bounds.computeArea();
  BVHCostEval best_cost;
  best_cost.cost = num_objects * BVH_C_isec;
  best_cost.axis = -1;
  best_cost.position = FLT_MAX;
  best_cost.event = -1;

  struct SampleBin {
    SampleBin() { count = 0;}
    BBox bounds;
    int count;
  };


  const int num_samples = BVH_num_samples;
  SampleBin bins[3][num_samples];

  Vector min_point = centroid_bounds.getMin();
  Vector max_point = centroid_bounds.getMax();
  Vector width = max_point - min_point;
  Vector scale((num_samples/width[0]) * .999f,
               (num_samples/width[1]) * .999f,
               (num_samples/width[2]) * .999f);
#ifdef LONGEST_AXIS
  int longest_axis = centroid_bounds.longestAxis();
#endif

  for (int i = objBegin; i < objEnd; i++) {
    BBox& obj_box = obj_bounds[object_ids[i]];
    Vector& obj_centroid = obj_centroids[object_ids[i]];
#ifdef LONGEST_AXIS
    for (int axis = longest_axis; axis == longest_axis; axis++) {
#else
    for (int axis = 0; axis < 3; axis++) {
#endif
      // Sample bin is where this position would fall to the left
      int which_bin = int((obj_centroid[axis] - min_point[axis]) * scale[axis]);
      which_bin = Clamp(which_bin, 0, num_samples - 1);
      bins[axis][which_bin].count++;
      bins[axis][which_bin].bounds.extendByBox(obj_box);
    }
  }

  // Now that we have all the sample points binned, we can just sweep over
  // them to search for the best cost
#ifdef LONGEST_AXIS
  for (int axis = longest_axis; axis == longest_axis; axis++) {
#else
  for (int axis = 0; axis < 3; axis++) {
#endif
    float left_areas[num_samples];
    float right_areas[num_samples];
    int left_counts[num_samples];
    int right_counts[num_samples];
    BBox left_box, right_box;
    int num_left = 0;
    int num_right = num_objects;

    // Sweep from left to right, aggregating area and counts
    for (int i = 0; i < num_samples; i++) {
      num_left += bins[axis][i].count;
      num_right -= bins[axis][i].count;
      left_box.extendByBox(bins[axis][i].bounds);
      left_counts[i] = num_left;
      right_counts[i] = num_right;
      left_areas[i]  = left_box.computeArea();
    }

    // Sweep from right to left, aggregating areas
    for (int i = num_samples - 1; i >= 0; i--) {
      if (i == num_samples - 1)
        right_areas[i] = FLT_MAX;
      else
        right_areas[i] = right_box.computeArea();

      right_box.extendByBox(bins[axis][i].bounds);
    }

    // NOTE(boulos): The last bin is pointless (since it indicates leaf)
    for (int i = 0; i < num_samples - 1; i++) {
      float cost = (left_areas[i] * left_counts[i] +
                    right_areas[i] * right_counts[i])*inv_parent_area * BVH_C_isec +
                    BVH_C_trav;
      if (cost < best_cost.cost) {
        // Found new best cost
        best_cost.cost = cost;
        best_cost.axis = axis;
        // NOTE(boulos): The position we want is the end of the bin, not the start
        float sample_position =
          (min_point[axis] + width[axis]*(static_cast<float>(i+1)/num_samples));
        best_cost.position = sample_position;
        best_cost.num_left = left_counts[i];
        best_cost.num_right = right_counts[i];
        best_cost.event = -1; // Unused
      }
    }
  }

  partition.axis = best_cost.axis;
  if ( partition.axis != -1 ) {
    // write out object ids [objBegin,objEnd) in appropriate order
    int middle = partitionObjects(objBegin, objEnd, partition.axis, best_cost.position);
    if (middle == objBegin || middle == objEnd) {
      // Splitting didn't find a valid split, split in the middle unless
      // we have too few
      const int kMinObjects = 3;
      if (num_objects < kMinObjects) {
        partition.axis = -1;
        partition.split = 0;
        return partition;
      }
      middle = objBegin + num_objects / 2;
    }

    for (int i = objBegin; i < middle; ++i) {
      partition.bounds_l.extendByBox(obj_bounds[object_ids[i]]);
    }
    for (int i = middle; i < objEnd; ++i) {
      partition.bounds_r.extendByBox(obj_bounds[object_ids[i]]);
    }

    partition.split = middle;
    return partition;
  }

  // making a leaf
  partition.split = 0;
  return partition;
}

  DynBVH::PartitionData DynBVH::partition2Objs(int nodeID, int objBegin) const {
    PartitionData partition;
    const Real nodeArea = nodes[ nodeID ].bounds.computeArea();
    const Real leftArea = obj_bounds[object_ids[objBegin]].computeArea();
    const Real rightArea = obj_bounds[object_ids[objBegin+1]].computeArea();
    const Real leafCost = BVH_C_isec * 2;

    const Real newCost = BVH_C_trav +
      (leftArea * BVH_C_isec + rightArea * BVH_C_isec )/nodeArea;

    if (newCost < leafCost) {
      partition.axis = nodes[nodeID].bounds.diagonal().indexOfMaxComponent();
      partition.split = objBegin+1;

      // For traversal performance reasons, we want the "left" side to be the
      // closer side (for ray going in positive direction), so swap if they are
      // reversed.
      if (obj_bounds[object_ids[objBegin+0]][0][partition.axis] >
          obj_bounds[object_ids[objBegin+1]][0][partition.axis])
        swap(object_ids[objBegin+0], object_ids[objBegin+1]);

      // perform a split!
      partition.bounds_l = obj_bounds[object_ids[objBegin+0]];
      partition.bounds_r = obj_bounds[object_ids[objBegin+1]];
      // use the max dimension as a heuristic for where the split should go.
      return partition;
    }
    else {
      partition.axis = -1;
      return partition;
    }
  }

  DynBVH::PartitionData DynBVH::partitionSAH(int nodeID,
                                             int objBegin, int objEnd) const
{
  PartitionData partition;

  int num_objects = objEnd - objBegin;
  if ( num_objects == 1 ) {
      partition.axis = -1;
      return partition;
  }

  BVHCostEval best_cost;
  best_cost.cost = BVH_C_isec * num_objects;
  best_cost.axis = -1;
  best_cost.position = FLT_MAX;
  best_cost.event = -1;

  // Try optimized special case for 2 objects
  if (num_objects == 2) {
    return partition2Objs(nodeID, objBegin);
  }

  for ( int axis = 0; axis < 3; axis++ ) {
    BVHCostEval new_cost;
    if ( buildEvents(nodeID, objBegin,objEnd,axis,new_cost) ) {
      if ( new_cost.cost < best_cost.cost ) {
        best_cost = new_cost;
      }
    }
  }

  partition.bounds_r = best_cost.bounds_r;
  partition.axis = best_cost.axis;
  if ( partition.axis != -1 ) {
    // write out object ids [objBegin,objEnd) in appropriate order
    // NOTE(boulos): Because the way the sweep works, we can't just
    // use the position and partitionObjects
    std::vector<BVHSAHEvent> events;
    events.reserve(objEnd-objBegin);
    for ( int i = objBegin; i < objEnd; i++ ) {
      BVHSAHEvent new_event;
      new_event.position = obj_centroids[object_ids[i]][partition.axis];
      new_event.obj_id   = object_ids[i];
      events.push_back(new_event);
    }

    partition.split = objBegin + best_cost.event;

    std::sort(events.begin(),events.end(),CompareBVHSAHEvent());
    for (int i = objBegin; i < objEnd; i++) {
      object_ids[i] = events[i-objBegin].obj_id;

      if (i < partition.split)
        partition.bounds_l.extendByBox(obj_bounds[object_ids[i]]);
    }

    return partition;
  }

  return partition; // making a leaf anyway
}

bool DynBVH::buildEvents(const int parentID,
                         int first,
                         int last,
                         int axis,
                         BVHCostEval& best_eval) const
{
  std::vector<BVHSAHEvent> events;
  events.reserve(last-first);
  for ( int i = first; i < last; i++ ) {
    BVHSAHEvent new_event;
    new_event.position = obj_centroids[object_ids[i]][axis];
    new_event.obj_id   = object_ids[i];
    events.push_back(new_event);
  }

  std::sort(events.begin(),events.end(),CompareBVHSAHEvent());

  int num_events = int(events.size());
  BBox left_box;

  int num_left = 0;
  int num_right = num_events;

  for ( size_t i = 0; i < events.size(); i++ ) {
    events[i].num_left = num_left;
    events[i].num_right = num_right;
    events[i].left_area = left_box.computeArea();

    BBox& tri_box = obj_bounds[events[i].obj_id];
    left_box.extendByBox(tri_box);

    num_left++;
    num_right--;
  }

  BBox right_box;

  best_eval.cost = FLT_MAX;
  best_eval.event = -1;

  const float inv_parent_area = 1.0f / nodes[parentID].bounds.computeArea();

  for ( int i = num_events - 1; i >= 0; i-- ) {
    BBox& tri_box = obj_bounds[events[i].obj_id];
    right_box.extendByBox(tri_box);

    if ( events[i].num_left > 0 && events[i].num_right > 0 ) {
      events[i].right_area = right_box.computeArea();

      float this_cost = (events[i].num_left * events[i].left_area +
                         events[i].num_right * events[i].right_area);
      this_cost *= inv_parent_area;
      this_cost *= BVH_C_isec;
      this_cost += BVH_C_trav;

      events[i].cost = this_cost;
      if ( this_cost < best_eval.cost ) {
        best_eval.bounds_r   = right_box;
        best_eval.cost       = this_cost;
        best_eval.position   = events[i].position;
        best_eval.axis       = axis;
        best_eval.event      = i;
        best_eval.num_left   = events[i].num_left;
        best_eval.num_right  = events[i].num_right;
      }
    }
  }
  return best_eval.event != -1;
}

bool DynBVH::buildFromFile(const string &file)
{
  nodes.clear();

  //Something's wrong, let's bail.
  if (!currGroup)
    return false;

  ifstream in(file.c_str(), ios::binary | ios::in);
  if (!in)
    return false;

  group_changed = false;

  unsigned int object_ids_size;
  in.read((char*)&object_ids_size, sizeof(object_ids_size));
  object_ids.resize(object_ids_size);

  in.read((char*) &object_ids[0], sizeof(object_ids[0])*object_ids.size());

  unsigned int nodes_size;
  in.read((char*)&nodes_size, sizeof(nodes_size));
  nodes.resize(nodes_size);

  in.read((char*) &nodes[0], sizeof(nodes[0])*nodes.size());
  num_nodes.set(nodes.size());

  in.close();

#ifdef RTSAH
  computeTraversalCost();
#endif

  return true;
}


bool DynBVH::saveToFile(const string &file)
{
  if (nodes.empty()) {
    needToSaveFile = true;
    saveFileName = file;
    return true; // We support it, so we return true now.  Hopefully it'll end
                 // up working when this actually occurs...
  }
  needToSaveFile = false;

  ofstream out(file.c_str(), ios::out | ios::binary);
  if (!out) return false;

  const unsigned int object_ids_size = object_ids.size();
  const unsigned int nodes_size = nodes.size();
  out.write((char*) &object_ids_size, sizeof(object_ids_size));
  out.write((char*) &object_ids[0], sizeof(object_ids[0])*object_ids_size);
  out.write((char*) &nodes_size, sizeof(nodes_size));
  out.write((char*) &nodes[0], sizeof(nodes[0])*nodes_size);

  out.close();
  return true;
}


// begin port from actual DynBVH
#ifdef MANTA_SSE
void DynBVH::templatedTraverse(const RenderContext& context, RayPacket& packet) const {
  struct StackNode {
    int nodeID;
    int firstActive;
  };

  StackNode nodeStack[64];// ={}; // NOTE(boulos): no need to 0 out
  int stackPtr = 0;

  int ID = 0;
  int firstActive = 0;

  const int frontSon[3] = {
    packet.getDirection(0, 0) > 0 ? 0 : 1,
    packet.getDirection(0, 1) > 0 ? 0 : 1,
    packet.getDirection(0, 2) > 0 ? 0 : 1
  };

  const BVHNode* const nodes = &this->nodes[0];
  __m128 min_rcp[3];
  __m128 max_rcp[3];

  RayPacketData* data = packet.data;
  MANTA_UNROLL(3);
  for (int d = 0; d < 3; d++) {
    min_rcp[d] = _mm_load_ps(&data->inverseDirection[d][0]);
    max_rcp[d] = _mm_load_ps(&data->inverseDirection[d][0]);

    MANTA_UNROLL(8);
    for (int pack = 1; pack < RayPacket::SSE_MaxSize; pack++) {
      min_rcp[d] = _mm_min_ps(min_rcp[d],
                              _mm_load_ps(&data->inverseDirection[d][pack * 4]));
      max_rcp[d] = _mm_max_ps(max_rcp[d],
                              _mm_load_ps(&data->inverseDirection[d][pack * 4]));
    }
  }

  // do sign check

  int signChecked =
    ((_mm_movemask_ps(_mm_cmpgt_ps(min_rcp[0], _mm_setzero_ps())) == 0xf) ||
     (_mm_movemask_ps(_mm_cmplt_ps(max_rcp[0], _mm_setzero_ps())) == 0xf)) &&
    ((_mm_movemask_ps(_mm_cmpgt_ps(min_rcp[1], _mm_setzero_ps())) == 0xf) ||
     (_mm_movemask_ps(_mm_cmplt_ps(max_rcp[1], _mm_setzero_ps())) == 0xf)) &&
    ((_mm_movemask_ps(_mm_cmpgt_ps(min_rcp[2], _mm_setzero_ps())) == 0xf) ||
     (_mm_movemask_ps(_mm_cmplt_ps(max_rcp[2], _mm_setzero_ps())) == 0xf));

  if (!signChecked) {
    // variant for non-equal signs
    for (;;) {
      const BVHNode& thisNode = nodes[ID];
      firstActive = firstActivePort(packet, firstActive, thisNode.bounds);
      if (firstActive < RayPacket::MaxSize) {
        if (!thisNode.isLeaf()) {
          // inner node
          int front = frontSon[thisNode.axis];
          nodeStack[stackPtr].nodeID = thisNode.child + 1 - front;
          nodeStack[stackPtr].firstActive = firstActive;
          stackPtr++;
          ID = thisNode.child + front;
          continue;
        } else {
          // leaf
          int lastActive = lastActivePort(packet, firstActive, thisNode.bounds);
          // this part has to differ from DynBVH due to requirements
          // on primitive intersection.  Shouldn't hurt though.
          RayPacket subpacket(packet, firstActive, lastActive+1);

          for (int i = 0; i < thisNode.children; i++ ) {
            const int object_id = object_ids[thisNode.child+i];
            currGroup->get(object_id)->intersect(context,subpacket);
          }
        }
      }
      if (stackPtr <= 0) {
        return;
      }
      --stackPtr;
      ID = nodeStack[stackPtr].nodeID;
      firstActive = nodeStack[stackPtr].firstActive;
    }
  }

  // sign matches case
  __m128 min_org[3];
  __m128 max_org[3];

  if (!packet.getFlag(RayPacket::ConstantOrigin)) {
    for (int d = 0; d < 3; d++) {
      min_org[d] = max_org[d] = _mm_load_ps(&data->origin[d][0]);
      for (int i = 1; i < RayPacket::SSE_MaxSize; i++) {
        min_org[d] = _mm_min_ps(min_org[d], _mm_load_ps(&data->origin[d][i * 4]));
        max_org[d] = _mm_max_ps(max_org[d], _mm_load_ps(&data->origin[d][i * 4]));
      }
    }
  }

  __m128 max_t = _mm_load_ps(&data->minT[0]);
  for (int pack=1; pack < RayPacket::SSE_MaxSize; pack++) {
    max_t = _mm_max_ps(max_t, _mm_load_ps(&data->minT[pack * 4]));
  }

  const __m128 signCorrected_max_rcp[3] = {
    frontSon[0]?max_rcp[0]:min_rcp[0],
    frontSon[1]?max_rcp[1]:min_rcp[1],
    frontSon[2]?max_rcp[2]:min_rcp[2]
  };

  const __m128 signCorrected_min_rcp[3] = {
    !frontSon[0]?max_rcp[0]:min_rcp[0],
    !frontSon[1]?max_rcp[1]:min_rcp[1],
    !frontSon[2]?max_rcp[2]:min_rcp[2]
  };

  const __m128 signCorrected_max_org[3] = {
    !frontSon[0]?max_org[0]:min_org[0],
    !frontSon[1]?max_org[1]:min_org[1],
    !frontSon[2]?max_org[2]:min_org[2]
  };

  const __m128 signCorrected_min_org[3] = {
    frontSon[0]?max_org[0]:min_org[0],
    frontSon[1]?max_org[1]:min_org[1],
    frontSon[2]?max_org[2]:min_org[2]
  };

  for (;;) {
    const BVHNode& thisNode = nodes[ID];
    firstActive = firstActiveSameSignFrustumPort(packet, firstActive, thisNode.bounds, frontSon,
                                                 signCorrected_min_org,
                                                 signCorrected_max_org,
                                                 signCorrected_min_rcp,
                                                 signCorrected_max_rcp,
                                                 max_t);
    if (firstActive < RayPacket::MaxSize) {
      if (!thisNode.isLeaf()) {
        // inner node
        int front = frontSon[thisNode.axis];
        nodeStack[stackPtr].nodeID = thisNode.child + int(1-front); // NOTE(boulos): check this...
        nodeStack[stackPtr].firstActive = firstActive;
        stackPtr++;
        ID = thisNode.child + front;
        continue;
      } else {
        int lastActive = lastThatIntersectsSameSignPort(packet,
                                                        firstActive,
                                                        thisNode.bounds,
                                                        frontSon);
        // Same deal as above.
        RayPacket subpacket(packet, firstActive, lastActive+1);

        for (int i = 0; i < thisNode.children; i++ ) {
          const int object_id = object_ids[thisNode.child+i];
          currGroup->get(object_id)->intersect(context,subpacket);
        }
      }
    }
    if (stackPtr <= 0) {
      return;
    }
    --stackPtr;
    ID = nodeStack[stackPtr].nodeID;
    firstActive = nodeStack[stackPtr].firstActive;
  }
}

int DynBVH::firstActivePort(RayPacket& packet, int firstActive, const BBox& box) const {
  __m128 box_min_x = _mm_set1_ps(box[0][0]);
  __m128 box_min_y = _mm_set1_ps(box[0][1]);
  __m128 box_min_z = _mm_set1_ps(box[0][2]);

  __m128 box_max_x = _mm_set1_ps(box[1][0]);
  __m128 box_max_y = _mm_set1_ps(box[1][1]);
  __m128 box_max_z = _mm_set1_ps(box[1][2]);

  const RayPacketData* data = packet.data;
  const bool constant_origin = packet.getFlag(RayPacket::ConstantOrigin);

  // NOTE(boulos): To get rid of a warning, we'll just do this one
  // always. TODO(boulos): Make this code correctly handle variable
  // sized ray packets.
  __m128 diff_x_min = _mm_sub_ps(box_min_x, _mm_load_ps(&data->origin[0][0]));;
  __m128 diff_y_min = _mm_sub_ps(box_min_y, _mm_load_ps(&data->origin[1][0]));;
  __m128 diff_z_min = _mm_sub_ps(box_min_z, _mm_load_ps(&data->origin[2][0]));;

  __m128 diff_x_max = _mm_sub_ps(box_max_x, _mm_load_ps(&data->origin[0][0]));;
  __m128 diff_y_max = _mm_sub_ps(box_max_y, _mm_load_ps(&data->origin[1][0]));;
  __m128 diff_z_max = _mm_sub_ps(box_max_z, _mm_load_ps(&data->origin[2][0]));;

  for (int i=firstActive;i < RayPacket::MaxSize; i+=4) {
    __m128 t0 = _mm_set1_ps(T_EPSILON);
    __m128 t1 = _mm_load_ps(&data->minT[i]);

    if (!constant_origin) {
      diff_x_min = _mm_sub_ps(box_min_x, _mm_load_ps(&data->origin[0][i]));
      diff_y_min = _mm_sub_ps(box_min_y, _mm_load_ps(&data->origin[1][i]));
      diff_z_min = _mm_sub_ps(box_min_z, _mm_load_ps(&data->origin[2][i]));

      diff_x_max = _mm_sub_ps(box_max_x, _mm_load_ps(&data->origin[0][i]));
      diff_y_max = _mm_sub_ps(box_max_y, _mm_load_ps(&data->origin[1][i]));
      diff_z_max = _mm_sub_ps(box_max_z, _mm_load_ps(&data->origin[2][i]));
    }

    {
      const __m128 tBoxMin = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                        diff_x_min);
      const __m128 tBoxMax = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                        diff_x_max);
      t0 = _mm_max_ps(t0, _mm_min_ps(tBoxMin, tBoxMax));
      t1 = _mm_min_ps(t1, _mm_max_ps(tBoxMin, tBoxMax));
    }

    {
      const __m128 tBoxMin = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                        diff_y_min);
      const __m128 tBoxMax = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                        diff_y_max);
      t0 = _mm_max_ps(t0, _mm_min_ps(tBoxMin, tBoxMax));
      t1 = _mm_min_ps(t1, _mm_max_ps(tBoxMin, tBoxMax));
    }

    {
      const __m128 tBoxMin = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                        diff_z_min);
      const __m128 tBoxMax = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                        diff_z_max);
      t0 = _mm_max_ps(t0, _mm_min_ps(tBoxMin, tBoxMax));
      t1 = _mm_min_ps(t1, _mm_max_ps(tBoxMin, tBoxMax));
    }

    if (_mm_movemask_ps(_mm_cmple_ps(t0, t1)) != 0x0)
      return i;
  }
  return RayPacket::MaxSize;
}

int DynBVH::lastActivePort(RayPacket& packet, int firstActive, const BBox& box) const {
  __m128 box_min_x = _mm_set1_ps(box[0][0]);
  __m128 box_min_y = _mm_set1_ps(box[0][1]);
  __m128 box_min_z = _mm_set1_ps(box[0][2]);

  __m128 box_max_x = _mm_set1_ps(box[1][0]);
  __m128 box_max_y = _mm_set1_ps(box[1][1]);
  __m128 box_max_z = _mm_set1_ps(box[1][2]);

  const RayPacketData* data = packet.data;

  __m128 diff_x_min = _mm_sub_ps(box_min_x, _mm_load_ps(&data->origin[0][0]));
  __m128 diff_y_min = _mm_sub_ps(box_min_y, _mm_load_ps(&data->origin[1][0]));
  __m128 diff_z_min = _mm_sub_ps(box_min_z, _mm_load_ps(&data->origin[2][0]));

  __m128 diff_x_max = _mm_sub_ps(box_max_x, _mm_load_ps(&data->origin[0][0]));
  __m128 diff_y_max = _mm_sub_ps(box_max_y, _mm_load_ps(&data->origin[1][0]));
  __m128 diff_z_max = _mm_sub_ps(box_max_z, _mm_load_ps(&data->origin[2][0]));

  const int last_ray = (RayPacket::SSE_MaxSize - 1) * 4;
  for (int i=last_ray; i > firstActive; i -= 4) {
    __m128 t0 = _mm_set1_ps(T_EPSILON);
    __m128 t1 = _mm_load_ps(&data->minT[i]);

    if (!packet.getFlag(RayPacket::ConstantOrigin)) {
      diff_x_min = _mm_sub_ps(box_min_x, _mm_load_ps(&data->origin[0][i]));
      diff_y_min = _mm_sub_ps(box_min_y, _mm_load_ps(&data->origin[1][i]));
      diff_z_min = _mm_sub_ps(box_min_z, _mm_load_ps(&data->origin[2][i]));

      diff_x_max = _mm_sub_ps(box_max_x, _mm_load_ps(&data->origin[0][i]));
      diff_y_max = _mm_sub_ps(box_max_y, _mm_load_ps(&data->origin[1][i]));
      diff_z_max = _mm_sub_ps(box_max_z, _mm_load_ps(&data->origin[2][i]));
    }

    {
      const __m128 tBoxMin = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                        diff_x_min);
      const __m128 tBoxMax = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                        diff_x_max);
      t0 = _mm_max_ps(t0, _mm_min_ps(tBoxMin, tBoxMax));
      t1 = _mm_min_ps(t1, _mm_max_ps(tBoxMin, tBoxMax));
    }

    {
      const __m128 tBoxMin = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                        diff_y_min);
      const __m128 tBoxMax = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                        diff_y_max);
      t0 = _mm_max_ps(t0, _mm_min_ps(tBoxMin, tBoxMax));
      t1 = _mm_min_ps(t1, _mm_max_ps(tBoxMin, tBoxMax));
    }

    {
      const __m128 tBoxMin = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                        diff_z_min);
      const __m128 tBoxMax = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                        diff_z_max);
      t0 = _mm_max_ps(t0, _mm_min_ps(tBoxMin, tBoxMax));
      t1 = _mm_min_ps(t1, _mm_max_ps(tBoxMin, tBoxMax));
    }

    // Unlike DynBVH we want to return last active "ray", so the last
    // entry in this simd is the answer.
    if (_mm_movemask_ps(_mm_cmple_ps(t0, t1)) != 0x0)
      return i + 3;
  }
  return firstActive + 3;
}

int DynBVH::firstActiveSameSignFrustumPort(RayPacket& packet,
                                           const int firstActive,
                                           const BBox& box,
                                           const int signs[3],
                                           const __m128 sc_min_org[3],
                                           const __m128 sc_max_org[3],
                                           const __m128 sc_min_rcp[3],
                                           const __m128 sc_max_rcp[3],
                                           const __m128& max_t) const {

  const __m128 box_near_x = _mm_set1_ps(box[signs[0]][0]);
  const __m128 box_near_y = _mm_set1_ps(box[signs[1]][1]);
  const __m128 box_near_z = _mm_set1_ps(box[signs[2]][2]);

  const __m128 box_far_x = _mm_set1_ps(box[1-signs[0]][0]);
  const __m128 box_far_y = _mm_set1_ps(box[1-signs[1]][1]);
  const __m128 box_far_z = _mm_set1_ps(box[1-signs[2]][2]);

  const RayPacketData* data = packet.data;

  __m128 near_minus_org_x  = _mm_sub_ps(box_near_x, _mm_load_ps(&data->origin[0][0]));
  __m128 near_minus_org_y  = _mm_sub_ps(box_near_y, _mm_load_ps(&data->origin[1][0]));
  __m128 near_minus_org_z  = _mm_sub_ps(box_near_z, _mm_load_ps(&data->origin[2][0]));

  __m128 far_minus_org_x   = _mm_sub_ps(box_far_x, _mm_load_ps(&data->origin[0][0]));
  __m128 far_minus_org_y   = _mm_sub_ps(box_far_y, _mm_load_ps(&data->origin[1][0]));
  __m128 far_minus_org_z   = _mm_sub_ps(box_far_z, _mm_load_ps(&data->origin[2][0]));

  // test first (assumed) packet
  {
    const int i = firstActive;
    __m128 t0 = _mm_set1_ps(T_EPSILON);
    __m128 t1 = _mm_load_ps(&data->minT[i]);

    if (!packet.getFlag(RayPacket::ConstantOrigin)) {
      near_minus_org_x = _mm_sub_ps(box_near_x, _mm_load_ps(&data->origin[0][i]));
      near_minus_org_y = _mm_sub_ps(box_near_y, _mm_load_ps(&data->origin[1][i]));
      near_minus_org_z = _mm_sub_ps(box_near_z, _mm_load_ps(&data->origin[2][i]));
    }

    const __m128 tBoxNearX = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]), near_minus_org_x);
    const __m128 tBoxNearY = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]), near_minus_org_y);
    const __m128 tBoxNearZ = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]), near_minus_org_z);

    t0 = _mm_max_ps(t0, tBoxNearX);
    t0 = _mm_max_ps(t0, tBoxNearY);
    t0 = _mm_max_ps(t0, tBoxNearZ);

    if (!packet.getFlag(RayPacket::ConstantOrigin)) {
      far_minus_org_x = _mm_sub_ps(box_far_x, _mm_load_ps(&data->origin[0][i]));
      far_minus_org_y = _mm_sub_ps(box_far_y, _mm_load_ps(&data->origin[1][i]));
      far_minus_org_z = _mm_sub_ps(box_far_z, _mm_load_ps(&data->origin[2][i]));
    }

    const __m128 tBoxFarX = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]), far_minus_org_x);
    const __m128 tBoxFarY = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]), far_minus_org_y);
    const __m128 tBoxFarZ = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]), far_minus_org_z);

    t1 = _mm_min_ps(t1, tBoxFarX);
    t1 = _mm_min_ps(t1, tBoxFarY);
    t1 = _mm_min_ps(t1, tBoxFarZ);

    if (_mm_movemask_ps(_mm_cmple_ps(t0,t1)) != 0x0)
      return i;
  }

  // first greedy "maybe hit" test failed.

  if (packet.getFlag(RayPacket::ConstantOrigin)) {
    __m128 t0 = _mm_set1_ps(T_EPSILON);
    __m128 t1 = max_t;

    const __m128 tBoxNearX = _mm_mul_ps(sc_max_rcp[0], near_minus_org_x);
    const __m128 tBoxNearY = _mm_mul_ps(sc_max_rcp[1], near_minus_org_y);
    const __m128 tBoxNearZ = _mm_mul_ps(sc_max_rcp[2], near_minus_org_z);

    t0 = _mm_max_ps(t0, tBoxNearX);
    t0 = _mm_max_ps(t0, tBoxNearY);
    t0 = _mm_max_ps(t0, tBoxNearZ);

    const __m128 tBoxFarX = _mm_mul_ps(sc_min_rcp[0], far_minus_org_x);
    const __m128 tBoxFarY = _mm_mul_ps(sc_min_rcp[1], far_minus_org_y);
    const __m128 tBoxFarZ = _mm_mul_ps(sc_min_rcp[2], far_minus_org_z);

    t1 = _mm_min_ps(t1, tBoxFarX);
    t1 = _mm_min_ps(t1, tBoxFarY);
    t1 = _mm_min_ps(t1, tBoxFarZ);

    if (_mm_movemask_ps(_mm_cmple_ps(t0,t1)) == 0x0)
      return RayPacket::MaxSize;
  } else {
    // IA for non-constant origin
    __m128 t0 = _mm_set1_ps(T_EPSILON);
    __m128 t1 = max_t;

    near_minus_org_x = _mm_sub_ps(box_near_x, sc_max_org[0]);
    near_minus_org_y = _mm_sub_ps(box_near_y, sc_max_org[1]);
    near_minus_org_z = _mm_sub_ps(box_near_z, sc_max_org[2]);

    const __m128 tBoxNearX = _mm_mul_ps(sc_max_rcp[0], near_minus_org_x);
    const __m128 tBoxNearY = _mm_mul_ps(sc_max_rcp[1], near_minus_org_y);
    const __m128 tBoxNearZ = _mm_mul_ps(sc_max_rcp[2], near_minus_org_z);

    t0 = _mm_max_ps(t0, tBoxNearX);
    t0 = _mm_max_ps(t0, tBoxNearY);
    t0 = _mm_max_ps(t0, tBoxNearZ);

    far_minus_org_x = _mm_sub_ps(box_far_x, sc_min_org[0]);
    far_minus_org_y = _mm_sub_ps(box_far_y, sc_min_org[1]);
    far_minus_org_z = _mm_sub_ps(box_far_z, sc_min_org[2]);

    const __m128 tBoxFarX = _mm_mul_ps(sc_min_rcp[0], far_minus_org_x);
    const __m128 tBoxFarY = _mm_mul_ps(sc_min_rcp[1], far_minus_org_y);
    const __m128 tBoxFarZ = _mm_mul_ps(sc_min_rcp[2], far_minus_org_z);

    t1 = _mm_min_ps(t1, tBoxFarX);
    t1 = _mm_min_ps(t1, tBoxFarY);
    t1 = _mm_min_ps(t1, tBoxFarZ);

    if (_mm_movemask_ps(_mm_cmple_ps(t0,t1)) == 0x0)
      return RayPacket::MaxSize;
  }

  // frustum culling failed.  probably at least one ray hits...
  for (int i = firstActive + 4; i < RayPacket::MaxSize; i+= 4) {
    __m128 t0 = _mm_set1_ps(T_EPSILON);
    __m128 t1 = _mm_load_ps(&data->minT[i]);

    if (!packet.getFlag(RayPacket::ConstantOrigin)) {
      near_minus_org_x = _mm_sub_ps(box_near_x, _mm_load_ps(&data->origin[0][i]));
      near_minus_org_y = _mm_sub_ps(box_near_y, _mm_load_ps(&data->origin[1][i]));
      near_minus_org_z = _mm_sub_ps(box_near_z, _mm_load_ps(&data->origin[2][i]));
    }

    const __m128 tBoxNearX = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                        near_minus_org_x);
    const __m128 tBoxNearY = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                        near_minus_org_y);
    const __m128 tBoxNearZ = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                        near_minus_org_z);

    t0 = _mm_max_ps(t0, tBoxNearX);
    t0 = _mm_max_ps(t0, tBoxNearY);
    t0 = _mm_max_ps(t0, tBoxNearZ);

    if (!packet.getFlag(RayPacket::ConstantOrigin)) {
      far_minus_org_x = _mm_sub_ps(box_far_x, _mm_load_ps(&data->origin[0][i]));
      far_minus_org_y = _mm_sub_ps(box_far_y, _mm_load_ps(&data->origin[1][i]));
      far_minus_org_z = _mm_sub_ps(box_far_z, _mm_load_ps(&data->origin[2][i]));
    }

    const __m128 tBoxFarX = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                       far_minus_org_x);
    const __m128 tBoxFarY = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                       far_minus_org_y);
    const __m128 tBoxFarZ = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                       far_minus_org_z);

    t1 = _mm_min_ps(t1, tBoxFarX);
    t1 = _mm_min_ps(t1, tBoxFarY);
    t1 = _mm_min_ps(t1, tBoxFarZ);
    if (_mm_movemask_ps(_mm_cmple_ps(t0, t1)) != 0x0)
      return i;
  }

  return RayPacket::MaxSize;
}

int DynBVH::lastThatIntersectsSameSignPort(RayPacket& packet,
                                           const int firstActive,
                                           const BBox& box,
                                           const int signs[3]) const {
  const __m128 box_near_x = _mm_set1_ps(box[signs[0]][0]);
  const __m128 box_near_y = _mm_set1_ps(box[signs[1]][1]);
  const __m128 box_near_z = _mm_set1_ps(box[signs[2]][2]);

  const __m128 box_far_x = _mm_set1_ps(box[1-signs[0]][0]);
  const __m128 box_far_y = _mm_set1_ps(box[1-signs[1]][1]);
  const __m128 box_far_z = _mm_set1_ps(box[1-signs[2]][2]);

  const RayPacketData* data = packet.data;

  __m128 near_minus_org_x = _mm_sub_ps(box_near_x, _mm_load_ps(&data->origin[0][0]));
  __m128 near_minus_org_y = _mm_sub_ps(box_near_y, _mm_load_ps(&data->origin[1][0]));
  __m128 near_minus_org_z = _mm_sub_ps(box_near_z, _mm_load_ps(&data->origin[2][0]));

  __m128 far_minus_org_x  = _mm_sub_ps(box_far_x, _mm_load_ps(&data->origin[0][0]));
  __m128 far_minus_org_y  = _mm_sub_ps(box_far_y, _mm_load_ps(&data->origin[1][0]));
  __m128 far_minus_org_z  = _mm_sub_ps(box_far_z, _mm_load_ps(&data->origin[2][0]));

  // frustum culling failed.  probably at least one ray hits...
  const int last_ray = (RayPacket::SSE_MaxSize - 1) * 4;
  for (int i = last_ray; i > firstActive; i-= 4) {
    __m128 t0 = _mm_set1_ps(T_EPSILON);
    __m128 t1 = _mm_load_ps(&data->minT[i]);

    if (!packet.getFlag(RayPacket::ConstantOrigin)) {
      near_minus_org_x = _mm_sub_ps(box_near_x, _mm_load_ps(&data->origin[0][i]));
      near_minus_org_y = _mm_sub_ps(box_near_y, _mm_load_ps(&data->origin[1][i]));
      near_minus_org_z = _mm_sub_ps(box_near_z, _mm_load_ps(&data->origin[2][i]));
    }

    const __m128 tBoxNearX = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                        near_minus_org_x);
    const __m128 tBoxNearY = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                        near_minus_org_y);
    const __m128 tBoxNearZ = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                        near_minus_org_z);

    t0 = _mm_max_ps(t0, tBoxNearX);
    t0 = _mm_max_ps(t0, tBoxNearY);
    t0 = _mm_max_ps(t0, tBoxNearZ);

    if (!packet.getFlag(RayPacket::ConstantOrigin)) {
      far_minus_org_x = _mm_sub_ps(box_far_x, _mm_load_ps(&data->origin[0][i]));
      far_minus_org_y = _mm_sub_ps(box_far_y, _mm_load_ps(&data->origin[1][i]));
      far_minus_org_z = _mm_sub_ps(box_far_z, _mm_load_ps(&data->origin[2][i]));
    }

    const __m128 tBoxFarX = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[0][i]),
                                       far_minus_org_x);
    const __m128 tBoxFarY = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[1][i]),
                                       far_minus_org_y);
    const __m128 tBoxFarZ = _mm_mul_ps(_mm_load_ps(&data->inverseDirection[2][i]),
                                       far_minus_org_z);

    t1 = _mm_min_ps(t1, tBoxFarX);
    t1 = _mm_min_ps(t1, tBoxFarY);
    t1 = _mm_min_ps(t1, tBoxFarZ);
    if (_mm_movemask_ps(_mm_cmple_ps(t0, t1)) != 0x0)
      return i + 3;
  }

  return firstActive + 3;
}
#endif // MANTA_SSE for USE_DYNBVH_PORTS

#if TREE_ROT
// Tree rotations for dynamic scenes comes from the 2012 I3D paper "Fast,
// Effective BVH Updates for Animated Scenes" by Kopta et al.
void DynBVH::rotateNode(int const nodeID)
{
  const int leftID = nodes[ nodeID ].child;
  const int rightID = nodes[ nodeID ].child + 1;

  BVHNode& leftNode = nodes[leftID];
  if (leftNode.isLeaf() && leftNode.children > 1) {
    // Check to see if we should split leaf
    const int objectBegin = leftNode.child;
    const int objectEnd = leftNode.child + leftNode.children;
    build(leftID, objectBegin, objectEnd, true, true);
  }

  BVHNode& rightNode = nodes[rightID];
  if (rightNode.isLeaf() && rightNode.children > 1) {
    // Check to see if we should split leaf
    const int objectBegin = rightNode.child;
    const int objectEnd = rightNode.child + rightNode.children;
    build(rightID, objectBegin, objectEnd, true, true);
  }

  const int leftLeftID = nodes[ leftID ].child;
  const int leftRightID = nodes[ leftID ].child + 1;
  const int rightLeftID = nodes[ rightID ].child;
  const int rightRightID = nodes[ rightID ].child + 1;
  Real nodeArea = nodes[ nodeID ].bounds.computeArea();
  Real leftArea = nodes[ leftID ].bounds.computeArea();
  Real rightArea = nodes[ rightID ].bounds.computeArea();
  Real leftLeftArea = 0.0;
  Real leftRightArea = 0.0;
  Real rightLeftArea = 0.0;
  Real rightRightArea = 0.0;
  BBox lowerBoxA;
  BBox lowerBoxB;
  Real lowerAreaA = 0.0;
  Real lowerAreaB = 0.0;
  int best = 0;
  Real bestCost = ( nodeArea * BVH_C_trav +
                    leftArea * costs[ leftID ] +
                    rightArea * costs[ rightID ] );

  // Step 1: Which rotation reduces the cost of this node the most?
  if ( !nodes[ leftID ].isLeaf() )
  {
    leftLeftArea = nodes[ leftLeftID ].bounds.computeArea();
    leftRightArea = nodes[ leftRightID ].bounds.computeArea();
    Real partialCost = ( nodeArea * BVH_C_trav +
                         rightArea * costs[ rightID ] +
                         leftRightArea * costs[ leftRightID ] +
                         leftLeftArea * costs[ leftLeftID ] );
    // (1)    N                     N      //
    //       / \                   / \     //
    //      L   R     ----->      L   LL   //
    //     / \                   / \       //
    //   LL   LR                R   LR     //
    BBox lowerBox1( nodes[ rightID ].bounds );
    lowerBox1.extendByBox( nodes[ leftRightID ].bounds );
    Real lowerArea1 = lowerBox1.computeArea();
    Real upperCost1 = lowerArea1 * BVH_C_trav + partialCost;
    if ( upperCost1 < bestCost )
    {
      best = 1;
      bestCost = upperCost1;
      lowerBoxA = lowerBox1;
      lowerAreaA = lowerArea1;
    }
    // (2)    N                     N      //
    //       / \                   / \     //
    //      L   R     ----->      L   LR   //
    //     / \                   / \       //
    //   LL   LR               LL   R      //
    BBox lowerBox2( nodes[ rightID ].bounds );
    lowerBox2.extendByBox( nodes[ leftLeftID ].bounds );
    Real lowerArea2 = lowerBox2.computeArea();
    Real upperCost2 = lowerArea2 * BVH_C_trav + partialCost;
    if ( upperCost2 < bestCost )
    {
      best = 2;
      bestCost = upperCost2;
      lowerBoxA = lowerBox2;
      lowerAreaA = lowerArea2;
    }
  }
  if ( !nodes[ rightID ].isLeaf() )
  {
    rightLeftArea = nodes[ rightLeftID ].bounds.computeArea();
    rightRightArea = nodes[ rightRightID ].bounds.computeArea();
    Real partialCost = ( nodeArea * BVH_C_trav +
                         leftArea * costs[ leftID ] +
                         rightRightArea * costs[ rightRightID ] +
                         rightLeftArea * costs[ rightLeftID ] );
    // (3)    N                     N        //
    //       / \                   / \       //
    //      L   R     ----->     RL   R      //
    //         / \                   / \     //
    //       RL   RR                L   RR   //
    BBox lowerBox3( nodes[ leftID ].bounds );
    lowerBox3.extendByBox( nodes[ rightRightID ].bounds );
    Real lowerArea3 = lowerBox3.computeArea();
    Real upperCost3 = lowerArea3 * BVH_C_trav + partialCost;
    if ( upperCost3 < bestCost )
    {
      best = 3;
      bestCost = upperCost3;
      lowerBoxA = lowerBox3;
      lowerAreaA = lowerArea3;
    }
    // (4)    N                     N       //
    //       / \                   / \      //
    //      L   R     ----->     RR   R     //
    //         / \                   / \    //
    //       RL   RR               RL   L   //
    BBox lowerBox4( nodes[ leftID ].bounds );
    lowerBox4.extendByBox( nodes[ rightLeftID ].bounds );
    Real lowerArea4 = lowerBox4.computeArea();
    Real upperCost4 = lowerArea4 * BVH_C_trav + partialCost;
    if ( upperCost4 < bestCost )
    {
      best = 4;
      bestCost = upperCost4;
      lowerBoxA = lowerBox4;
      lowerAreaA = lowerArea4;
    }
    if ( !nodes[ leftID ].isLeaf() )
    {
      Real partialCost = ( nodeArea * BVH_C_trav +
                           leftLeftArea * costs[ leftLeftID ] +
                           leftRightArea * costs[ leftRightID ] +
                           rightLeftArea * costs[ rightLeftID ] +
                           rightRightArea * costs[ rightRightID ] );
      // (5)       N                      N        //
      //         /   \                  /   \      //
      //        L     R     ----->     L     R     //
      //       / \   / \              / \   / \    //
      //      LL LR RL RR            LL RL LR RR   //
      BBox lowerBox5L( nodes[ leftLeftID ].bounds );
      lowerBox5L.extendByBox( nodes[ rightLeftID ].bounds );
      Real lowerArea5L = lowerBox5L.computeArea();
      BBox lowerBox5R( nodes[ leftRightID ].bounds );
      lowerBox5R.extendByBox( nodes[ rightRightID ].bounds );
      Real lowerArea5R = lowerBox5R.computeArea();
      Real upperCost5 = ( lowerArea5L + lowerArea5R ) * BVH_C_trav + partialCost;
      if ( upperCost5 < bestCost )
      {
        best = 5;
        bestCost = upperCost5;
        lowerBoxA = lowerBox5L;
        lowerAreaA = lowerArea5L;
        lowerBoxB = lowerBox5R;
        lowerAreaB = lowerArea5R;
      }
      // (6)       N                      N        //
      //         /   \                  /   \      //
      //        L     R     ----->     L     R     //
      //       / \   / \              / \   / \    //
      //      LL LR RL RR            LL RR RL LR   //
      BBox lowerBox6L( nodes[ leftLeftID ].bounds );
      lowerBox6L.extendByBox( nodes[ rightRightID ].bounds );
      Real lowerArea6L = lowerBox6L.computeArea();
      BBox lowerBox6R( nodes[ rightLeftID ].bounds );
      lowerBox6R.extendByBox( nodes[ leftRightID ].bounds );
      Real lowerArea6R = lowerBox6R.computeArea();
      Real upperCost6 = ( lowerArea6L + lowerArea6R ) * BVH_C_trav + partialCost;
      if ( upperCost6 < bestCost )
      {
        best = 6;
        bestCost = upperCost6;
        lowerBoxA = lowerBox6L;
        lowerAreaA = lowerArea6L;
        lowerBoxB = lowerBox6R;
        lowerAreaB = lowerArea6R;
      }
    }
  }

  // Step 2: Apply that rotation and update all affected costs.
  costs[ nodeID ] = bestCost / nodeArea;

  if (best == 0)
    return;
  Real lowerAreaA_inv = 1 / lowerAreaA;

  switch( best )
  {
    case 0:
      return;
    case 1:
      nodes[ leftID ].bounds = lowerBoxA;
      costs[ leftID ] = BVH_C_trav + ( ( leftRightArea * costs[ leftRightID ] +
                                         rightArea * costs[ rightID ] ) * lowerAreaA_inv );
      swap( nodes[ leftLeftID ], nodes[ rightID ] );
      swap( costs[ leftLeftID ], costs[ rightID ] );
      swap( subtree_size[ leftLeftID ], subtree_size[ rightID ] );
      subtree_size[ leftID ] = (subtree_size[ leftLeftID ] +
                                subtree_size[ leftRightID ] + 1);
      break;
    case 2:
      nodes[ leftID ].bounds = lowerBoxA;
      costs[ leftID ] = BVH_C_trav + ( ( leftLeftArea * costs[ leftLeftID ] +
                                         rightArea * costs[ rightID ] ) * lowerAreaA_inv );
      swap( nodes[ leftRightID ], nodes[ rightID ] );
      swap( costs[ leftRightID ], costs[ rightID ] );
      swap( subtree_size[ leftRightID ], subtree_size[ rightID ] );
      subtree_size[ leftID ] = (subtree_size[ leftLeftID ] +
                                subtree_size[ leftRightID ] + 1);
      break;
    case 3:
      nodes[ rightID ].bounds = lowerBoxA;
      costs[ rightID ] = BVH_C_trav + ( ( rightRightArea * costs[ rightRightID ] +
                                          leftArea * costs[ leftID ] ) * lowerAreaA_inv );
      swap( nodes[ rightLeftID ], nodes[ leftID ] );
      swap( costs[ rightLeftID ], costs[ leftID ] );
      swap( subtree_size[ rightLeftID ], subtree_size[ leftID ] );
      subtree_size[ rightID ] = (subtree_size[ rightLeftID ] +
                                 subtree_size[ rightRightID ] + 1);
      break;
    case 4:
      nodes[ rightID ].bounds = lowerBoxA;
      costs[ rightID ] = BVH_C_trav + ( ( rightLeftArea * costs[ rightLeftID ] +
                                          leftArea * costs[ leftID ] ) * lowerAreaA_inv );
      swap( nodes[ rightRightID ], nodes[ leftID ] );
      swap( costs[ rightRightID ], costs[ leftID ] );
      swap( subtree_size[ rightRightID ], subtree_size[ leftID ] );
      subtree_size[ rightID ] = (subtree_size[ rightLeftID ] +
                                 subtree_size[ rightRightID ] + 1);
      break;
    case 5:
      nodes[ leftID ].bounds = lowerBoxA;
      nodes[ rightID ].bounds = lowerBoxB;
      costs[ leftID ] = BVH_C_trav + ( ( leftLeftArea * costs[ leftLeftID ] +
                                         rightLeftArea * costs[ rightLeftID ] ) * lowerAreaA_inv );
      costs[ rightID ] = BVH_C_trav + ( ( leftRightArea * costs[ leftRightID ] +
                                          rightRightArea * costs[ rightRightID ] ) / lowerAreaB );
      swap( nodes[ leftRightID ], nodes[ rightLeftID ] );
      swap( costs[ leftRightID ], costs[ rightLeftID ] );
      swap( subtree_size[ leftRightID ], subtree_size[ rightLeftID ] );
      subtree_size[ leftID ] = (subtree_size[ leftLeftID ] +
                                subtree_size[ leftRightID ] + 1);
      subtree_size[ rightID ] = (subtree_size[ rightLeftID ] +
                                 subtree_size[ rightRightID ] + 1);
      break;
    case 6:
      nodes[ leftID ].bounds = lowerBoxA;
      nodes[ rightID ].bounds = lowerBoxB;
      costs[ leftID ] = BVH_C_trav + ( ( leftLeftArea * costs[ leftLeftID ] +
                                         rightRightArea * costs[ rightRightID ] ) * lowerAreaA_inv );
      costs[ rightID ] = BVH_C_trav + ( ( leftRightArea * costs[ leftRightID ] +
                                          rightLeftArea * costs[ rightLeftID ] ) / lowerAreaB );
      swap( nodes[ leftRightID ], nodes[ rightRightID ] );
      swap( costs[ leftRightID ], costs[ rightRightID ] );
      swap( subtree_size[ leftRightID ], subtree_size[ rightRightID ] );
      subtree_size[ leftID ] = (subtree_size[ leftLeftID ] +
                                subtree_size[ leftRightID ] + 1);
      subtree_size[ rightID ] = (subtree_size[ rightLeftID ] +
                                 subtree_size[ rightRightID ] + 1);

  }
#if COUNT_SWAPS
  swapsDone++;
#endif
  // Step 3: Guess at a new "split" axis for affected nodes.
  if ( best <= 2 || best >= 5 )
  {
    Vector overlap( Max( nodes[ leftLeftID ].bounds.getMin(),
                         nodes[ leftRightID ].bounds.getMin() ) -
                    Min( nodes[ leftLeftID ].bounds.getMax(),
                         nodes[ leftRightID ].bounds.getMax() ) );
    nodes[ leftID ].axis = overlap.indexOfMaxComponent();
    if ( nodes[ leftLeftID ].bounds.getMax()[ nodes[ leftID ].axis ] >
         nodes[ leftRightID ].bounds.getMax()[ nodes[ leftID ].axis ] )
    {
      swap( nodes[ leftLeftID ], nodes[ leftRightID ] );
      swap( costs[ leftLeftID ], costs[ leftRightID ] );
      swap( subtree_size[ leftLeftID ], subtree_size[ leftRightID ] );
    }
  }
  if ( best >= 3 )
  {
    Vector overlap( Max( nodes[ rightLeftID ].bounds.getMin(),
                         nodes[ rightRightID ].bounds.getMin() ) -
                    Min( nodes[ rightLeftID ].bounds.getMax(),
                         nodes[ rightRightID ].bounds.getMax() ) );
    nodes[ rightID ].axis = overlap.indexOfMaxComponent();
    if ( nodes[ rightLeftID ].bounds.getMax()[ nodes[ rightID ].axis ] >
         nodes[ rightRightID ].bounds.getMax()[ nodes[ rightID ].axis ] )
    {
      swap( nodes[ rightLeftID ], nodes[ rightRightID ] );
      swap( costs[ rightLeftID ], costs[ rightRightID ] );
      swap( subtree_size[ rightLeftID ], subtree_size[ rightRightID ] );

    }
  }
  Vector overlap( Max( nodes[ leftID ].bounds.getMin(),
                       nodes[ rightID ].bounds.getMin() ) -
                  Min( nodes[ leftID ].bounds.getMax(),
                       nodes[ rightID ].bounds.getMax() ) );
  nodes[ nodeID ].axis = overlap.indexOfMaxComponent();
  if ( nodes[ leftID ].bounds.getMax()[ nodes[ nodeID ].axis ] >
       nodes[ rightID ].bounds.getMax()[ nodes[ nodeID ].axis ] )
  {
    swap( nodes[ leftID ], nodes[ rightID ] );
    swap( costs[ leftID ], costs[ rightID ] );
    swap( subtree_size[ leftID ], subtree_size[ rightID ] );
  }

  if (nodes[nodeID].isLargeSubtree) {
    nodes[leftID].isLargeSubtree = subtree_size[leftID] > largeSubtreeSize;
    nodes[rightID].isLargeSubtree = subtree_size[rightID] > largeSubtreeSize;

    // update grandchildren if they exist 
    if ( best <= 2 || best >= 5 )
      {
        nodes[leftLeftID].isLargeSubtree = subtree_size[leftLeftID] > largeSubtreeSize;
        nodes[leftRightID].isLargeSubtree = subtree_size[leftRightID] > largeSubtreeSize;
      }
    if(best >= 3)
      {
        nodes[rightLeftID].isLargeSubtree = subtree_size[rightLeftID] > largeSubtreeSize;
        nodes[rightRightID].isLargeSubtree = subtree_size[rightRightID] > largeSubtreeSize;
      }
  }
}

void DynBVH::rotateTree( int const nodeID )
{
  const BVHNode &node = nodes[ nodeID ];
  if ( !node.isLeaf() )
  {
    const int leftID = node.child;
    const int rightID = node.child + 1;
    rotateTree( leftID );
    rotateTree( rightID );
    rotateNode( nodeID );
  }
}
#endif // TREE_ROT

template <bool saveCost>
Real DynBVH::computeCost(int const nodeID )
{
  const BVHNode &node = nodes[ nodeID ];
  Real cost;
  if ( node.isLeaf() ) {
    //cost = node.children * BVH_C_isec + BVH_C_trav;
    cost = node.children * BVH_C_isec;
  }
  else
  {
    const int leftID = node.child;
    const int rightID = node.child + 1;
    Real leftCost = computeCost<saveCost>( leftID );
    Real rightCost = computeCost<saveCost>( rightID );
    Real area = node.bounds.computeArea();
    Real leftArea  = nodes[ leftID ].bounds.computeArea();
    Real rightArea = nodes[ rightID ].bounds.computeArea();
    cost = BVH_C_trav + ( leftArea * leftCost +
                          rightArea * rightCost ) / area;
  }
#if TREE_ROT
  if (saveCost)
    costs[nodeID] = cost;
#endif

  return cost;
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(DynBVH, AccelerationStructure, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(DynBVH);

#ifndef SWIG
  MANTA_DECLARE_RTTI_BASECLASS(DynBVH::BVHNode, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(DynBVH::BVHNode);
#endif
}

#ifndef SWIG
void DynBVH::BVHNode::readwrite(ArchiveElement* archive) {
  archive->readwrite("bounds", bounds);
  archive->readwrite("child", child);
  archive->readwrite("axis", axis);
  //archive->readwrite("isLeftCheaper", (char) isLeftCheaper);
  archive->readwrite("children", children);
}
#endif

void DynBVH::readwrite(ArchiveElement* archive) {
  MantaRTTI<AccelerationStructure>::readwrite(archive, *this);
  archive->readwrite("group", currGroup);
  archive->readwrite("object_ids", object_ids);
  archive->readwrite("nodes", nodes);
  if (archive->reading()) {
    int num_nodes_nonatomic;
    archive->readwrite("num_nodes", num_nodes_nonatomic);
    num_nodes.set(num_nodes_nonatomic);
  } else {
    int num_nodes_nonatomic = num_nodes;
    archive->readwrite("num_nodes", num_nodes_nonatomic);
  }
}

