#ifndef Geometry_h
#define Geometry_h

#include <Core/Geometry/VectorT.h>
#include <Core/Math/SSEDefs.h>

#include <vector>

#include <iostream>
using namespace std;

//#define BUILD_DEBUG

typedef Manta::VectorT<double, 2> Point2D;

namespace Manta {
  namespace BSPs {

  typedef VectorT<double, 3> Point;
  typedef vector<Point> Polygon;   //Assume this is a convex planar polygon.

  class BuildSplitPlane
  {
  public:
    //plane is:     dot(normal, X) + d == 0
    //Half-space is dot(normal, X) + d <= 0 
    Point normal;
    double d;

    //point to other half-space
    void flip() {
      normal = -normal;
      d = -d;
    }

    bool operator<(const BuildSplitPlane &p) const
    {
      for (int i=0; i < 3; ++i) {
        if (normal[i] < p.normal[i]) return true;
        if (normal[i] > p.normal[i]) return false;
      }
      return d < p.d;
    }
  };


  void printPolygon(const Polygon &face);

  bool isDegenerateFace(const Polygon &face);

  bool overlaps(const Polygon &face1, const Polygon &face2,
                float &intersectedArea, 
                bool forceIntersectionAreaComputation=false);
  inline bool overlaps(const Polygon &face1, const Polygon &face2) {
    float intersectedArea=0;
    return overlaps(face1, face2, intersectedArea);
  }

  bool isSubset(const Polygon &smallFace, const Polygon &bigFace);

  Point getTriangleNormal(const Point &p0, const Point &p1,
                          const Point &p2);

  //The points in face might not be ordered.
  //Note that the returned normal could point in either direction.
  Point getFaceNormal(const Polygon& face);

  //The points in the face are ordered around the polygon.
  Point getCenter(const Polygon& face);
  //The points in the face are ordered around the polygon.
  Point getOrderedFaceNormal(const Polygon& face);

  void getOrthonormalBasis(const Point &w, Point &u, Point &v); 

  double getArea(const Polygon& face);

  double getArea2D(vector<Point2D>& face);
  double getTriangleArea(const Point &p1, const Point &p2,
                         const Point &p3);

  void radialSort(Polygon &cap);

  bool verifyUnique(const Polygon &face);

  bool almostEqualRelative(double A, double B,
                           double maxRelativeError=1e-6, 
                           double maxAbsoluteError=1e-7);

  inline bool almostEqualPoints(const Point &l, const Point &r,
                                double maxAbsoluteError=1e-6)
  {
    return fabs(l[0]-r[0]) < maxAbsoluteError &&
           fabs(l[1]-r[1]) < maxAbsoluteError &&
           fabs(l[2]-r[2]) < maxAbsoluteError;
  }

  inline bool almostEqualPointsRelative(const Point &l, const Point &r,
                                double maxRelativeError=1e-6, double maxAbsoluteError=1e-7)
  {
    return almostEqualRelative(l[0], r[0], maxRelativeError, maxAbsoluteError) &&
      almostEqualRelative(l[1], r[1], maxRelativeError, maxAbsoluteError) &&
      almostEqualRelative(l[2], r[2], maxRelativeError, maxAbsoluteError);
  }



  inline double signedDistance(const BuildSplitPlane &plane, const Point &p)
  {
    return Dot(plane.normal, p) + plane.d;
  }

  double distanceAlongPlane(const Point basis[2], const Point &center,
                            const BuildSplitPlane &plane, const Point &p);


  inline float inverseSqrt(float f) {
#ifdef MANTA_SSE
    sse_t x = _mm_load_ss(&f);
    x = _mm_rsqrt_ss(x); //we don't need much precision here.
    float rsqrt;
    _mm_store_ss( &rsqrt, x );
    //The compiler should keep the next line in xmm registers
    return rsqrt*(1.5f - 0.5f*f*rsqrt*rsqrt);
#else
    return 1.0f / Sqrt(f);
#endif
  }

  struct ltPoint {
    bool operator()(const Point &a, const Point &b) const
    {
      for (int i=0; i < 3; ++i) {
        if (a[i] < b[i]) return true;
        if (a[i] > b[i]) return false;
      }
      return false; //they are equal
    }
  };
  };
};

#endif //Geometry_h
