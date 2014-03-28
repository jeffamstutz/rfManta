#include <Core/Geometry/VectorT.h>
#include <Model/Groups/BSP/Polytope.h>
#include <Model/Groups/BSP/BSP.h>
#include <Model/Groups/BSP/aip.h>
#include <Model/Groups/BSP/chainHull.h>
#include <algorithm>
#include <map>
#include <iostream> //for debugging

namespace Manta {

  using namespace BSPs;

  Polytope::Polytope() : debugPrint(false){
  }

  Polytope::~Polytope() {
  }

  void Polytope::printAsObj(int offset) const
  {
    cout <<endl;
    for (size_t f=0; f < faces.size(); ++f) {
      for (size_t v=0; v < faces[f].size(); ++v)
        cout << "v " << faces[f][v] << "\n";
    }

    int v_count = 1;
    for (size_t f=0; f < faces.size(); ++f) {
      int v_start = v_count + offset;
      cout <<endl;
      for (size_t i=2; i < faces[f].size(); ++i) {
        cout <<"f " << v_start << " " <<v_start+i-1 << " " <<v_start+i<<endl;
      }
      v_count += faces[f].size();
    }

    cout <<endl;
  }

  void Polytope::printAsPly()
  {
    int numVertex = 0;
    for (size_t f=0; f < faces.size(); ++f)
      numVertex += faces[f].size();

    cout <<endl;
    cout <<"element vertex " << numVertex <<endl;
    cout <<"element face " << faces.size() <<endl;
    cout <<"property list uchar int vertex_indices" <<endl;
    cout <<"end_header\n";
    for (size_t f=0; f < faces.size(); ++f) {
      for (size_t v=0; v < faces[f].size(); ++v)
//         cout << "v " 
             cout<< faces[f][v]<<endl;
    }

    int v_count = 0;
    for (size_t f=0; f < faces.size(); ++f) {
      cout << "\n"<< faces[f].size()<< " ";
      for (size_t v=0; v < faces[f].size(); ++v)
        cout << v_count++ << " ";
    }
    cout <<endl;
  }


  void Polytope::clear()
  {
    faces.clear();
  }

  void Polytope::initialize(const BBox &bounds)
  {
    clear();
    faces.resize(6);

    faces[0].push_back(Point(bounds[0][0], bounds[0][1], bounds[0][2]));
    faces[0].push_back(Point(bounds[0][0], bounds[0][1], bounds[1][2]));
    faces[0].push_back(Point(bounds[0][0], bounds[1][1], bounds[1][2]));
    faces[0].push_back(Point(bounds[0][0], bounds[1][1], bounds[0][2]));

    faces[1].push_back(Point(bounds[0][0], bounds[0][1], bounds[0][2]));
    faces[1].push_back(Point(bounds[0][0], bounds[1][1], bounds[0][2]));
    faces[1].push_back(Point(bounds[1][0], bounds[1][1], bounds[0][2]));
    faces[1].push_back(Point(bounds[1][0], bounds[0][1], bounds[0][2]));

    faces[2].push_back(Point(bounds[0][0], bounds[0][1], bounds[0][2]));
    faces[2].push_back(Point(bounds[0][0], bounds[0][1], bounds[1][2]));
    faces[2].push_back(Point(bounds[1][0], bounds[0][1], bounds[1][2]));
    faces[2].push_back(Point(bounds[1][0], bounds[0][1], bounds[0][2]));

    faces[3].push_back(Point(bounds[1][0], bounds[0][1], bounds[0][2]));
    faces[3].push_back(Point(bounds[1][0], bounds[0][1], bounds[1][2]));
    faces[3].push_back(Point(bounds[1][0], bounds[1][1], bounds[1][2]));
    faces[3].push_back(Point(bounds[1][0], bounds[1][1], bounds[0][2]));

    faces[4].push_back(Point(bounds[0][0], bounds[0][1], bounds[1][2]));
    faces[4].push_back(Point(bounds[0][0], bounds[1][1], bounds[1][2]));
    faces[4].push_back(Point(bounds[1][0], bounds[1][1], bounds[1][2]));
    faces[4].push_back(Point(bounds[1][0], bounds[0][1], bounds[1][2]));

    faces[5].push_back(Point(bounds[0][0], bounds[1][1], bounds[0][2]));
    faces[5].push_back(Point(bounds[0][0], bounds[1][1], bounds[1][2]));
    faces[5].push_back(Point(bounds[1][0], bounds[1][1], bounds[1][2]));
    faces[5].push_back(Point(bounds[1][0], bounds[1][1], bounds[0][2]));

    //In case we have a flat BBox, we need to remove degenerate faces.
    cleanup();

  }
  
