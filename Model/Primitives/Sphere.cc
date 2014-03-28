
#include <Model/Primitives/Sphere.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>
#include <Core/Math/TrigSSE.h>
#include <Core/Math/Expon.h>
#include <Core/Math/SSEDefs.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Interface/RayPacket.h>
#include <Interface/InterfaceRTTI.h>
#include <MantaSSE.h>
#include <Interface/Context.h>
#include <Interface/SampleGenerator.h>

using namespace Manta;
using namespace std;

#define USE_RTSL_OUTPUT 0

Sphere::Sphere()
{
}

Sphere::Sphere(Material* material, const Vector& center, Real radius)
  : PrimitiveCommon(material), center(center), radius(radius)
{
  inv_radius = 1/radius;
}

Sphere::~Sphere()
{
}

Sphere* Sphere::clone(CloneDepth depth, Clonable* incoming)
{
  Sphere *copy;
  if (incoming)
    copy = dynamic_cast<Sphere*>(incoming);
  else
    copy = new Sphere();

  PrimitiveCommon::clone(depth, copy);
  copy->center = center;
  copy->radius = radius;
  copy->inv_radius = inv_radius;
  return copy;
}

void Sphere::computeBounds(const PreprocessContext&, BBox& bbox) const
{
  bbox.extendBySphere(center, radius);
}

#if USE_RTSL_OUTPUT
#  include "SphereRTSL.cc"
#else

