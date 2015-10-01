#ifndef Manta_Model_Groups_KDTree_h
#define Manta_Model_Groups_KDTree_h

#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>
#include <Core/Thread/Barrier.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/AccelerationStructure.h>
#include <Model/Primitives/MeshTriangle.h>
#include <Interface/RayPacket.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <stdio.h>
#include <assert.h>

// #define COLLECT_STATS

// define RTSAH if you want to use RTSAH traversal order
// #define RTSAH //note: this currently only works with single ray traversal.
              //Packet traversal will see no improvement if this is enabled.


namespace Manta
{

  class KDTree : public AccelerationStructure
  {
  public:

#ifdef _MSC_VER
#  ifdef MANTA_SSE
#    define KDTREE_MANTA_SSE MANTA_SSE
#    undef MANTA_SSE
#  endif
#endif

#ifndef SWIG

#ifdef MANTA_SSE
# define MAILBOX

// must be a power of two...
# define MAILBOX_ENTRIES 64
# define MAILBOX_MASK (MAILBOX_ENTRIES-1)
    struct Mailbox
    {
      MANTA_ALIGN(16) int triID[MAILBOX_ENTRIES];

      inline void clear()
      {
        sse_int_t m1 = set4i(-1);
        MANTA_UNROLL(4)
        for (int i=0;i<MAILBOX_ENTRIES;i+=4)
          store44i((sse_int_t*)(triID+i),m1);
      }
      inline bool testAndMark(int id)
      {
        const int hash = id & MAILBOX_MASK;
        if (triID[hash] == id)
          return true;
        triID[hash] = id;
        return false;
      }
    };
#endif //MANTA_SSE

    struct Node
    {
      union {
        float planePos;
        int numPrimitives;
      };
      unsigned int isLeaf : 1;
      unsigned int planeDim : 2;
#ifdef RTSAH
      unsigned int isLeftCheaper : 1;
      unsigned int childIdx : 28;
#else
      unsigned int childIdx : 29;
#endif // RTSAH
    };
    vector <Node> nodes;
    vector <int> itemList;

    void makeLeaf(unsigned int nodeID, vector<int> &prim)
    {
      nodes[nodeID].isLeaf = 1;
      nodes[nodeID].childIdx = itemList.size();
      nodes[nodeID].numPrimitives = prim.size();
      copy(prim.begin(),prim.end(),back_inserter(itemList));
    }
    void makeInner(unsigned int nodeID,
                   int dim, float pos)
    {
      nodes[nodeID].isLeaf = 0;
      nodes[nodeID].childIdx = nodes.size();
      nodes[nodeID].planePos = pos;
      nodes[nodeID].planeDim = dim;
      nodes.push_back(Node());
      nodes.push_back(Node());
    }

    BBox bounds;

    Group *currGroup;
    Mesh *mesh;

#ifdef COLLECT_STATS
    //for performance measurements. Can be removed.
    struct Stats {
      Stats () { reset(); }

      void reset() {
        nIntersects = 0;
        nTraversals = 0;
        nTotalRays = 0;
        nTotalRaysInPacket = 0;
        nTotalPackets = 0;
        nEmptyLeavesVisited = 0;
        nLeavesVisited = 0;
      }

      long nIntersects;
      long nTraversals;
      long nTotalRays;
      long nTotalRaysInPacket;
      long nTotalPackets;
      long nEmptyLeavesVisited;
      long nLeavesVisited;

      // hopefully big enough to keep any false sharing among different
      // processors from occuring.
      char emptySpace[128];
    };

    mutable vector<Stats> stats; // one per thread

