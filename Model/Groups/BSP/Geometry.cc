#include <Model/Groups/BSP/Geometry.h>
#include <Core/Geometry/AffineTransformT.h>
#include <Model/Groups/BSP/aip.h>
#include <set>
#include <algorithm>

using namespace std;

namespace Manta {
  namespace BSPs {
  //http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
  bool almostEqualRelative(double A, double B,
                           double maxRelativeError, 
                           double maxAbsoluteError)

  {
    if (fabs(A - B) < maxAbsoluteError)
      return true;
    double relativeError;
    if (fabs(B) > fabs(A))
      relativeError = fabs((A - B) / B);
    else
      relativeError = fabs((A - B) / A);
    if (relativeError <= maxRelativeError)
      return true;
    return false;
  }

  struct PointAngle
  {
    Point p;
    double cos_theta;
    PointAngle(Point p, double cos_theta): 
      p(p), cos_theta(cos_theta)
    { }
    bool operator<(const PointAngle &b) const
    {
      //Two points that are equal might have very different
      //angles. i.e. 0 and 2PI. Because of this two points that are
      //almost equal might also have very different angles (0.001 and
      //2PI-.0001), so we might end up with duplicate points.
      return cos_theta < b.cos_theta;
    }
  };

  //if the points in cap form a convex planar polygon, but are out
  //of order, a radial sort will make it a legit polygon again.
  void radialSort(Polygon &cap)
  {
    //2 or less elements are already radially sorted (if you ignore
    //the winding order).
    if (cap.size() < 3)
      return;

    Point center = getCenter(cap);

    //TODO: Optimize by replacing set with static vector + sort + unique.

    //Rather than store angle, we optimize by using costheta.  This
    //lets us avoid calling expensive acos.
    set<PointAngle> pointAngles;
    pointAngles.insert(PointAngle(cap[0], 1));

    const Point base_vec = (cap[0] - center).normal();

    const Point base_direction = getFaceNormal(cap);

    for (size_t i=1; i < cap.size(); ++i) {
      if (!(cap[i] == cap[0])) {
        const Point vec = (cap[i] - center).normal();
        double costheta = Dot(base_vec, vec);
        //costheta only defined [-1,1]. Precision issues can make it
        //slightly outside this.
        if (costheta > 1)
          costheta = 1;
        else if (costheta < -1)
          costheta = -1;
//         double angle = acos(costheta);

        const Point direction = Cross(base_vec, vec);
        const double sameDirection = Dot(base_direction, direction);
        if (sameDirection < 0) {
          costheta = 2-costheta;
//           angle = M_PI*2 - angle;
        }
        pointAngles.insert(PointAngle(cap[i], costheta));
      }
    }

    cap.clear();
    int i=0;
    for (set<PointAngle>::iterator iter = pointAngles.begin();
         iter != pointAngles.end(); ++iter, ++i) {
      bool okToAdd = true;

      //ouch, this is expensive. Should check and see whether there's
      //a faster way.
      for (size_t j=0; j < cap.size(); ++j) {
        if (almostEqualPoints(iter->p, cap[j])) {
          okToAdd = false;
          break;
        }
      }
      if (okToAdd)
        cap.push_back(iter->p);
    }
  }

  bool verifyUnique(const Polygon &face)
  {
    set<Point, ltPoint> face_set(face.begin(), face.end());
    if (face.size() != face_set.size()) {
      cout << face.size()  << " vs " <<face_set.size()<<endl;
      exit(1);
    }
    return true;
  }

  void printPolygon(const Polygon &face)
  {
    for (size_t i=0; i < face.size(); ++i) {
      cout <<"v " <<face[i]<<endl;
    }
    for (size_t i=0; i < face.size()-2; ++i) {
    cout << "f " << 1<< " " << i+2 << " " <<i+3 <<endl;
    }
  }

  bool isDegenerateFace(const Polygon &face)
  {
    //if the three points are each off by 1e-x, then the area will be
    //of magnitude 1e-2x. 
    double area=0;
    for (size_t i=2; i < face.size(); ++i) {
      area += getTriangleArea(face[0], face[i-1], face[i]);
    }
    return area < 1e-12; //This would work better if I could compare
    //with the areas of the other faces. (relative
    //compare)
  }

