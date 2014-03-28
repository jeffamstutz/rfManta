
#include <Model/TexCoordMappers/LinearMapper.h>
#include <Interface/RayPacket.h>
#include <Core/Math/MiscMath.h>

using namespace Manta;

LinearMapper::LinearMapper(const Vector& origin, const Vector& v1,
                           const Vector& v2, const Vector& v3)
{
  transform.initWithBasis(v1, v2, v3, origin);
  transform.invert();
}

LinearMapper::LinearMapper(const AffineTransform& transform)
  : transform(transform)
{
  this->transform.invert();
}

LinearMapper::~LinearMapper()
{
}

void LinearMapper::computeTexCoords2(const RenderContext&,
				      RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++)
    // Unproject a hit point by multiplying by inverse.
    rays.setTexCoords(i, transform.multiply_point(rays.getHitPosition(i)));
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void LinearMapper::computeTexCoords3(const RenderContext&,
				      RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++)
    // Unproject a hit point by multiplying by inverse.
    rays.setTexCoords(i, transform.multiply_point(rays.getHitPosition(i)));
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}