  bool Polytope::verifyWindingOrder() const
  {
    for (size_t f=0; f < faces.size(); ++f) {
      const Point v1 = getOrderedFaceNormal(faces[f]);
      for (size_t i=2; i < faces[f].size(); ++i) {
        const Point v2 = getTriangleNormal(faces[f][0], faces[f][i-1], faces[f][i]).normal();
        if (Dot(v1, v2) < .98) { // really it should be <1
//           if (getTriangleArea(faces[f][0], faces[f][i-1], faces[f][i]) < 1e-7)
//             continue;
          cout << "winding order is jacked!"<<endl;
          cout <<"face: "<<f<<endl;
          cout << v1 << "\tvs\t"<<v2<<endl;
          cout <<Dot(v1, v2)<<endl;

          for (size_t i=2; i < faces[f].size(); ++i) {
            cout <<" normal: "<< getTriangleNormal(faces[f][0], faces[f][i-1], faces[f][i]) <<endl;
          }

          printAsObj();
//           exit(1);
          return false;
        }
      }
    }
    return true;
  }

  bool Polytope::hasSimilarFace(const Polytope &poly, const Polygon &cap) const
  {
    //We assume that if cap contains a face of the poly, then
    //they are the same. This handles situations where the cap contains extra
    //(redundant) vertices that came from different faces.
    //It should not be possible to have a cap with less vertices and
    //still be equal because those extra points would have been added
    //to the cap.

    for (size_t face=0; face < poly.faces.size(); ++face) {
#ifdef BUILD_DEBUG
      const size_t polySize = poly.faces[face].size();
      if (polySize <= cap.size()) {
        if (isSubset(poly.faces[face], cap)) {

          if (polySize < cap.size()) {
            cout <<"there were less.\n";
//             for (size_t i =0; i < poly.faces[face].size(); ++i)
//               cout <<poly.faces[face][i]<<endl;
//             cout <<endl;
//             for (size_t i =0; i < cap.size(); ++i)
//               cout <<cap[i]<<endl;
//             poly.printAsObj();
//             //               exit(1);
//             debugPrint = true;
          }

//           return true;
        }
      }
      else
        if (isSubset(cap, poly.faces[face])) {
          if (polySize > cap.size()) {
            cout <<"there were more\n";
            // exit(1);
          }
//           return true;
        }
#endif
      if (overlaps(poly.faces[face], cap))
        return true;
    }
    return false;
  }

