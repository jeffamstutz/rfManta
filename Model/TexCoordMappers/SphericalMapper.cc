
#include <Model/TexCoordMappers/SphericalMapper.h>
#include <Interface/RayPacket.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>

using namespace Manta;

SphericalMapper::SphericalMapper(const Vector& center, Real radius)
  : center(center), radius(radius)
{
  inv_radius = 1/radius;
}

SphericalMapper::~SphericalMapper()
{
}

void SphericalMapper::computeTexCoords2(const RenderContext&,
				      RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++){
    Vector n = rays.getHitPosition(i)-center;
    Real w = n.normalize() * inv_radius;
    Real angle = Clamp(n.z(), (Real)-1, (Real)1);
    Real theta = Acos(angle);
    Real phi = Atan2(n.x(), n.y());
    rays.setTexCoords(i, Vector((phi+(Real)M_PI)*(Real)(0.5*M_1_PI),
                                theta*(Real)M_1_PI,
                                w));
  }
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void SphericalMapper::computeTexCoords3(const RenderContext&,
                                        RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++){
    Vector n = rays.getHitPosition(i)-center;
    Real w = n.normalize() * inv_radius;
    Real angle = Clamp(n.z(), (Real)-1, (Real)1);
    Real theta = Acos(angle);
    Real phi = Atan2(n.x(), n.y());
    rays.setTexCoords(i, Vector((phi+(Real)M_PI)*(Real)(0.5*M_1_PI),
                                theta*(Real)M_1_PI,
                                w));
  }
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

