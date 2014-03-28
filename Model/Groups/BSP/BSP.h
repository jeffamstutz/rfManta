#ifndef _MANTA_BSP_H_
#define _MANTA_BSP_H_

#include <Core/Geometry/BBox.h>
#include <Core/Geometry/VectorT.h>
#include <Interface/AccelerationStructure.h>
#include <Interface/RayPacket.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <Model/Groups/BSP/BSH.h>
#include <Model/Groups/BSP/Polytope.h>
#include <Model/Groups/BSP/Geometry.h>

#include <assert.h>

//BSP_INTERSECTION improves performance a little bit. It seems to
//work, but considering how much more complicated it makes things for
//just a small gain, I personally prefer to leave it off.
//#define BSP_INTERSECTION

//The BSP acceleration structure has a very slow build (best to save
//the tree to a file for future use) and only handles triangle meshes.
//It is however often faster than the other acceleration structures when
//the scene is "complicated" or if there are non-coherent packets.


namespace Manta
{
  class BSP : public AccelerationStructure
  {
  public:
    struct BSPNode
    {
//     //plane is:     dot(normal, X) + d == 0
//     //Half-space is dot(normal, X) + d <= 0
      Vector normal;
      union {
        Real d;
#ifdef BSP_INTERSECTION
        struct {
          unsigned int objectsIndex : 30;
          unsigned int triangleBounds : 2; //0x1 is all 3 edges bounded,
                                           //0x2 is face bounded
        };
#else
        unsigned int objectsIndex;
#endif
      };

      unsigned int children; //leftChild=children. rightChild=children+1.

      inline void makeLeaf(unsigned int index, int nObjects) {
        setAsLeaf();
        setNumPrimitives(nObjects);
        objectsIndex = index;
#ifdef BSP_INTERSECTION
        triangleBounds = 0;
#endif
      }

      inline void makeInternal(int first_child, const Vector &normal, Real d) {
        children = first_child;
        this->normal = normal;
        this->d = d;
        checkAndMakeKDTree();
      }

      inline bool isLeaf() const {
        return children == 0;
      }
      inline void setAsLeaf() {
        children=0;
      }

      inline unsigned int numPrimitives() const {
        assert(isLeaf());
        union {
          Real f;
          unsigned int i;
        };
        f = normal[0];
        return i;
      }
      inline void setNumPrimitives(unsigned int size) {
        assert(isLeaf());
        union {
          Real f;
          unsigned int i;
        };
        i = size;
        normal[0] = f;
      }

      inline void checkAndMakeKDTree() {
        assert(!isLeaf());

        if (!(normal[0]*normal[1] == 0 &&
              normal[0]*normal[2] == 0 &&
              normal[1]*normal[2] == 0))
          return;

        if (normal.maxComponent() > 0.9999999) {
          union {
            Real f;
            unsigned int i;
          };
          i = normal.indexOfMaxComponent();
          normal[1] = f;
          normal[0] = 10;
          d*=-1;
        }
      }
      inline bool isKDTree() const {
        assert(!isLeaf());
        return normal[0] == 10;
      }

      inline int kdtreePlaneDim() const {
        assert(isKDTree());
        union {
          Real f;
          unsigned int i;
        };
        f = normal[1];
        return i;
      }

#ifdef BSP_INTERSECTION
      inline void setTriPlaneDepth(int depth) {
        union {
          Real f;
          int i;
        };
        i = depth;
        normal[2] = f;
      }
      inline int getTriPlaneDepth() const {
        union {
          Real f;
          int i;
        };
        f = normal[2];
        return i;
      }
#endif
    };

    vector<BSPNode> nodes;
    vector<int> objects; //vector of objects added to BSP tree. Might
                         //contain the same object multiple times
                         //(object split by plane)

    //If this BSP is a subset of another acceleration structure, then
    //if intersect() is called on this bsp we don't need to do a
    //bounds intersection test since we implicitly know that already
    //happened in order to get to this node.
    bool isSubset;

    //getting the triangle vertex data is expensive, especially
    //converting from float to double. So here we make it easier to
    //get. Each element of vertices contains 3 Points corresponding
    //to a triangle.
    vector<Point*>vertices;

    BBox bounds;

    Mesh *mesh;

    Polytope polytope;