  bool Polytope::findSplitEdge(PointDistMap &pointDists, const Polygon &face) const
  {
    const double MIN_DIST = 2e-6;

    int v1 = -1,
        v2 = -1;

    //TODO: This can probably be heavily optimized. Should do that.
    for (size_t i1=0; i1 < face.size()-1; ++i1) {
      PointDist &i1distOnPlane = pointDists.find(face[i1])->second;
      if (fabs(i1distOnPlane.dist) >= MIN_DIST || i1distOnPlane.offPlane)
        continue;
      for (size_t i2=i1+1; i2 < face.size(); ++i2) {
        PointDist &i2distOnPlane = pointDists.find(face[i2])->second;
        if (fabs(i2distOnPlane.dist) >= MIN_DIST || i2distOnPlane.offPlane)
          continue;
        
        //We now have two vertices i1 and i2 that are supposed to be
        //on the plane. Now lets find out if the edge formed by i1 and
        //i2 cleanly divides the set of vertices into those that are
        //above and those that are below the plane.
        //TODO: How do we handle dist==0? Should we handle that
        //specifically?
        double insideSign=0;
        size_t i;
        for (i=i1+1; i < i2; ++i) {
          const PointDist &distOnPlane = pointDists.find(face[i])->second;
          if (insideSign == 0)
            insideSign = distOnPlane.dist;
          else if (insideSign*distOnPlane.dist < 0)
            break; //signs disagree
        }
        if (i != i2) {//did they all agree?
          continue;  //no.
        }
        
        insideSign = 0;
        for (i=(i2+1)%face.size(); i!=i1; i=(i+1)%face.size()) {
          const PointDist &distOnPlane = pointDists.find(face[i])->second;
          if (insideSign == 0)
            insideSign = distOnPlane.dist;
          else if (insideSign*distOnPlane.dist < 0)
            break; //signs disagree
        }
        if (i != i1) { //did they all agree?
          continue;  //no.
        }

        if (v1<0 ||
            (fabs(pointDists.find(face[i1])->second.dist) + fabs(pointDists.find(face[i2])->second.dist))
            <(fabs(pointDists.find(face[v1])->second.dist) + fabs(pointDists.find(face[v2])->second.dist))) {
          //We found our edges!
          v1 = i1;
          v2 = i2;
        }
      }
    }
    if (v1 >= 0) {
      //cout <<"using "<<v1<<" and "<<v2<<endl;
      //i1distOnPlane.dist = 0;
      //i2distOnPlane.dist = 0;
      //Set the other vertices as being off the plane.
      for (int i=0; i < static_cast<int>(face.size()); ++i) {
        if (i != v1 && i != v2) {
          PointDist &distOnPlane = pointDists.find(face[i])->second;
          distOnPlane.offPlane = true;
        }
      }
      return true;
    }
    
    //Found no edge.
    v1=-1;
    for (size_t i=0; i < face.size(); ++i) {
      PointDist &distOnPlane = pointDists.find(face[i])->second;
      if (v1 < 0 && fabs(distOnPlane.dist) < MIN_DIST && !distOnPlane.offPlane) {
        distOnPlane.dist=0;
        v1 = i;
      }
      else 
//         perhaps, it should be all the points are ON the plane...?
        distOnPlane.offPlane = true;
    }
    return false;
  }

