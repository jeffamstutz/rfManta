#ifndef _MANTA_BSH_H
#define _MANTA_BSH_H

#include <Model/Groups/BSP/Geometry.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>
#include <Model/Groups/Mesh.h>

namespace Manta
{
  using namespace BSPs;
  enum Location{negativeSide, positiveSide, bothSides, eitherSide};

  class BSH
  {
  public:
    struct BSHNode
    {
      BBox bounds;
      Point center;
      double radius;

      union {
        unsigned int children;  //INTERNAL NODE:
                                //leftChild=children. rightChild=children+1.
        unsigned int objectsIndex;  //LEAF NODE:
      };

      int parent;

      int size; //number of primitives contained by this node and its
                //children
      int numSplits; //number of triangles that were split by this node tree.
      
      bool _isLeaf;

      inline void makeLeaf(int begin, int end) {
        objectsIndex = begin;
        size = end - begin;
        numSplits=0;
        _isLeaf = true;
      }

      inline void makeInternal(int first_child) {
        children = first_child;
        _isLeaf = false;
        numSplits = 0;
      }

      inline bool isLeaf() const {
        return _isLeaf;
      }

      inline Location whichSide(const BuildSplitPlane& plane) const {
        const double dist = signedDistance(plane, center);
        if (dist < -radius)
          return negativeSide;
        else if (dist > radius)
          return positiveSide;
        else {
          return bothSides;
        }
      }
      inline bool nearPlaneDist(const BuildSplitPlane& plane,
                                double maxError) const {
        const double dist = signedDistance(plane, center);
        return (fabs(dist) <= radius+maxError);
      }
      inline bool nearPlaneAngle(const BuildSplitPlane& plane,
                                 const Point &p,
                                 double minAngle) const {
        const double dist = signedDistance(plane, center);
        if (fabs(dist) < radius)
          return true;
        const double r = dist < 0 ? -radius : radius;

        const Point center_p = center-p;

        const Point v1 = (center_p - plane.normal*dist);
        const Point v2 = (center_p - plane.normal*r);
          
        const double costheta = 1-fabs(Dot(v1, v2)) *
          (inverseSqrt(static_cast<float>(v1.length2())) *
           inverseSqrt(static_cast<float>(v2.length2())));
        return costheta < minAngle;
      }
    };
    
    vector<BSHNode> nodes;

    struct ClippedTriangle {
      int originalTriID;
      Point tri[3]; //vertices of the clipped triangle
      BuildSplitPlane plane;
      bool fixedPlane;
      
      ClippedTriangle() { } //Do not use. This is only for std::vector
      ClippedTriangle(int originalTriID, const Point &v0,
                      const Point &v1, const Point &v2) : 
        originalTriID(originalTriID), fixedPlane(false) {
        tri[0] = v0;
        tri[1] = v1;
        tri[2] = v2;

        plane.normal = getTriangleNormal(tri[0], tri[1], tri[2]).normal();
        plane.d = Dot(plane.normal, -(tri[0]+tri[1]+tri[2])/3.0);
      }
    };
    vector<ClippedTriangle> triID;

    Mesh* mesh;

    BSH() { }
    BSH(const BSH &bsh) {
      nodes = bsh.nodes;
      mesh = bsh.mesh;
    }
    
    //this becomes negative side of plane and positiveBSH is the
    //positive side.
    void split(const BuildSplitPlane &plane, BSH &positiveBSH,
               const Location coplanar_primitiveSide);

    void build(Mesh *mesh, const BBox &bounds);
    void buildSubset(Mesh *mesh, const vector<int> &subsetObjects,
                     const BBox &bounds);

    inline size_t size() const { return nodes[0].size; }
    inline size_t numSplits() const { return nodes[0].numSplits; }

    void pointsNearPlaneDist(vector<Point> &points,
                             const BuildSplitPlane &plane,
                             const int nodeID=0) const;
    void pointNearPlaneAngle(const vector<Point> &points,
                             const BuildSplitPlane &plane,
                             Point &closePoint, double &minAngle,
                             const int nodeID=0) const;

    void countLocations(const BuildSplitPlane &plane,
                        int &nLeft, int &nRight, int &nEither,
                        const int nodeID=0) const;

    class Iterator {
    public:
      const BSH& bsh;
      int currNode;
      int i;

      Iterator(const BSH& bsh, int currNode) :
        bsh(bsh), currNode(currNode), i(-1) { }

      const ClippedTriangle &get() const { 
        return bsh.triID[bsh.nodes[currNode].objectsIndex+i]; 
      }
      Iterator &next();
      bool operator!=(const Iterator &iter) const {
        return (currNode != iter.currNode) || (i != iter.i);
      }
    };
    Iterator begin() const;
    inline static const Iterator& end() { 
      const static BSH foo;
      const static Iterator endIterator(foo, 0);
      return endIterator; 
    }

  private:
    void localMedianBuild(int nodeID, int &nextFree, 
                          const size_t begin, const size_t end);

    void spatialMedianBuild(int nodeID, int &nextFree,
                            const size_t begin, const size_t end,
                            const BBox &bounds);
    void refit(size_t nodeID);

    void splitTriangle(const ClippedTriangle &tri,
                       const BuildSplitPlane &plane,
                       ClippedTriangle negSide[2], 
                       ClippedTriangle posSide[2]);

    void split(const BuildSplitPlane &plane, BSH &positiveBSH, 
               const Location coplanar_primitiveSide,
               size_t nodeID);

    void nodeTriIDCopy(size_t nodeID, const vector<ClippedTriangle> &triID,
                       vector<ClippedTriangle> &tmpCTri);

    void compact(size_t &highestSeen, size_t node=0);

    Location getLocation(const ClippedTriangle &ctri, 
                         const BuildSplitPlane &plane) const;

    //These are used only during the build (maybe I should just pass
    //these as arguments?).
    static vector<int> tmpTriID1, tmpTriID2;
    static vector<ClippedTriangle> tmpCTri;
    static vector<Vector> triCentroid;
  };
};
#endif