  bool overlaps(const Polygon &face1, const Polygon &face2, 
                float &intersectedArea, bool forceIntersectionAreaComputation)
  {
    Point normal1 = getOrderedFaceNormal(face1);
    Point normal2 = getOrderedFaceNormal(face2);
    double normalDot = Dot(normal1, normal2);
    if (fabs(normalDot) < 0.9999) {
#ifdef BUILD_DEBUG
      if (fabs(normalDot) > .9999)
        cout<<"normals are not parallel: " <<normal1 << " and " <<normal2<<endl;
#endif
      return false;
    }

    if (normalDot < 0)
      normal2*=-1;

    Point center1 = getCenter(face1);
    Point center2 = getCenter(face2);

    if (!forceIntersectionAreaComputation &&
        almostEqualPoints(center1, center2)) {
//       cout <<"almost same centers: " << center1 << " and "<< center2<<endl;
      return true;
    }

    Point averageNormal = 0.5*(normal1+normal2);
    double d1 = Dot(averageNormal, -center1);
    double d2 = Dot(averageNormal, -center2);
    if (!almostEqualRelative(d1, d2, 5e-6)) {
#ifdef BUILD_DEBUG
      //I'm curious if it would have made it...
      if (almostEqualRelative(d1, d2, 1e-4)) {
        AffineTransformT<double> transform;
        transform.initWithRotation(averageNormal, Point(0,0,1));

        vector<Point2D> face1_t, face2_t;
        for (size_t i=0; i < face1.size(); ++i) {
          Point v_t = transform.multiply_point(face1[i]);
          face1_t.push_back(Point2D(v_t[0], v_t[1]));
        }
        for (size_t i=0; i < face2.size(); ++i) {
          Point v_t = transform.multiply_point(face2[i]);
          face2_t.push_back(Point2D(v_t[0], v_t[1]));
        }

        if (normalDot < 0)
          reverse(face2_t.begin(), face2_t.end());

        intersectedArea = PolygonIntersect::intersectionArea(face1_t, face2_t);

        float area1 = getArea(face1);
        float area2 = getArea(face2);

        float areaRatio = intersectedArea/Min(area1, area2);
    
        if (areaRatio > .8 ) {
          cout <<"normalDot was: " <<normalDot<<endl;
          cout <<"distances differ: " <<d1 <<" and "<<d2
               <<" relative: " << ((d1-d2)/d2)<<endl;
        }
      }
#endif
      return false;
    }

    //they are pretty much on the same plane. Now lets find out if
    //they overlap at all.
    AffineTransformT<double> transform;
    transform.initWithRotation(averageNormal, Point(0,0,1));

    vector<Point2D> face1_t, face2_t;
    for (size_t i=0; i < face1.size(); ++i) {
      Point v_t = transform.multiply_point(face1[i]);
      face1_t.push_back(Point2D(v_t[0], v_t[1]));
    }
    for (size_t i=0; i < face2.size(); ++i) {
      Point v_t = transform.multiply_point(face2[i]);
      face2_t.push_back(Point2D(v_t[0], v_t[1]));
    }

    if (normalDot < 0)
      reverse(face2_t.begin(), face2_t.end());

    intersectedArea = PolygonIntersect::intersectionArea(face1_t, face2_t);

    float area1 = getArea(face1);
    float area2 = getArea(face2);

//     cout <<"overlap area is: " << area<<endl;
//     cout <<"face 1 area is:  " <<area1<<endl;
//     cout <<"face 2 area is:  " <<area2<<endl;

    float areaRatio = intersectedArea/Min(area1, area2);
    
//     cout <<"distance: " <<(d1-d2)/d1<<endl;
#ifdef BUILD_DEBUG
    if (areaRatio > 1e-2 && areaRatio < .8)
      cout <<"area ratio: "<<areaRatio << endl;
#endif
//     exit(1);
    if (areaRatio > .2 ) return true;
    else return false;

  }

  bool isSubset(const Polygon &smallFace, const Polygon &bigFace)
  {
    return overlaps(smallFace, bigFace);
  }

  Point getTriangleNormal(const Point &p0, const Point &p1,
                          const Point &p2)
  {
    const Point edge1 = p1 - p0; 
    const Point edge2 = p2 - p0; 
    return Cross(edge1, edge2);
  }