  void Polytope::findPointsOnPlane(const BuildSplitPlane& plane,
                                   PointDistMap &pointDists) const
  {
    //TODO: change from using stl::map to something based on vector.

    bool debug = false;

    const double MIN_DIST = 2e-6;
    for (size_t f=0; f < faces.size(); ++f) {
      unsigned int pointsOnPlane = 0;
      unsigned int pointsAlmostOnPlane = 0;
      for (size_t i=0; i < faces[f].size(); ++i) {
        const double dist = signedDistance(plane, faces[f][i]);

        const PointDist &distOnPlane = 
          pointDists.insert(make_pair(faces[f][i],
                                      PointDist(dist, false))).first->second;
        if (fabs(distOnPlane.dist) < MIN_DIST && !distOnPlane.offPlane)
          ++pointsOnPlane;
        //TODO: VERY IMPORTANT: include plane normal so that big
        //planes are treated fairly (otherwise, the 3*MIN_DIST thing
        //is too restrictive).
        else if (fabs(distOnPlane.dist) < 3*MIN_DIST && !distOnPlane.offPlane) 
          ++pointsAlmostOnPlane;
      }
      if (pointsOnPlane > 2 &&
          pointsOnPlane + pointsAlmostOnPlane < faces[f].size()) {

#ifdef BUILD_DEBUG
        cout <<"looks like we need to minimize face "<<  f <<" ! " 
             <<pointsOnPlane << " + "<<pointsAlmostOnPlane<<" < "
             <<faces[f].size() <<"\n";

        for (size_t i=0; i < faces[f].size(); ++i) {
          cout << "  i: " <<signedDistance(plane, faces[f][i]) <<endl;
        }
#endif
        debug = true;
        
        findSplitEdge(pointDists, faces[f]);
      }
    }

    //Now we need to figure out which points can be placed exactly on the plane.
    for (size_t f=0; f < faces.size(); ++f) {
      unsigned int pointsOnPlane = 0;
      unsigned int pointsAlmostOnPlane = 0;
      for (size_t i=0; i < faces[f].size(); ++i) {
        const PointDist &distOnPlane = pointDists.find(faces[f][i])->second;
        if (fabs(distOnPlane.dist) < MIN_DIST && !distOnPlane.offPlane)
          ++pointsOnPlane;
        else if (fabs(distOnPlane.dist) < 3*MIN_DIST && !distOnPlane.offPlane)
          ++pointsAlmostOnPlane;
      }

#ifdef BUILD_DEBUG
      if (debug) {
        cout <<"face " << f << " has: " <<pointsOnPlane<< " + "
             <<pointsAlmostOnPlane<<" < "<<faces[f].size()<<endl;       
      }
#endif
      //all on plane
      if (pointsOnPlane >= 3 && 
          pointsOnPlane+pointsAlmostOnPlane == faces[f].size())
        for (size_t i=0; i < faces[f].size(); ++i) {
//           cout <<"1: " <<f<<" " <<i<<"\t"<<pointDists.find(faces[f][i])->second.distUlps
//                <<"and "<<pointDists.find(faces[f][i])->second.dist<<endl;
          pointDists.find(faces[f][i])->second.dist = 0;
        }
      //an edge or just a corner
      else if (pointsOnPlane > 0 && pointsOnPlane < 3)
        for (size_t i=0; i < faces[f].size(); ++i) {
          PointDist &distOnPlane = pointDists.find(faces[f][i])->second;
//           cout <<"2: " <<f<<" " <<i<<"\t"<<pointDists.find(faces[f][i])->second.distUlps
//                <<"and "<<pointDists.find(faces[f][i])->second.dist<<endl;
          if (fabs(distOnPlane.dist) < MIN_DIST && !distOnPlane.offPlane)
            distOnPlane.dist = 0;
        }
      //partial face on plane (bad!)
      else if (pointsOnPlane > 2) {
#ifdef BUILD_DEBUG
        cout.precision(20);
        cout <<f<<" oops, partial face on plane! " <<pointsOnPlane<< " + "
             <<pointsAlmostOnPlane<<" < "<<faces[f].size()<<endl;
        
        cout <<f<<"out of: " <<faces.size()<<endl;
        for (size_t i=0; i < faces[f].size(); ++i) {
//           cout << i<<": " <<signedUlpsDistance(plane, faces[f][i]);
//                <<" vs " <<pointDists.find(faces[f][i])->second.distUlps<<"\t";
          cout << i<<": " <<signedDistance(plane, faces[f][i])
               <<" vs " <<pointDists.find(faces[f][i])->second.dist
               <<"offPlane: "<<pointDists.find(faces[f][i])->second.offPlane<<endl;
        }
          printAsObj();
          cout << "plane " << plane.normal<< "  " <<plane.d<<endl;
#endif

          //exit(1);
          //debugPrint = true;//exit(1);
      }
      else {
//         cout <<pointsOnPlane<<" and " <<pointsAlmostOnPlane<<endl;
//         for (size_t i=0; i < faces[f].size(); ++i)
//           cout <<"3: " <<f<<" " <<i<<"\t"<<pointDists.find(faces[f][i])->second.distUlps
//                <<"and "<<pointDists.find(faces[f][i])->second.dist<<endl;
      }
    }
  }

