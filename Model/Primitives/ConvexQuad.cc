#include <Model/Primitives/ConvexQuad.h>
#include <Core/Exceptions/BadPrimitive.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>
#include <Model/Primitives/Plane.h>

#include <iostream>
#include <sstream>

using namespace Manta;

ConvexQuad::ConvexQuad(Material *mat,
		       const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3)
  : PrimitiveCommon(mat)
{
  // Store the vertices, as they will be needed in the intersect
  // routine.
  v[0] = v0;
  v[1] = v1;
  v[2] = v2;
  v[3] = v3;

  // Create a plane that includes the quad.
  normal = Cross(v[1]-v[0], v[3]-v[0]);
  normal.normalize();

  // Clearly, v0, v1, and v3 belong to the plane; check if v2 does too.
  // TODO: what is the correct value to use for eps?
//   const Real eps = 1e-3;
//   const Real value = Abs(Dot(v[2]-v[0], normal));
//   if(value > eps){
//     std::cerr << "value: " << value << std::endl;
//     std::cerr << v[0] << std::endl
//               << v[1] << std::endl
//               << v[2] << std::endl
//               << v[3] << std::endl;
//     throw BadPrimitive("ConvexQuad vertices are not co-planar");
//   }

  // Check that the quad is convex.
  for(int i=0; i<4; i++){
    int j = (i+1) % 4;
    int k = (i+2) % 4;
    if(Dot(Cross(v[j]-v[i], v[k]-v[j]), normal) < 0){
      std::stringstream s;
      s << "ConvexQuad vertices (" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ") do not specify a convex region";
      throw BadPrimitive(s.str().c_str());
    }
  }
}

ConvexQuad::~ConvexQuad(){

}

void ConvexQuad::computeBounds(const PreprocessContext& context,
			       BBox& bbox_) const {
  bbox_.extendByPoint(v[0]);
  bbox_.extendByPoint(v[1]);
  bbox_.extendByPoint(v[2]);
  bbox_.extendByPoint(v[3]);
}

void ConvexQuad::intersect(const RenderContext& context,
			   RayPacket& rays) const {
  const int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;

  // Begin by intersecting the rays with the plane containing the
  // quad.
  for(int i=rays.begin(); i<rays.end(); i++){
    Real dn = Dot(rays.getDirection(i), normal);

    // The ray is parallel to the plane, so no hit.
    if(dn == 0)
      continue;

    Real ao = Dot((v[0]-rays.getOrigin(i)), normal);
    Real t = ao/dn;

    const Vector p = rays.getOrigin(i) + t*rays.getDirection(i);

    if(debugFlag)
      cerr << "p = " << p << endl;

    // Now check if the hit position falls inside the convex region
    // itself.
    bool inside = true;
    for(int j=0; j<4; j++){
      const Real d = Dot(Cross(v[(j+1)%4]-v[j], p-v[j]), normal);
      if(debugFlag)
	cerr << "side " << j << ": d = " << d << endl;
      if(d < 0.0){
	inside = false;
	break;
      }
    }

    if(inside)
      rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
  }
}

void ConvexQuad::computeNormal(const RenderContext& context,
			       RayPacket& rays) const {
  for(int i=rays.begin(); i<rays.end(); i++)
    rays.setNormal(i, normal);
}
