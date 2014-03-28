#include <Model/Primitives/QuadFacedHexahedron.h>
#include <Core/Exceptions/BadPrimitive.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>

#include <iostream>
#include <sstream>

using namespace Manta;

QuadFacedHexahedron::QuadFacedHexahedron(Material *matl,
                                         const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3,
                                         const Vector& v4, const Vector& v5, const Vector& v6, const Vector& v7,
                                         bool coplanar_test)
  : PrimitiveCommon(matl)
{
  // Save the lower left front corner, and the upper right back
  // corner; these will serve as the anchor points for the six planes
  // bounding the hex.
  corner[0] = v0;
  corner[1] = v6;

  // Compute the plane normals.  
  normal[Bottom] = Cross(v3 - v0, v1 - v0).normal();
  normal[Left]   = Cross(v4 - v0, v3 - v0).normal();
  normal[Front]  = Cross(v1 - v0, v4 - v0).normal();
  normal[Right]  = Cross(v5 - v6, v2 - v6).normal();
  normal[Back]   = Cross(v2 - v6, v7 - v6).normal();
  normal[Top]    = Cross(v7 - v6, v5 - v6).normal();

  // Test for coplanarity in each face.
  if(coplanar_test){
    bool ok = true;
    std::stringstream s;
    if(!coplanar(corner[0], normal[Bottom], v0, v1, v2, v3)){
      s << "QuadFacedHexahedron: bottom face "
        << "(v0 = " << v0
        << ", v1 = " << v1
        << ", v2 = " << v2
        << ", v3 = " << v3 << ") is not co-planar";
      ok = false;
    }
    if(!coplanar(corner[0], normal[Left], v0, v3, v7, v4)){
      s << "QuadFacedHexahedron: left face "
        << "(v0 = " << v0
        << ", v3 = " << v3
        << ", v7 = " << v7
        << ", v4 = " << v4 << ") is not co-planar";
      ok = false;
    }
    if(!coplanar(corner[0], normal[Front], v0, v1, v5, v4)){
      s << "QuadFacedHexahedron: front face "
        << "(v0 = " << v0
        << ", v1 = " << v1
        << ", v5 = " << v5
        << ", v4 = " << v4 << ") is not co-planar";
      ok = false;
    }
    if(!coplanar(corner[1], normal[Top], v4, v5, v6, v7)){
      s << "QuadFacedHexahedron: top face "
        << "(v4 = " << v4
        << ", v5 = " << v5
        << ", v6 = " << v6
        << ", v7 = " << v7 << ") is not co-planar";
      ok = false;
    }
    if(!coplanar(corner[1], normal[Right], v5, v1, v2, v6)){
      s << "QuadFacedHexahedron: right face "
        << "(v5 = " << v5
        << ", v1 = " << v1
        << ", v2 = " << v2
        << ", v6 = " << v6 << ") is not co-planar";
      ok = false;
    }
    if(!coplanar(corner[1], normal[Back], v3, v2, v6, v7)){
      s << "QuadFacedHexahedron: back face "
        << "(v3 = " << v3
        << ", v2 = " << v2
        << ", v6 = " << v6
        << ", v7 = " << v7 << ") is not co-planar";
      ok = false;
    }

    if(!ok)
      throw BadPrimitive(s.str());
  }
}

void QuadFacedHexahedron::computeBounds(const PreprocessContext& context,
                                        BBox& bbox) const {
  Vector v[8];
  v[0] = corner[0];

  v[1] = intersectThreePlanes(corner[1], normal[Right],
                              corner[0], normal[Front],
                              corner[0], normal[Bottom]);

  v[2] = intersectThreePlanes(corner[1], normal[Right],
                              corner[1], normal[Back],
                              corner[0], normal[Bottom]);

  v[3] = intersectThreePlanes(corner[0], normal[Left],
                              corner[1], normal[Back],
                              corner[0], normal[Bottom]);

  v[4] = intersectThreePlanes(corner[0], normal[Left],
                              corner[0], normal[Front],
                              corner[1], normal[Top]);

  v[5] = intersectThreePlanes(corner[1], normal[Right],
                              corner[0], normal[Front],
                              corner[1], normal[Top]);

  v[6] = corner[1];

  v[7] = intersectThreePlanes(corner[0], normal[Left],
                              corner[1], normal[Back],
                              corner[1], normal[Top]);

  for(unsigned i=0; i<8; i++)
    bbox.extendByPoint(v[i]);
}