  void Polytope::split(const BuildSplitPlane &plane, Polytope &side1, Polytope &side2) const
  {
    PointDistMap pointDists;
    findPointsOnPlane(plane, pointDists);

    //We are making these Polygons (really stl::vectors) static so
    //that the vector doesn't allocate/deallocate memory all the
    //time. Of course this would need to be changed into a some sort
    //of pool if we end up threading this code.
    // static 
      Polygon cap;
    cap.clear();

    for (size_t f=0; f < faces.size(); ++f) {
#ifdef BUILD_DEBUG
      verifyUnique(faces[f]);
#endif
      static Polygon face1, face2;
      face1.clear();
      face2.clear();
      splitPolygon(pointDists, plane, faces[f], face1, face2, cap);

      if (face1.size() > 2)
        side1.faces.push_back(face1);
      if (face2.size() > 2) {
        side2.faces.push_back(face2);
      }
    }

    //radially sort points in cap.
    
    //It's possible to get no edge. Imagine a split plane that only
    //touches an edge or a vertex of the polytope.
    if (true) {

      radialSort(cap);
      //A radial sort removes duplicate points using several
      //comparison methods. Since we then need to compare the cap with
      //these faces to see if they are similar, we perform a radial
      //sort on these faces as well so that if they are similar, they
      //will show it.


      if (isDegenerateFace(cap))
        cap.clear();

//     side1.verifyWindingOrder();
    Polytope side1Copy = side1;
      for (size_t f=0; f < side1.faces.size(); ++f) {
        radialSort(side1.faces[f]);
      }
#ifdef BUILD_DEBUG
      if (!side1.verifyWindingOrder()) {
        printAsObj();
        
        for (size_t f=0; f < side2.faces.size(); ++f) {
          radialSort(side2.faces[f]);
        }
        side2.printAsObj();
        exit(1);
      }
#endif
      for (size_t f=0; f < side2.faces.size(); ++f) {
        radialSort(side2.faces[f]);
      }

      if (cap.size() > 2) {

        side1.cleanup();
        side2.cleanup();
//         we want to remove the face which is larger or smaller? 
        if (!hasSimilarFace(side1, cap))
          side1.faces.push_back(cap);
        else {
//           cout <<"cap1:\n";
//           printPolygon(cap);
//           for (size_t i=0; i < cap.size(); ++i)
//             cout <<i<<" has dist of:" <<signedUlpsDistance(plane, cap[i])<<" \t"<< signedDistance(plane, cap[i])<<endl;
//           for (size_t f=0; f < faces.size(); ++f)
//             for (size_t i=0; i < faces[f].size(); ++i)
//               cout <<f << " " <<i<<" has dist of:" <<signedUlpsDistance(plane, faces[f][i])<<" \t" <<signedDistance(plane, faces[f][i])<<endl;

        }
        if (!hasSimilarFace(side2, cap))
          side2.faces.push_back(cap);
        else {
//           cout <<"cap2:\n";
//           printPolygon(cap);
        }
      }
      else {
//         size_t oldsize = side1.faces.size();
        side1.cleanup();
//         if (oldsize != side1.faces.size()) {
//           cout <<oldsize << "  " <<side1.faces.size()<<endl;
//           exit(1);
//         }
//         oldsize = side2.faces.size();
        side2.cleanup();
//         if (oldsize != side2.faces.size()) {
//           cout <<oldsize << "  " <<side2.faces.size()<<endl;
//           exit(1);
//         }
      }
    }

#ifdef BUILD_DEBUG
    side1.verifyWindingOrder();
    side2.verifyWindingOrder();
#endif
  }


  void Polytope::splitPolygon(const PointDistMap &pointDists,
                              const BuildSplitPlane &plane, const Polygon &in,
                              Polygon &side1, Polygon &side2, Polygon &cap) const
  {
    //This is the Reentrant Polygon Clipping algorithm by Sutherland
    //and Hodgman.

    Point s, f;
    for (size_t i=0; i < in.size(); ++i) {
      const Point &p = in[i];

      const PointDist &distOnPlane = pointDists.find(p)->second;
      double p_d = distOnPlane.dist;
      if (i==0) { //first point?
        s = f = p;
      }
      else {
        const PointDist &distOnPlane = pointDists.find(s)->second;
        double s_d = distOnPlane.dist;

        //if s_d and p_d are supposed to be 0, but aren't, then we
        //might end up seeing that as an intersection if the signs
        //differ. This will result in the cap having two points which
        //are very similar.

        if (s_d*p_d < 0) {  //does line sp cross dividing plane?
          //compute intersection I, of sp and plane.
          const double length1 = fabs(p_d);
          const double length2 = fabs(s_d);

          const double a = length1 / (length1 + length2);

          Point I;
          //This version will give more consistent results, but I
          //don't know if it buys us anything really useful.
          if (a > .5)
            I = p + a*(s-p);
          else 
            I = s + (1-a)*(p-s);

          side1.push_back(I);
          side2.push_back(I);
          cap.push_back(I);
        }
      }

      //How does p relate to plane?
      if (p_d == 0) { //on the plane
        side1.push_back(p);
        side2.push_back(p);

        //Note: if the polygon is on the plane then we will end up
        //with side1, side2, and cap all being copies of the polygon.
        //Since cap is then added to side1 and side2, both sides will
        //now have two copies of the polygon and double the area. If
        //this repeats, the number of polygons and of course the
        //areas, will double each time, which can be bad!.
        cap.push_back(p);
      }
      else if (p_d < 0)
        side1.push_back(p);
      else if (p_d > 0) {
        side2.push_back(p);
      }
      s=p;
    }

    //now we need to close the two polygons.
    //TODO: once we know it works, do this closing as part of the for
    //loop.
    const Point &p = f;

    const PointDist &distPOnPlane = pointDists.find(p)->second;
    double p_d = distPOnPlane.dist;
    const PointDist &distSOnPlane = pointDists.find(s)->second;
    double s_d = distSOnPlane.dist;

    if (s_d*p_d < 0) {  //does line sp cross dividing plane?
      //compute intersection I, of sp and plane.
      const double length1 = fabs(p_d);
      const double length2 = fabs(s_d);

      const double a = length1 / (length1 + length2);
      Point I;

      //This version will give more consistent results, but I
      //don't know if it buys us anything really useful.
      if (a > .5)
        I = p + a*(s-p);
      else 
        I = s + (1-a)*(p-s);

      side1.push_back(I);
      side2.push_back(I);
      cap.push_back(I);
    }
  }

