
#include <Model/Primitives/Disk.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/Trig.h>

using namespace Manta;
using namespace std;

Disk::Disk(Material* mat, const Vector& center, const Vector& n,
           Real radius, const Vector& axis) 
  : PrimitiveCommon(mat, this), _c(center), _n(n), _r(radius),
    _minTheta(0.0), _maxTheta(2.0 * M_PI),
    _partial(false)
{
  _n.normalize();
  _d = -Dot(_n, _c);
  setupAxes(axis);
}

Disk::Disk(Material* mat, const Vector& center, const Vector& n,
           Real radius, const Vector& axis, Real minTheta, Real maxTheta) 
  : PrimitiveCommon(mat, this), _c(center), _n(n), _r(radius),
    _minTheta(minTheta), _maxTheta(maxTheta),
    _partial(true)
{
  _n.normalize();
  _d = -Dot(_n, _c);
  setupAxes(axis);
}

Disk::~Disk()
{
}

void Disk::computeBounds(const PreprocessContext& /*context*/,
                         BBox& bbox) const
{
  bbox.extendByDisc(_c, _n, _r);
}

void Disk::intersect(const RenderContext& /*context*/, RayPacket& rays) const
{
  if (rays.getFlag(RayPacket::ConstantOrigin)) {
    Vector rayO = rays.getOrigin(rays.begin());
    Real nDotO(Dot(_n, rayO));

    for (int i = rays.begin(); i < rays.end(); i++) {
      Vector rayD = rays.getDirection(i);
      Real denom = Dot(_n, rayD);

      if (denom < -DENOM_EPSILON || denom > DENOM_EPSILON) {
        Real t = -(_d + nDotO) / denom;
        if (checkBounds(rayO + t * rayD)) {
          rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
        }
      }
    }
  } else {
    for (int i = rays.begin(); i < rays.end(); i++) {
      Vector rayO = rays.getOrigin(i);
      Vector rayD = rays.getDirection(i);
      Real denom = Dot(_n, rayD);

      if (denom < -DENOM_EPSILON || denom > DENOM_EPSILON) {
        Real t = -(_d + Dot(_n, rayO)) / denom;
        if (checkBounds(rayO + t * rayD)) {
          rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
        }
      }
    }
  }
}


void Disk::computeNormal(const RenderContext& /*context*/,
                         RayPacket& rays) const
{
  for (int i = rays.begin(); i < rays.end(); i++)
    rays.setNormal(i, _n);

  // set flags to indicate packet now has unit normals
  rays.setFlag(RayPacket::HaveNormals & RayPacket::HaveUnitNormals);
}

void Disk::computeTexCoords2(const RenderContext& /*context*/,
                             RayPacket& rays) const
{
  // compute hit locations if necessary
  rays.computeHitPositions();

  // set 2-d texture coordinates as returned by getTexCoords()
  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector p = rays.getHitPosition(i);
    Vector dir = p - _c;
    Real dist = dir.normalize();
    Real theta = Atan2(Dot(_v, dir), Dot(_u, dir));
    if(theta < 0)
      theta += (Real)M_PI;
    rays.setTexCoords(i, Vector(dist/_r, (theta - _minTheta) / (_maxTheta - _minTheta), 0));
  }

  // set flag to show texcoords have been computed
  rays.setFlag(RayPacket::HaveTexture2 | RayPacket::HaveTexture3);
}

void Disk::computeTexCoords3(const RenderContext& /*context*/,
                             RayPacket& rays) const
{
  // compute hit locations if necessary
  if (!(rays.getFlag(RayPacket::HaveHitPositions)))
    rays.computeHitPositions();

  // set 3-d texture coordinates to be the hit locations
  for (int i = rays.begin(); i < rays.end(); i++) {
    Vector p = rays.getHitPosition(i);
    Vector dir = p - _c;
    Real dist = dir.normalize();
    Real theta = Atan2(Dot(_v, dir), Dot(_u, dir));
    if(theta < 0)
      theta += (Real)M_PI;
    rays.setTexCoords(i, Vector(dist/_r, (theta - _minTheta) / (_maxTheta - _minTheta), 0));
  }

  // set flag to show texcoords have been computed
  rays.setFlag(RayPacket::HaveTexture3 | RayPacket::HaveTexture3);
}

/**
 * checkBounds(p)
 *
 * Assumes the point p lies in the same plane as the disk.  Returns true if p
 * lies within the region of the plane encompassed by the (possibly partial)
 * disk, false otherwise.
 **/
bool Disk::checkBounds(const Vector& p) const {
  Vector dir(p - _c);
  Real dist(dir.normalize());

  if (dist > _r)
    return false;

  if (_partial) {
    Real theta(Atan2(Dot(_v, dir), Dot(_u, dir)));

    if (theta < 0)
      theta = 2 * (Real)M_PI + theta;
    if ((theta < _minTheta) || (theta > _maxTheta))
      return false;
  }

  return true;
}

/**
 * setupAxes(axis)
 *
 * Sets up the local coordinate system of the disk.  The disk normal is always
 * preserved.  After calling this function, the u axis will be the projection of
 * the given vector axis onto the plane of the disk.  If the given vector is
 * close to the direction of the normal, then the z-axis is used instead of the
 * given vector.
 **/
void Disk::setupAxes(const Vector& axis)
{
  static const Real EPSILON(1.0e-6);

  _u = axis;
  _u.normalize();
  _v = Cross(_n, _u);
  if (_v.length2() < EPSILON) {
    _u = Vector(0.0, 0.0, 1.0);
    _v = Cross(_n, _u);
    if (_v.length2() < EPSILON) {
      _u = Vector(0.0, 1.0, 0.0);
      _v = Cross(_n, _u);
    }
  }
  _v.normalize();
  _u = Cross(_v, _n);
}
