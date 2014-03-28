
#include <Interface/Primitive.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/Vector.h>
#include <Core/Exceptions/InternalError.h>

using namespace Manta;

Primitive::Primitive() {
}

Primitive::~Primitive() {
}

void Primitive::getRandomPoints(Packet<Vector>& points,
                                Packet<Vector>& normals,
                                Packet<Real>& pdfs,
                                const RenderContext& context,
                                RayPacket& rays) const {
  throw InternalError("Unimplemented getRandomPoint for Primitive");
}

void Primitive::computeGeometricNormal(const RenderContext& context,
                                        RayPacket& rays) const {
  rays.computeNormals<true>(context);

  for(int i = 0; i < 3; ++i) {
    memcpy(&rays.data->geometricNormal[i][rays.begin()],
           &rays.data->normal[i][rays.begin()],
           (rays.end()-rays.begin())*sizeof(Real));
  }

  rays.setFlag( RayPacket::HaveGeometricNormals );
  if( rays.getFlag( RayPacket::HaveUnitNormals ) )
    rays.setFlag( RayPacket::HaveUnitGeometricNormals );
}

void Primitive::computeSurfaceDerivatives(const RenderContext& context,
                                          RayPacket& rays) const {
  rays.computeNormals<true>(context);
  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector U = rays.getNormal(i).findPerpendicular();
    Vector V = Cross(rays.getNormal(i), U);
    rays.setSurfaceDerivativeU(i, U);
    rays.setSurfaceDerivativeV(i, V);
  }
  rays.setFlag(RayPacket::HaveSurfaceDerivatives);
}