  //This tries to return a normalized "high quality" normal for the
  //convex face.
  Point getFaceNormal(const Polygon& face)
  {
    //Assume that the vertices in the face are not ordered. The normal
    //would be defined by the cross product of the two main principal
    //components (wouldn't that be the third componenent?). But doing
    //PCA is a lot of work (both for the computer and for the
    //programmer).

    //find centroid
    Point center = getCenter(face);

    //If the vertices do not define a convex polygon, then the center
    //and face[0] might end up lying on the same point (or very close
    //to it). It is for this reason that we require face to be a
    //convex poygon.  If the vertices are not planar (i.e. we use all
    //the vertices of the polytope), then base_vec might point in the
    //direction of the normal, which would obviously prevent us from
    //evee returning the correct normal.

    //But what we can try is to find the vertex furthest from the
    //center. This would give a fairly good base_vec.
    Point base_vec;
    double biggest=0;
    for (size_t i=1; i < face.size(); ++i) {
      const Point vec = (face[i] - center);
      double length = vec.length2();
      if (length > biggest) {
        base_vec = vec;
        biggest = length;
      }
    }
    
    Point bestNormal;
    biggest=0;
    for (size_t i=1; i < face.size(); ++i) {
      const Point vec = (face[i] - center);
      Point normal = Cross(base_vec, vec);
      double length = normal.length2();
      if (length > biggest) {
        bestNormal = normal;
        biggest = length;
      }
    }

    //If the face is degenerate, biggest will still be 0.
    if (biggest == 0)
      return Point(0,0,0);
    else
      return bestNormal / Sqrt(biggest);
  }

  Point getCenter(const Polygon& vertices)
  {
    Point center(0,0,0);
    for (size_t i=0; i < vertices.size(); ++i)
      center += vertices[i];
    center /= vertices.size();
    return center;
  }

  Point getOrderedFaceNormal(const Polygon& face)
  {
    //adapted from http://tog.acm.org/GraphicsGems/gemsiii/newell.c
    //This assumes the vertices are in the correct order.
    if (face.size() < 3)
      return Point (0,0,0);

    Point u, v;
    Point normal(0,0,0);
    for (size_t i=0; i < face.size(); ++i) {
      u = face[i];
      v = face[(i+1) % face.size()];
      normal[0] += (u[1]-v[1]) * (u[2]+v[2]);
      normal[1] += (u[2]-v[2]) * (u[0]+v[0]);
      normal[2] += (u[0]-v[0]) * (u[1]+v[1]);
    }
    /* normalize the polygon normal to obtain the first
       three coefficients of the plane equation
    */
    return normal.normal();
  }

  double getArea(const Polygon& face)
  {
    //TODO: look at http://jgt.akpeters.com/papers/Sunday02/FastArea.html
    double area=0;
    for (size_t i=2; i < face.size(); ++i) {
      area += getTriangleArea(face[0], face[i-1], face[i]);
    }
    return area;
  }

  double getArea2D(vector<Point2D>& face)
  {
    //This comes from sunday's JGT paper on newell normal...
   // guarantee the first two vertices are also at array end
    face.push_back(face[0]);
    face.push_back(face[1]);

    double sum = 0.0;
    //    double *xptr = x+1, *ylow = y, *yhigh = y+2;
    for (size_t i=1; i <= face.size()-2; i++) {
      //sum += (*xptr++) * ( (*yhigh++) - (*ylow++) );
      sum += face[i][0] * (face[i+1][1]-face[i-1][1]);
    }
    face.resize(face.size()-2);
    return 0.5*sum;
  }

  double getTriangleArea(const Point &p0, const Point &p1,
                         const Point &p2)
  {
    return 0.5 * getTriangleNormal(p0, p1, p2).length();
  }

  void getOrthonormalBasis(const Point &w, Point &u, Point &v)
  {
    //From Hughes and Moller: Building an Orthonormal Basis from a
    //Unit Vector (in JGT).
    const double x = fabs(w[0]);
    const double y = fabs(w[1]);
    const double z = fabs(w[2]);
    if (x <= y && x <= z)
      u = Point(0, -w[2], w[1]);
    else if (y <= x && y <= z)
      u = Point(-w[2], 0, w[0]);
    else //if (z <= x && z <= y)
      u = Point(-w[1], w[0], 0);
    u = u.normal();
    v = Cross(w, u); //this is normalized if w is normalized.
  }
}
}