void Sphere::intersect(const RenderContext&, RayPacket& rays) const
{
  switch(rays.getAllFlags() & (RayPacket::ConstantOrigin|RayPacket::NormalizedDirections)){
  case RayPacket::ConstantOrigin|RayPacket::NormalizedDirections:
    {
      // Rays of constant origin and normalized directions
      Vector O_original(rays.getOrigin(rays.begin())-center);
#ifdef MANTA_SSE
      int b = (rays.rayBegin + 3) & (~3);
      int e = rays.rayEnd & (~3);
      if(b >= e){
        for(int i = rays.begin(); i < rays.end(); i++){
          const Vector D(rays.getDirection(i));
          //const Real A = 1;//Dot(D, D);
          Real B = Dot(O_original, D);

          const Real t = -B; // start ray between sphere hit points
          const Vector O = O_original+t*D;
          B = Dot(O, D);

          const Real C = Dot(O, O) - radius*radius;
          const Real disc = B*B-C;

          if(disc >= 0){
            Real r = Sqrt(disc);
            Real t0 = t - (r+B);
            if(t0 > T_EPSILON){
              rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
            } else {
              Real t1 = t + (r-B);
              rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      }
      else {
        int i = rays.rayBegin;
        for(;i<b;i++){
          const Vector D(rays.getDirection(i));
          //const Real A = 1;//Dot(D, D);
          Real B = Dot(O_original, D);

          const Real t = -B; // start ray between sphere hit points
          const Vector O = O_original+t*D;
          B = Dot(O, D);

          const Real C = Dot(O, O) - radius*radius;
          const Real disc = B*B-C;

          if(disc >= 0){
            Real r = Sqrt(disc);
            Real t0 = t - (r+B);
            if(t0 > T_EPSILON){
              rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
            } else {
              Real t1 = t + (r-B);
              rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
            }
          }
        }

        RayPacketData* data = rays.data;
        sse_t O_originalx = set4(O_original[0]);
        sse_t O_originaly = set4(O_original[1]);
        sse_t O_originalz = set4(O_original[2]);
        sse_t radius2 = set4(radius*radius);
        for(;i<e;i+=4){
          sse_t Dx = load44(&data->direction[0][i]);
          sse_t Dy = load44(&data->direction[1][i]);
          sse_t Dz = load44(&data->direction[2][i]);

          sse_t B = dot4(O_originalx, O_originaly, O_originalz, Dx, Dy, Dz);
          sse_t t = sub4(_mm_zero, B);
          sse_t Ox = add4(O_originalx, mul4(t,Dx));
          sse_t Oy = add4(O_originaly, mul4(t,Dy));
          sse_t Oz = add4(O_originalz, mul4(t,Dz));
          B = dot4(Ox, Oy, Oz, Dx, Dy, Dz);
          sse_t C = sub4(dot4(Ox, Oy, Oz, Ox, Oy, Oz), radius2);

          sse_t disc = sub4(mul4(B, B), C);
          sse_t hit = cmp4_ge(disc, zero4());
          if(getmask4(hit) == 0)
            continue;

          sse_t r = sqrt4(disc);
          sse_t t0 = sub4(t, add4(r, B));
          sse_t hit0 = and4(hit, cmp4_gt(t0, set4(T_EPSILON)));
          rays.hitWithoutTminCheck(i, hit0, t0, getMaterial(), this, getTexCoordMapper());

          hit = andnot4(hit0, hit);
          // Rays that successfully hit at t0 are masked off
          if(getmask4(hit) == 0)
            continue;

          sse_t t1 = add4(t, sub4(r, B));
          sse_t hit1 = and4(hit, cmp4_gt(t1, set4(T_EPSILON)));
          rays.hitWithoutTminCheck(i, hit1, t1, getMaterial(), this, getTexCoordMapper());
        }
        for(;i<rays.rayEnd;i++){
          const Vector D(rays.getDirection(i));
          //const Real A = 1;//Dot(D, D);
          Real B = Dot(O_original, D);

          const Real t = -B; // start ray between sphere hit points
          const Vector O = O_original+t*D;
          B = Dot(O, D);

          const Real C = Dot(O, O) - radius*radius;
          const Real disc = B*B-C;

          if(disc >= 0){
            Real r = Sqrt(disc);
            Real t0 = t - (r+B);
            if(t0 > T_EPSILON){
              rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
            } else {
              Real t1 = t + (r-B);
              rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      }
#else
      for(int i = rays.begin();i<rays.end();i++){
        const Vector D(rays.getDirection(i));
        //const Real A = 1;//Dot(D, D);
        Real B = Dot(O_original, D);

        const Real t = -B; // start ray between sphere hit points
        const Vector O = O_original+t*D;
        B = Dot(O, D);

        const Real C = Dot(O, O) - radius*radius;
        const Real disc = B*B-C;

        if(disc >= 0){
          Real r = Sqrt(disc);
          Real t0 = t - (r+B);
          if(t0 > T_EPSILON){
            rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
          } else {
            Real t1 = t + (r-B);
            rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
          }
        }
      }
#endif
    }
  break;
  case RayPacket::ConstantOrigin:
    {
      // Rays of constant origin for not normalized directions
      const Vector O_original(rays.getOrigin(rays.begin())-center);
      for(int i = rays.begin();i<rays.end();i++){
        const Vector D(rays.getDirection(i));
        const Real A = Dot(D, D);
        Real B = Dot(O_original, D);

        const Real t = -B; // start ray between sphere hit points
        const Vector O = O_original+t*D;
        B = Dot(O, D);

        const Real C = Dot(O, O) - radius*radius;
        const Real disc = B*B-A*C;

        if(disc >= 0){
          const Real A_inv = 1/A;
          Real r = Sqrt(disc);
          Real t0 = t - (r+B)*A_inv;
          if(t0 > T_EPSILON){
            rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
          } else {
            Real t1 = t + (r-B)*A_inv;
            rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
          }
        }
      }
    }
    break;
  case RayPacket::NormalizedDirections:
    {
#ifdef MANTA_SSE
      int b = (rays.rayBegin + 3) & (~3);
      int e = rays.rayEnd & (~3);
      if(b >= e){
        for(int i = rays.begin(); i < rays.end(); i++){
          Vector O(rays.getOrigin(i)-center);
          const Vector D(rays.getDirection(i));
          //const Real A = 1;//Dot(D, D);
          Real B = Dot(O, D);

          const Real t = -B; // start ray between sphere hit points
          O = O+t*D;
          B = Dot(O, D);

          const Real C = Dot(O, O) - radius*radius;
          const Real disc = B*B-C;

          if(disc >= 0){
            Real r = Sqrt(disc);
            Real t0 = t - (r+B);
            if(t0 > T_EPSILON){
              rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
            } else {
              Real t1 = t + (r-B);
              rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      } else {
        int i = rays.rayBegin;
        for(;i<b;i++){
          Vector O(rays.getOrigin(i)-center);
          const Vector D(rays.getDirection(i));
          //const Real A = 1;//Dot(D, D);
          Real B = Dot(O, D);

          const Real t = -B; // start ray between sphere hit points
          O = O+t*D;
          B = Dot(O, D);

          const Real C = Dot(O, O) - radius*radius;
          const Real disc = B*B-C;

          if(disc >= 0){
            Real r = Sqrt(disc);
            Real t0 = t - (r+B);
            if(t0 > T_EPSILON){
              rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
            } else {
              Real t1 = t + (r-B);
              rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
        RayPacketData* data = rays.data;
        sse_t radius2 = set4(radius*radius);
        for(;i<e;i+=4){
          sse_t Ox = sub4(load44(&data->origin[0][i]), set4(center[0]));
          sse_t Oy = sub4(load44(&data->origin[1][i]), set4(center[1]));
          sse_t Oz = sub4(load44(&data->origin[2][i]), set4(center[2]));
          sse_t Dx = load44(&data->direction[0][i]);
          sse_t Dy = load44(&data->direction[1][i]);
          sse_t Dz = load44(&data->direction[2][i]);

          sse_t B = dot4(Ox, Oy, Oz, Dx, Dy, Dz);
          sse_t t = sub4(_mm_zero, B);
          Ox = add4(Ox, mul4(t,Dx));
          Oy = add4(Oy, mul4(t,Dy));
          Oz = add4(Oz, mul4(t,Dz));
          B = dot4(Ox, Oy, Oz, Dx, Dy, Dz);
          sse_t C = sub4(dot4(Ox, Oy, Oz, Ox, Oy, Oz), radius2);

          sse_t disc = sub4(mul4(B, B), C);
          sse_t hit = cmp4_ge(disc, zero4());
          if(getmask4(hit) == 0)
            continue;

          sse_t r = _mm_sqrt_ps(disc);
          sse_t t0 = sub4(t, add4(r, B));
          sse_t hit0 = and4(hit, cmp4_gt(t0, set4(T_EPSILON)));
          rays.hitWithoutTminCheck(i, hit0, t0, getMaterial(), this, getTexCoordMapper());

          hit = andnot4(hit0, hit);
          // Rays that successfully hit at t0 are masked off
          if(getmask4(hit) == 0)
            continue;

          sse_t t1 = add4(t, sub4(r, B));
          sse_t hit1 = and4(hit, cmp4_gt(t1, set4(T_EPSILON)));
          rays.hitWithoutTminCheck(i, hit1, t1, getMaterial(), this, getTexCoordMapper());
        }
        for(;i<rays.rayEnd;i++){
          Vector O(rays.getOrigin(i)-center);
          const Vector D(rays.getDirection(i));
          //const Real A = 1;//Dot(D, D);
          Real B = Dot(O, D);

          const Real t = -B; // start ray between sphere hit points
          O = O+t*D;
          B = Dot(O, D);

          const Real C = Dot(O, O) - radius*radius;
          const Real disc = B*B-C;

          if(disc >= 0){
            Real r = Sqrt(disc);
            Real t0 = t - (r+B);
            if(t0 > T_EPSILON){
              rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
            } else {
              Real t1 = t + (r-B);
              rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
            }
          }
        }
      }
#else
      // Rays of non-constant origin and normalized directions
      for(int i = rays.begin();i<rays.end();i++){
        Vector O(rays.getOrigin(i)-center);
        const Vector D(rays.getDirection(i));
        //const Real A = 1;//Dot(D, D);
        Real B = Dot(O, D);

        const Real t = -B; // start ray between sphere hit points
        O = O+t*D;
        B = Dot(O, D);

        const Real C = Dot(O, O) - radius*radius;
        const Real disc = B*B-C;

        if(disc >= 0){
          Real r = Sqrt(disc);
          Real t0 = t - (r+B);
          if(t0 > T_EPSILON){
            rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
          } else {
            Real t1 = t + (r-B);
            rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
          }
        }
      }
#endif
    }
    break;
  case 0:
    {
      // Rays of non-constant origin and non-normalized directions
      for(int i = rays.begin();i<rays.end();i++){
        Vector O(rays.getOrigin(i)-center);
        const Vector D(rays.getDirection(i));
        const Real A = Dot(D, D);
        Real B = Dot(O, D);

        const Real t = -B; // start ray between sphere hit points
        O = O+t*D;
        B = Dot(O, D);

        const Real C = Dot(O, O) - radius*radius;
        const Real disc = B*B-A*C;

        if(disc >= 0){
          const Real A_inv = 1/A;
          Real r = Sqrt(disc);
          Real t0 = t - (r+B)*A_inv;
          if(t0 > T_EPSILON){
            rays.hit(i, t0, getMaterial(), this, getTexCoordMapper());
          } else {
            Real t1 = t + (r-B)*A_inv;
            rays.hit(i, t1, getMaterial(), this, getTexCoordMapper());
          }
        }
      }
    }
    break;
  }
}

void Sphere::computeNormal(const RenderContext&, RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++){
    Vector n = rays.getHitPosition(i) - center;
    n *= inv_radius;
    rays.setNormal(i, n);
  }
  rays.setFlag(RayPacket::HaveUnitNormals);
}
#endif // #if USE_RTSL_OUTPUT

void Sphere::computeTexCoords2(const RenderContext&,
                               RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++){
    Vector n = (rays.getHitPosition(i)-center)*inv_radius;
    Real angle = Clamp(n.z(), (Real)-1, (Real)1);
    Real theta = Acos(angle);
    Real phi = Atan2(n.y(), n.x());
    Real x = phi*(Real)(0.5*M_1_PI);
    if (x < 0)
      x += 1;
    Real y = theta*(Real)M_1_PI;
    rays.setTexCoords(i, Vector(x, y, 0));
  }
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void Sphere::computeTexCoords3(const RenderContext&,
                               RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++){
    Vector n = (rays.getHitPosition(i)-center)*inv_radius;
    Real angle = Clamp(n.z(), (Real)-1, (Real)1);
    Real theta = Acos(angle);
    Real phi = Atan2(n.y(), n.x());
    Real x = phi*(Real)(0.5*M_1_PI);
    if (x < 0)
      x += 1;
    Real y = theta*(Real)M_1_PI;
    rays.setTexCoords(i, Vector(x, y, 0));
  }
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void Sphere::getRandomPoints(Packet<Vector>& points,
                             Packet<Vector>& normals,
                             Packet<Real>& pdfs,
                             const RenderContext& context,
                             RayPacket& rays) const {
  // TODO(boulos): Change this code to only do work for active rays
  Packet<Real> r1;
  Packet<Real> r2;
  context.sample_generator->nextSeeds(context, r1, rays);
  context.sample_generator->nextSeeds(context, r2, rays);

  const Real inv_area = 1 / (4*M_PI*radius*radius);

#ifdef MANTA_SSE
  for (int i = 0; i < Packet<Vector>::MaxSize; i+=4) {
    sse_t z = sub4(_mm_one, mul4(_mm_two, load44(&r1.data[i])));
    const sse_t r = mul4(set4(radius), sqrt4(sub4(_mm_one, mul4(z, z))));
    z = add4(set4(center[2]), mul4(set4(radius), z));
    const sse_t phi = mul4(set4(2*M_PI), load44(&r2.data[i]));
    sse_t cos_phi, sin_phi;
    sincos4(phi, &cos_phi, &sin_phi);
    const sse_t x = add4(set4(center[0]), mul4(r, cos_phi));
    const sse_t y = add4(set4(center[1]), mul4(r, sin_phi));

    store44(&points.vectordata[0][i], x);
    store44(&points.vectordata[1][i], y);
    store44(&points.vectordata[2][i], z);

    store44(&normals.vectordata[0][i],
           mul4(sub4(load44(&points.vectordata[0][i]), set4(center[0])),
                set4(inv_radius)));
    store44(&normals.vectordata[1][i],
           mul4(sub4(load44(&points.vectordata[1][i]), set4(center[1])),
                set4(inv_radius)));
    store44(&normals.vectordata[2][i],
           mul4(sub4(load44(&points.vectordata[2][i]), set4(center[2])),
                set4(inv_radius)));

    store44(&pdfs.data[i], set4(inv_area));
  }
#else
  for (int i = rays.begin(); i < rays.end(); i++) {
    Real z = 1 - 2*r1.get(i);
    Real r = radius*Sqrt(1 - z*z);
    z = center[2] + radius*z;
    Real phi = 2 * M_PI * r2.get(i);
    Real x = center[0] + r*Cos(phi);
    Real y = center[1] + r*Sin(phi);
    points.set(i, Vector(x, y, z));
  }
  for (int i = rays.begin(); i < rays.end(); i++) {
    normals.set(i, (points.get(i) - center) * inv_radius);
  }
  for (int i = rays.begin(); i < rays.end(); i++) {
    pdfs.set(i, inv_area);
  }
#endif
}

Interpolable::InterpErr Sphere::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  PrimitiveCommon::interpolate(keyframes);
  center = Vector(0,0,0);
  radius = 0.0f;

  for (size_t i=0; i < keyframes.size(); ++i) {
    Interpolable::keyframe_t kt = keyframes[i];
    Sphere *sphere = dynamic_cast<Sphere*>(keyframes[i].keyframe);
    if (sphere == NULL)
      return notInterpolable;
    radius += sphere->radius * keyframes[i].t;
    center += sphere->center * keyframes[i].t;
  }

  inv_radius = 1/radius;

  return success;
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(Sphere, PrimitiveCommon, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(Sphere);
}

void Sphere::readwrite(ArchiveElement* archive)
{
  MantaRTTI<PrimitiveCommon>::readwrite(archive, *this);
  archive->readwrite("center", center);
  archive->readwrite("radius", radius);
  if(archive->reading())
    inv_radius = 1./radius;
}