  void Polytope::cleanup()
  {
    for (vector<Polygon>::iterator iter=faces.begin(); iter!=faces.end(); ++iter) {
//       cout <<"testing face\n";
      if (isDegenerateFace(*iter)) {
        iter = faces.erase(iter);
        if (faces.empty())
          break;
        if (iter != faces.begin())
          --iter;
      }

      //TODO: remove colinear vertices.

      //if dot product of two adjacent edges is >0.9999, then remove
      //middle vertex.

    }
  }

  bool Polytope::isFlat() const {
    //Determine whether all the vertices lie on the plane determined
    //by one of the faces.

//      return faces.size() < 4;

    //This is a very simple safe test (assuming nothing bad happened!)
    if (faces.size() < 1)
      return true;

    //We are making this Polygon (really stl::vectors) static so
    //that the vector doesn't allocate/deallocate memory all the
    //time. Of course this would need to be changed into a some sort
    //of pool if we end up threading this code.
    static Polygon vertices;
    vertices.clear();
    for (size_t f=0; f < faces.size(); ++f) {
      vertices.insert(vertices.end(), faces[f].begin(),faces[f].end());
    }
    
    Point normal = getFaceNormal(vertices);

    Point u, v;
    getOrthonormalBasis(normal, u, v);

    static vector<Point2D> vertices_t;
    vertices_t.clear();
    for (size_t i=0; i < vertices.size(); ++i) {
      Point2D v_t(Dot(vertices[i], u), Dot(vertices[i], v));
      vertices_t.push_back(v_t);
    }

    static vector<Point2D> hull;
    convexHull(vertices_t, hull);
    
    const float flattenedArea = getArea2D(hull);

    double area = 0;
    for (size_t f=0; f < faces.size(); ++f) {
      for (size_t i=2; i < faces[f].size(); ++i) {
        area += getTriangleArea(faces[f][0], faces[f][i-1], faces[f][i]);
      }
    }
    
//     cout <<flattenedArea <<" > " <<0.5*area<<endl;

    return flattenedArea >= 0.4999*area;
  }


  float Polytope::getSurfaceArea()
  {
    //TODO: if this function becomes expensive, store the area in each
    //face and then only recompute when the face has been broken up.
    double area = 0;
    for (size_t f=0; f < faces.size(); ++f) {
      for (size_t i=2; i < faces[f].size(); ++i) {
        area += getTriangleArea(faces[f][0], faces[f][i-1], faces[f][i]);
      }
    }

    if (isFlat())
      return area*2;
    else
      return area;
  }

  void Polytope::updateVertices()
  {
    vertices.clear();
    set<Point, ltPoint> vertices_set;
    for (size_t f=0; f < faces.size(); ++f)
      for (size_t i=0; i < faces[f].size(); ++i)
        vertices_set.insert(faces[f][i]);
    vertices.insert(vertices.begin(), vertices_set.begin(), vertices_set.end());
  }

};
