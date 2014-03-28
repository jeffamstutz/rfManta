
#include <Model/Primitives/Hemisphere.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/Expon.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>

// TODO: tighter bounds

using namespace Manta;
using namespace std;

Hemisphere::Hemisphere(Material* material, const Vector& center,
                       Real radius, const Vector& normal)
  : PrimitiveCommon(material, this), _c(center), _r(radius), _n(normal)
{
  setupAxes();
  _inv_r = 1/_r;
}

Hemisphere::~Hemisphere() {
}

void Hemisphere::computeBounds(const PreprocessContext&, BBox& bbox) const
{
  bbox.extendBySphere(_c, _r);
}

void Hemisphere::intersect(const RenderContext&, RayPacket& rays) const
{
  switch(rays.getAllFlags()&(RayPacket::ConstantOrigin |
                             RayPacket::NormalizedDirections)) {
  case RayPacket::ConstantOrigin | RayPacket::NormalizedDirections:
    {
      // Rays all have the same origin and unit-length directions
      Vector rayO = rays.getOrigin(rays.begin());
      Vector tRayO = rayO - _c;
      Real c = Dot(tRayO, tRayO) - _r * _r;
      
      for (int i = rays.begin(); i < rays.end(); i++) {
        Vector rayD = rays.getDirection(i);
        
        Real b = Dot(tRayO, rayD);
        Real disc = b * b - c;
        
        if (disc >= 0) {
          Real r = Sqrt(disc);
          Real t = -(b + r);
          if (t > 0 && checkBounds(rayO + t * rayD)) {
            rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
          } else {
            t = r - b;
            if (t > 0 && checkBounds(rayO + t * rayD)) {
              rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      }
    }
    break;
  case RayPacket::ConstantOrigin:
    {
      // Rays all have the same origin
      Vector rayO = rays.getOrigin(rays.begin());
      Vector tRayO = rayO - _c;
      Real c = Dot(tRayO, tRayO) - _r * _r;
      
      for (int i = rays.begin(); i < rays.end(); i++) {
        Vector rayD = rays.getDirection(i);
        
        Real a = Dot(rayD, rayD);
        Real b = Dot(tRayO, rayD);
        Real disc = b * b - a * c;
        
        if (disc >= 0) {
          Real r = Sqrt(disc);
          Real t = -(b + r) / a;
          if (t > 0 && checkBounds(rayO + t * rayD)) {
            rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
          } else {
            t = (r - b) / a;
            if (t > 0 && checkBounds(rayO + t * rayD)) {
              rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      }
    }
    break;
  case RayPacket::NormalizedDirections:
    {
      // Rays all have unit-length direction
      for (int i = rays.begin(); i < rays.end(); i++) {
        Vector rayO = rays.getOrigin(i);
        Vector rayD = rays.getDirection(i);
        Vector tRayO = rayO - _c;
        
        Real b = Dot(tRayO, rayD);
        Real c = Dot(tRayO, tRayO) - _r * _r;
        Real disc = b * b - c;
        
        if (disc >= 0) {
          Real r = Sqrt(disc);
          Real t = -(b + r);
          if (t > 0.0 && checkBounds(rayO + t * rayD)) {
            rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
          } else {
            t = r - b;
            if (t > 0 && checkBounds(rayO + t * rayD)) {
              rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      }
    }
    break;
  case 0:
    {
      // General rays
      for (int i = rays.begin(); i < rays.end(); i++) {
        Vector rayO = rays.getOrigin(i);
        Vector rayD = rays.getDirection(i);
        Vector tRayO = rayO - _c;
        
        Real a = Dot(rayD, rayD);
        Real b = Dot(tRayO, rayD);
        Real c = Dot(tRayO, tRayO) - _r * _r;
        Real disc = b * b - a * c;
        
        if (disc >= 0) {
          Real r = Sqrt(disc);
          Real t = -(b + r) / a;
          if ((t > 0) && checkBounds(rayO + t * rayD)) {
            rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
          } else {
            t = (r - b) / a;
            if (t > 0 && checkBounds(rayO + t * rayD)) {
              rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      }
    }
  }
}

void Hemisphere::computeNormal(const RenderContext& /*context*/,
                               RayPacket& rays) const
{
  rays.computeHitPositions();

  for (int i = rays.begin(); i < rays.end(); i++)
    rays.setNormal(i, (rays.getHitPosition(i) - _c) * _inv_r);
  rays.setFlag(RayPacket::HaveUnitNormals);
}

void Hemisphere::computeTexCoords2(const RenderContext& context, RayPacket& rays) const
{
  if (!(rays.getFlag(RayPacket::HaveNormals & RayPacket::HaveUnitNormals)))
    computeNormal(context, rays);

  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector n = rays.getNormal(i);
    Real theta = Acos(Dot(_n, n));
    Real phi = Atan2(Dot(_u, n), Dot(_v, n));
    Real x = phi * (Real)(M_1_PI*0.5);
    Real y = theta * (Real)M_2_PI;
    rays.setTexCoords(i, Vector(x, y, 0));
  }

  rays.setFlag(RayPacket::HaveTexture2 | RayPacket::HaveTexture3);
}

void Hemisphere::computeTexCoords3(const RenderContext& context, RayPacket& rays) const
{
  if (!(rays.getFlag(RayPacket::HaveNormals & RayPacket::HaveUnitNormals)))
    computeNormal(context, rays);

  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector n = rays.getNormal(i);
    Real theta = Acos(Dot(_n, n));
    Real phi = Atan2(Dot(_u, n), Dot(_v, n));
    Real x = phi * (Real)(M_1_PI*0.5);
    Real y = theta * (Real)M_2_PI;
    rays.setTexCoords(i, Vector(x, y, 0));
  }

  rays.setFlag(RayPacket::HaveTexture2 | RayPacket::HaveTexture3);
}

/**
 * checkBounds(p)
 *
 * Given a point p assumed to lie on the sphere, checks to make sure it lies
 * within the correct hemisphere.
 **/
bool Hemisphere::checkBounds(const Vector& p) const
{
  return (Dot(_n, p - _c) >= 0);
}

void Hemisphere::setupAxes()
{
  _u = Vector(1, 0, 0);
  _v = Cross(_n, _u);
  if (_v.length2() < T_EPSILON) {
    _u = Vector(0, 1, 0);
    _v = Cross(_n, _u);
  }
  _v.normalize();
  _u = Cross(_v, _n);
}