    Stats accumulateStats() const {
      Stats finalStats;
      for (size_t i=0; i < stats.size(); ++i) {
        finalStats.nIntersects += stats[i].nIntersects;
        finalStats.nTraversals += stats[i].nTraversals;
        finalStats.nTotalRays += stats[i].nTotalRays;
        finalStats.nTotalRaysInPacket += stats[i].nTotalRaysInPacket;
        finalStats.nTotalPackets += stats[i].nTotalPackets;
        finalStats.nEmptyLeavesVisited +=stats[i].nEmptyLeavesVisited;
        finalStats.nLeavesVisited +=stats[i].nLeavesVisited;
      }
      return finalStats;
    }
#endif
    Barrier barrier;

#endif // SWIG
    void updateStats(int proc, int numProcs)
    {
#ifdef COLLECT_STATS
      if (proc == 0) {
        Stats finalStats = accumulateStats();

        printf("intersections per packet:        %f\n", (float)finalStats.nIntersects/finalStats.nTotalPackets);
        printf("node traversals per packet:      %f\n", (float)finalStats.nTraversals/finalStats.nTotalPackets);
        printf("empty/nonempty leaves per packet:%f / %f\n", (float)finalStats.nEmptyLeavesVisited/finalStats.nTotalPackets,
               (float)(finalStats.nLeavesVisited - finalStats.nEmptyLeavesVisited)/finalStats.nTotalPackets);
        printf("average ray packet size:         %f\n", (float)finalStats.nTotalRaysInPacket/finalStats.nTotalPackets);
        printf("number of packets:               %ld\n", finalStats.nTotalPackets);
        printf("number of rays:                  %ld\n", finalStats.nTotalRays);
        printf("\n");

        stats.resize(numProcs);
      }

      barrier.wait(numProcs);

      stats[proc].reset();
#endif
    }

    KDTree() : currGroup(NULL), mesh(NULL), barrier("KDTreer barrier")
    {
#ifdef COLLECT_STATS
      stats.resize(1);
#endif
    }

    void preprocess(const PreprocessContext&);
    void intersect(const RenderContext& context, RayPacket& rays) const;
#ifndef SWIG
    template<bool COMMON_ORIGIN>
    void intersectNode(unsigned int nodeID,
                       const RenderContext& context,
                       RayPacket& rays,
                       const Real *const  t_in,
                       const Real *const  t_out,
                       const int  *const  valid
#ifdef MAILBOX
                       , Mailbox &mailbox
#endif
                       ) const;
#endif

#ifndef SWIG
    //single ray traversal
    struct TrvStack {
      unsigned int nodeID;
      float t;
      float t_n;
    };
    template<bool anyHit>
    void traverse(const RenderContext &context, RayPacket &ray,
                  const Vector &org, const Vector &dir,
                  const Vector &rcp, TrvStack *const stackBase) const;
#endif

    bool intersectBounds(const RenderContext& context, RayPacket& rays) const;

    void computeBounds(const PreprocessContext& context,
                       BBox& bbox) const
    {
      currGroup->computeBounds(context, bbox);
    }

    void setGroup(Group* new_group);
    void groupDirty() {} // does nothing for now
    Group* getGroup() const { return currGroup; }

    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent) { }

    void rebuild(int proc=0, int numProcs=1);

    void build(unsigned int nodeID,
               vector<int> &primitives,
               const BBox &bounds,
               double &totalCost, int depth=0);

    bool buildFromFile(const string &file);
    bool saveToFile(const string &file);

    void printStats();

  protected:

    void computeTraversalCost();
    VectorT<float, 2> computeSubTreeTraversalCost(unsigned int nodeID,
                                                  const BBox& nodeBounds,
                                                  float nodeSA);

#ifndef SWIG
    struct TreeStats{
      int maxDepth;
      int maxObjectsInLeaf;
      int childrenLeafHist[1024]; //hopefully 1024 is more than enough
    };
    void collectTreeStats(unsigned int nodeID, int depth, TreeStats &stats);
#endif
  };

#ifdef _MSC_VER
#  ifdef KDTREE_MANTA_SSE
#    define MANTA_SSE KDTREE_MANTA_SSE
#    undef KDTREE_MANTA_SSE
#  endif
#endif

} // end namespace Manta

#endif