void QuadFacedHexahedron::intersect(const RenderContext& context,
                                    RayPacket& rays) const {
  const int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;
  if(debugFlag)
    std::cout << "QuadFacedHexadron::intersect()" << std::endl;

  Real t[6];

  for(int i=rays.begin(); i<rays.end(); i++){
    // TODO(choudhury): Make the slab intersection computation
    // cleaner.  Sort the results between slabs early and eliminate
    // half of the test cases ("if(t[f] < t[f+1]) ... else if(t[f+1] <
    // t[f]) ...") later on.  Also make the entire special case testing cleaner.

    // TODO(choudhury): Rendering from inside the hex is broken for
    // some reason.  Probably has to do with incomplete special case
    // testing.

    for(int f=0; f<6; f += 2){
      Real dn0 = Dot(rays.getDirection(i), normal[f]);
      Real dn1 = Dot(rays.getDirection(i), normal[f+1]);
      Real ao0 = 0.0, ao1 = 0.0;

      if(dn0 == 0){
        t[f] = std::numeric_limits<Real>::max();
      }
      else{
        ao0 = Dot(corner[0] - rays.getOrigin(i), normal[f]);
        t[f] = ao0/dn0;
      }

      if(dn1 == 0){
        t[f+1] = std::numeric_limits<Real>::max();
      }
      else{
        ao1 = Dot(corner[1] - rays.getOrigin(i), normal[f+1]);
        t[f+1] = ao1/dn1;
      }

      // Check if we are inside the slab, on the same side as the hex
      // ("cis").
      if(-ao0 <= 0.0 && -ao1 <= 0.0){
        // For both t's positive, entry occurs at negative infinity,
        // and exit occurs at the least positive of the two forward
        // intersections.
        if(t[f] < t[f+1] && t[f] >= 0.0)
          t[f+1] = -std::numeric_limits<Real>::max();
        else if(t[f+1] < t[f] && t[f+1] >= 0.0)
          t[f] = -std::numeric_limits<Real>::max();

        // However if both are negative, then the entry is the max and
        // the exit is infinity.
        if(t[f] > t[f+1] && t[f] <= 0.0)
          t[f+1] = std::numeric_limits<Real>::max();
        else if(t[f+1] > t[f] && t[f+1] <= 0.0)
          t[f] = std::numeric_limits<Real>::max();
      }
      else if(-ao0 >= 0.0 && -ao1 >= 0.0){
        // If we are inside the slab but on the other side from the
        // hex ("trans"), the situation is converse of the above case.
        if(t[f] > t[f+1] && t[f+1] >= 0.0)
          t[f+1] = std::numeric_limits<Real>::max();
        else if(t[f+1] > t[f] && t[f] >= 0.0)
          t[f] = std::numeric_limits<Real>::max();

        // Ditto for t's both negative.
        if(t[f] < t[f+1] && t[f+1] <= 0.0)
          t[f+1] = -std::numeric_limits<Real>::max();
        else if(t[f+1] < t[f] && t[f] <= 0.0)
          t[f] = -std::numeric_limits<Real>::max();
      }
      else{
        // If we're outside there are special cases too.  If one t is
        // negative and the other positive, really we are entering the
        // slab at the positive t, and then never leaving, so convert
        // the negative t to positive infinity.
        if(t[f] < 0.0 && t[f+1] > 0.0)
          t[f] = std::numeric_limits<Real>::max();
        else if(t[f+1] < 0.0 && t[f] > 0.0)
          t[f+1] = std::numeric_limits<Real>::max();
      }
    }

    if(debugFlag){
      std::cout << "Bottom: " << t[0] << std::endl
                << "Left  : " << t[1] << std::endl
                << "Front : " << t[2] << std::endl
                << "Right : " << t[3] << std::endl
                << "Back  : " << t[4] << std::endl
                << "Top   : " << t[5] << std::endl;
    }

    // Compute the max t over the set of minimum t, one from each
    // opposing pair of planes, and the "min over max".
    Real exit_t = Min(Min(Max(t[Front], t[Back]),
                          Max(t[Left], t[Right])),
                      Max(t[Bottom], t[Top]));

    int nearest[] = {Back, Right, Top};

    Real vals[3] = {t[Back], t[Right], t[Top]};
    if(t[Front] < t[Back]){
      nearest[0] = Front;
      vals[0] = t[Front];
    }
    if(t[Left] < t[Right]){
      nearest[1] = Left;
      vals[1] = t[Left];
    }
    if(t[Bottom] < t[Top]){
      nearest[2] = Bottom;
      vals[2] = t[Bottom];
    }

    int faceIdx=-1;
    if(vals[0] > vals[1] && vals[0] > vals[2])
      faceIdx = 0;
    else if(vals[1] > vals[2])
      faceIdx = 1;
    else
      faceIdx = 2;

    Real entry_t = vals[faceIdx];
    int hitface = nearest[faceIdx];

    if(debugFlag){
      std::cout << "entry_t = " << entry_t << std::endl
                << "exit_t  = " << exit_t << std::endl;
    }

    // If the exit_t is less than the entry_t, then there was no intersection.
    if(exit_t < entry_t)
      continue;

    // If the entry_t is negative, use the exit_t.
    if(entry_t < 0.0){
      if(rays.hit(i, exit_t, getMaterial(), this, getTexCoordMapper()))
        rays.getScratchpad<int>(0)[i] = hitface;
    }
    else if(rays.hit(i, entry_t, getMaterial(), this, getTexCoordMapper()))
      rays.getScratchpad<int>(0)[i] = hitface;
  }
}

void QuadFacedHexahedron::computeNormal(const RenderContext& context,
                                        RayPacket& rays) const {
  // NOTE(choudhury): using this class with an RTInstance (and
  // possibly other types of instances) corrupts the scratchpad and
  // this section of code becomes seemingly buggy.  Don't use this
  // class with RTInstance and other offending instance classes.
  for(int i=rays.begin(); i<rays.end(); i++){
    const int which = rays.getScratchpad<int>(0)[i];
    rays.setNormal(i, normal[which]);
  }
}

Vector QuadFacedHexahedron::intersectThreePlanes(const Vector& p1, const Vector& N1,
                                                 const Vector& p2, const Vector& N2,
                                                 const Vector& p3, const Vector& N3){
  AffineTransform A_inv;
  A_inv.initWithBasis(Vector(N1[0], N2[0], N3[0]),
                      Vector(N1[1], N2[1], N3[1]),
                      Vector(N1[2], N2[2], N3[2]),
                      Vector(0.0,0.0,0.0));
  A_inv.invert();

  Vector b(Dot(p1, N1), Dot(p2, N2), Dot(p3, N3));

  return A_inv.multiply_vector(b);
}

bool QuadFacedHexahedron::coplanar(const Vector& p, const Vector& N,
                                   const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3){
  static const Real eps = 1.0e-3;
  return (Dot(v0-p, N) < eps &&
          Dot(v1-p, N) < eps &&
          Dot(v2-p, N) < eps &&
          Dot(v3-p, N) < eps);
}