    //for performance measurements. Can be removed.
    mutable long nTriIntersects;
    mutable long nBSPTriIntersects;
    mutable long nTraversals;
    mutable long nKDTraversals;
    mutable long nLeafs;
    mutable long nTotalRays;

    void update(int proc, int numProcs)
    {
      //#define COLLECT_STATS
    #ifdef COLLECT_STATS
      printf("tri intersections per ray:       %f\n", (float)nTriIntersects/nTotalRays);
      printf("BSP tri intersections per ray:   %f\n", (float)nBSPTriIntersects/nTotalRays);
      printf("node traversals per ray:         %f\n", (float)nTraversals/nTotalRays);
      printf("node bsp traversals per ray:     %f\n", (float)(nTraversals-nKDTraversals)/nTotalRays);
      printf("node kdtree traversals per ray:  %f\n", (float)(nKDTraversals)/nTotalRays);
      printf("leaf visits per ray:             %f\n", (float)(nLeafs)/nTotalRays);
      printf("number of rays:                  %ld\n", nTotalRays);
      nTriIntersects = 0;
      nBSPTriIntersects = 0;
      nTraversals = 0;
      nKDTraversals = 0;
      nLeafs = 0;
      nTotalRays  = 0;
    #endif
    }

    BSP() : mesh(NULL), nTriIntersects(0), nBSPTriIntersects(0),
            nTraversals(0), nKDTraversals(0),
            nLeafs(0), nTotalRays(0)
    {}

    void preprocess(const PreprocessContext&);
    void intersect(const RenderContext& context, RayPacket& rays) const;
    inline void intersectSingleRays(const RenderContext& context, RayPacket& rays) const;
    inline void intersectPacket(const RenderContext& context, RayPacket& rays) const;

    struct TrvStack {
      int nodeID;
      float t;
      int depth;
      int farPlane;
    };

    void traverse(const RenderContext &context, RayPacket &ray,
                       const Vector &org, const Vector &dir, const Vector &rcp,
                       TrvStack *const stackBase) const;

    void computeBounds(const PreprocessContext& context,
                       BBox& bbox) const
    {
      mesh->computeBounds(context, bbox);
    }

    void setGroup(Group* new_group);
    void groupDirty() {} // does nothing for now
    Group* getGroup() const { return mesh; }

    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent) { }

    void rebuild(int proc=0, int numProcs=1);

    void rebuildSubset(const vector<int> &subsetObjects, const BBox &bounds);

    void setTriangleBounds(vector<BuildSplitPlane> &splitPlanes, int &nodeID,
                           int &nextFreeNode);

    void build(int nodeID, int &nextFreeNode,
               vector<BuildSplitPlane> &splitplanes, BSH &bsh, int depth = 0);

    BuildSplitPlane getBuildSplitPlane(BSH &primitives,
                             vector<BuildSplitPlane> &splitPlanes,
                             Location &coplanar_side);
    BuildSplitPlane getSAHSplit(BSH &primitives,
                                vector<BuildSplitPlane> &splitPlanes,
                                Location &coplanar_primitivesSide);

    struct SplitData {
      SplitData() : splitPrimitive(-1), splitType(-1) {
      }
      BuildSplitPlane split;
      int splitPrimitive;
      int splitType; //0 for face, 1, 2, 3 for the three edges, etc...
      int nLeft, nRight;
      float lArea, rArea;
      float cost;
      Location coplanar_side;
    };

    //assume last BuildSplitPlane in planes determines left and right SA.
    bool getSurfaceAreas(SplitData &splitdata);

    void calculateSAH(float parentArea, SplitData &splitdata,
                      const BSH &primitives, const Polytope &polytope);

    BuildSplitPlane getBestSplitPlane(const BSH &primitives,
                                      BuildSplitPlane plane,
                                      const vector<Point> &initialVertices);

    bool buildFromFile(const string &file);
    bool saveToFile(const string &file);

    void printPrimitivesAsObj(const BSH &primitives) const;
    void printOriginalPrimitivesAsObj(const BSH &primitives) const;

    void printStats();
    struct TreeStats{
      static const int MAX = 1024;
      float total_SAH_cost;
      int maxDepth;
      int maxObjectsInLeaf;
      int childrenLeafHist[MAX]; //Better not have more than MAX primitives in a node!
      int splitType[MAX][2]; //Tree better not be more than MAX deep!
    };
    void collectTreeStats(int nodeID, int depth, TreeStats &stats);
  };
};

#endif
