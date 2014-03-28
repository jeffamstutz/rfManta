#ifndef chainhull_h
#define chainhull_h
#include <Core/Geometry/VectorT.h>
#include <algorithm>

namespace Manta {
namespace BSPs
{
  typedef VectorT<double, 2> Point2D;

  //lexicographic sort
  struct lexicographic2DPoint {
    bool operator()(const Point2D &a, const Point2D &b)
    {
      return a[0] < b[0] || (a[0] == b[0] && a[1] < b[1]);
    }
  };

  //http://www.algorithmist.com/index.php/Monotone_Chain_Convex_Hull.cpp
  // 2D cross product.  Return a positive value, if OAB makes a
  // counter-clockwise turn, negative for clockwise turn, and zero if
  // the points are collinear.
  inline double cross(const Point2D &O, const Point2D &A, const Point2D &B)
  {
    return (A[0] - O[0]) * (B[1] - O[1]) - 
      (A[1] - O[1]) * (B[0] - O[0]);
  }

  // Returns a list of points on the convex hull in counter-clockwise
  // order.  Note: the last point in the returned list is the same as
  // the first one.
  void convexHull(vector<Point2D> &P, vector<Point2D> &H)
  {
    int n = P.size(), k = 0;
    H.resize(2*n);

    // Sort points lexicographically
    sort(P.begin(), P.end(), lexicographic2DPoint());

    // Build lower hull
    for (int i = 0; i < n; i++) {
      while (k >= 2 && cross(H[k-2], H[k-1], P[i]) <= 0) k--;
      H[k++] = P[i];
    }

    // Build upper hull
    for (int i = n-2, t = k+1; i >= 0; i--) {
      while (k >= t && cross(H[k-2], H[k-1], P[i]) <= 0) k--;
      H[k++] = P[i];
    }

    H.resize(k);
  }
};
};
#endif //chainhull_h
