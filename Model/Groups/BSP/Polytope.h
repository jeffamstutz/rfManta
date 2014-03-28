#ifndef Polytope_h
#define Polytope_h

#include <Model/Groups/BSP/Geometry.h>
#include <Core/Geometry/BBox.h>
#include <vector>
#include <set>
#include <iostream>
using namespace std;

namespace Manta {
  using namespace BSPs;

  class Polytope {
  public:    
    Polytope();
    ~Polytope();

    mutable bool debugPrint;

    float getSurfaceArea();

    void printAsObj(int offset=0) const;
    void printAsPly();
    void clear();
    void initialize(const BBox &bounds);
    bool isFlat() const;

    void split(const BuildSplitPlane &plane, Polytope &side1, Polytope &side2) const;

    void updateVertices();
//   private:
    vector<Polygon> faces;
    vector<Point> vertices;

    struct PointDist {
      double dist;
      bool offPlane;
      PointDist(double dist, bool offPlane) : dist(dist), offPlane(offPlane) { }
    };
    typedef map<Point, PointDist, ltPoint> PointDistMap;
    
    bool findSplitEdge(PointDistMap &pointDists, const Polygon &face) const;
    void findPointsOnPlane(const BuildSplitPlane& plane, 
                           PointDistMap &pointDists) const;
    
    void splitPolygon(const PointDistMap &pointDists, 
                      const BuildSplitPlane &plane, const Polygon &in,
                      Polygon &side1, Polygon &side2, Polygon &cap) const;

    bool verifyWindingOrder() const;
    bool hasSimilarFace(const Polytope &poly, const Polygon &cap) const;
    void cleanup();
  };
};

#endif
