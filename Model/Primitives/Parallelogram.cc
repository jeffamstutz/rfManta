#include <Model/Primitives/Parallelogram.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/RayPacket.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/MiscMath.h>
#include <MantaSSE.h>
#include <Core/Math/SSEDefs.h>
#include <Interface/Context.h>
#include <Interface/SampleGenerator.h>

using namespace Manta;

#define USE_SCRATCHPAD 0

#define maskedStore_ps(mask, oldD, newD)                            \
  _mm_store_ps(oldD,                                                \
               _mm_or_ps(_mm_and_ps(mask, newD),                    \
                         _mm_andnot_ps(mask, _mm_load_ps(oldD))))

Parallelogram::Parallelogram()
{
}

Parallelogram::Parallelogram(Material* material, const Vector& anchor,
                             const Vector& in_v1, const Vector& in_v2)
  : PrimitiveCommon(material, this), anchor(anchor), v1(in_v1), v2(in_v2),
    v1_unscaled(in_v1), v2_unscaled(in_v2) {
  normal = Cross(v1, v2);
  inv_area = Real(1)/normal.normalize();
  d = Dot(normal, anchor);
  v1 *= 1./v1.length2();
  v2 *= 1./v2.length2();
}

Parallelogram::~Parallelogram() {
}

// NOTE(boulos): We might want to consider having the constructor just
// call this function
void Parallelogram::changeGeometry(Vector new_anchor,
                                   Vector new_v1,
                                   Vector new_v2) {
  anchor = new_anchor;
  v1_unscaled = new_v1;
  v2_unscaled = new_v2;
  v1 = new_v1;
  v2 = new_v2;
  normal = Cross(v1, v2);
  inv_area = Real(1)/normal.normalize();
  d = Dot(normal, anchor);
  v1 *= 1./v1.length2();
  v2 *= 1./v2.length2();
}

void Parallelogram::computeBounds(const PreprocessContext&, BBox& bbox) const
{
  // Extend the box by the four corners of the parallelogram.
  // v1 and v2 are stored with an inverse length to avoid extra
  // computation in the intersect routine, but we must divide that
  // out here.
  bbox.extendByPoint(anchor);
  bbox.extendByPoint(anchor+v1/v1.length2());
  bbox.extendByPoint(anchor+v2/v2.length2());
  bbox.extendByPoint(anchor+v1/v1.length2()+v2/v2.length2());
}

void Parallelogram::computeNormal(const RenderContext&, RayPacket& rays) const
{
  for(int i=rays.begin();i<rays.end();i++)
    rays.setNormal(i, normal);
  // We know the normal is unit length, so let the RayPacket know too.
  rays.setFlag(RayPacket::HaveUnitNormals);
}

void Parallelogram::intersect(const RenderContext&, RayPacket& rays) const
{
  // You should normalize the directions to make the "dt" dot product
  // meaningful.  As the ray directions get longer and shorter it can
  // affect this value.  You should be doing this dot product against
  // normalized vectors anyway.
  rays.normalizeDirections();
  if(rays.getFlag(RayPacket::ConstantOrigin)){
    Real num = d-Dot(normal, rays.getOrigin(rays.begin()));
    Vector a(rays.getOrigin(rays.begin())-anchor);
    Real o1 = Dot(a, v1);
    Real o2 = Dot(a, v2);
#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        Real dt=Dot(rays.getDirection(i), normal);
        if(Abs(dt) < (Real)1.e-6)
          continue;
        Real t=num/dt;
        if(t>rays.getMinT(i))
          continue;
        Vector vi(rays.getDirection(i)*t);
        Real a1 = Dot(v1, vi)+o1;
        if (a1 < 0 || a1 > 1)
          continue;
        Real a2 = Dot(v2, vi)+o2;
        if (a2 < 0 || a2 > 1)
          continue;

        if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
        }
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        Real dt=Dot(rays.getDirection(i), normal);
        if(Abs(dt) < (Real)1.e-6)
          continue;
        Real t=num/dt;
        if(t>rays.getMinT(i))
          continue;
        Vector vi(rays.getDirection(i)*t);
        Real a1 = Dot(v1, vi)+o1;
        if (a1 < 0 || a1 > 1)
          continue;
        Real a2 = Dot(v2, vi)+o2;
        if (a2 < 0 || a2 > 1)
          continue;

        if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
        }
      }
      RayPacketData* data = rays.data;
#if USE_SCRATCHPAD == 2
      float* scratch1 = rays.getScratchpad<float>(0);
      float* scratch2 = rays.getScratchpad<float>(1);
#endif
      __m128 normalx = _mm_set1_ps(normal[0]);
      __m128 normaly = _mm_set1_ps(normal[1]);
      __m128 normalz = _mm_set1_ps(normal[2]);
      __m128 vec_o1 = _mm_set1_ps(o1);
      __m128 vec_o2 = _mm_set1_ps(o2);
      __m128 vec_num = _mm_set1_ps(num);
      for(;i<e;i+=4){
        __m128 dx = _mm_load_ps(&data->direction[0][i]);
        __m128 dy = _mm_load_ps(&data->direction[1][i]);
        __m128 dz = _mm_load_ps(&data->direction[2][i]);
        __m128 dt = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, normalx), _mm_mul_ps(dy, normaly)), _mm_mul_ps(dz, normalz));

        //if(Abs(dt) < (Real)1.e-6)
        //continue;
        __m128 t = _mm_div_ps(vec_num, dt);
        __m128 hit = _mm_and_ps(_mm_cmplt_ps(t, _mm_load_ps(&data->minT[i])),
                                _mm_cmpgt_ps(t, _mm_set1_ps(T_EPSILON)));
        if(_mm_movemask_ps(hit) == 0)
          continue;

        // Real t=num/dt
        // if(t>rays.getMinT(i))
        //   continue;
        // Vector vi(rays.getDirection(i)*t);
        // Real a1 = Dot(v1, vi)*o1;
        __m128 vix = _mm_mul_ps(dx, t);
        __m128 viy = _mm_mul_ps(dy, t);
        __m128 viz = _mm_mul_ps(dz, t);
        __m128 a1 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(vix, _mm_set1_ps(v1[0])), _mm_mul_ps(viy, _mm_set1_ps(v1[1]))), _mm_mul_ps(viz, _mm_set1_ps(v1[2]))), vec_o1);
        // if (a1 < 0 || a1 > 1)
        //   continue;
        __m128 zero = _mm_setzero_ps();
        __m128 one = _mm_set1_ps(1.0f);
        hit = _mm_and_ps(hit, _mm_and_ps(_mm_cmpge_ps(a1, zero), _mm_cmple_ps(a1, one)));
        if(_mm_movemask_ps(hit) == 0)
          continue;

        // Real a2 = Dot(v2, vi)+o2;
        // if (a2 < 0 || a2 > 1)
        //   continue;

        __m128 a2 = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(vix, _mm_set1_ps(v2[0])), _mm_mul_ps(viy, _mm_set1_ps(v2[1]))), _mm_mul_ps(viz, _mm_set1_ps(v2[2]))), vec_o2);
        hit = _mm_and_ps(hit, _mm_and_ps(_mm_cmpge_ps(a2, zero), _mm_cmple_ps(a2, one)));
        if(_mm_movemask_ps(hit) == 0)
          continue;

        rays.hitWithoutTminCheck(i, hit, t, getMaterial(), this, getTexCoordMapper());

#if USE_SCRATCHPAD == 1
        // Copy the barycentric coordinates to the scratch pad
        MANTA_ALIGN(16) float ra1[4];
        MANTA_ALIGN(16) float ra2[4];
        _mm_store_ps(ra1, a1);
        _mm_store_ps(ra2, a2);

        if(_mm_movemask_ps(hit) == 15){
          rays.scratchpad<Vector>(i+0) = Vector(ra1[0], ra2[0], 0);
          rays.scratchpad<Vector>(i+1) = Vector(ra1[1], ra2[1], 0);
          rays.scratchpad<Vector>(i+2) = Vector(ra1[2], ra2[2], 0);
          rays.scratchpad<Vector>(i+3) = Vector(ra1[3], ra2[3], 0);
        } else {
          int hit_mask = _mm_movemask_ps(hit);
          for(int j = 0; j < 4; ++j) {
            if (hit_mask & (1 << j))
              rays.scratchpad<Vector>(i+j) = Vector(ra1[j], ra2[j], 0);
          }
        }
#else
#if USE_SCRATCHPAD == 2
        if (_mm_movemask_ps(hit) == 15) {
          _mm_store_ps(&scratch1[i], a1);
          _mm_store_ps(&scratch2[i], a2);
        } else {
          maskedStore_ps(hit, (float*)&scratch1[i], a1);
          maskedStore_ps(hit, (float*)&scratch2[i], a2);
        }
#else
        if (_mm_movemask_ps(hit) == 15) {
          _mm_store_ps(&data->texCoords[0][i], a1);
          _mm_store_ps(&data->texCoords[1][i], a2);
          _mm_store_ps(&data->texCoords[2][i], _mm_setzero_ps());
        } else {
          maskedStore_ps(hit, &data->texCoords[0][i], a1);
          maskedStore_ps(hit, &data->texCoords[1][i], a2);
          maskedStore_ps(hit, &data->texCoords[2][i], _mm_setzero_ps());
        }
#endif
#endif
      }
      for(;i<rays.rayEnd;i++){
        Real dt=Dot(rays.getDirection(i), normal);
        if(Abs(dt) < (Real)1.e-6)
          continue;
        Real t=num/dt;
        if(t>rays.getMinT(i))
          continue;
        Vector vi(rays.getDirection(i)*t);
        Real a1 = Dot(v1, vi)+o1;
        if (a1 < 0 || a1 > 1)
          continue;
        Real a2 = Dot(v2, vi)+o2;
        if (a2 < 0 || a2 > 1)
          continue;

        if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
        }
      }
    }
#else
    for(int i = rays.begin(); i < rays.end(); i++){
      Real dt=Dot(rays.getDirection(i), normal);
      if(Abs(dt) < (Real)1.e-6)
        continue;
      Real t=num/dt;
      if(t>rays.getMinT(i))
        continue;
      Vector vi(rays.getDirection(i)*t);
      Real a1 = Dot(v1, vi)+o1;
      if (a1 < 0 || a1 > 1)
        continue;
      Real a2 = Dot(v2, vi)+o2;
      if (a2 < 0 || a2 > 1)
        continue;

      if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
      }
    }
#endif
  } else {
#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i=rays.rayBegin;i<rays.rayEnd;i++){
        Vector dir = rays.getDirection(i);
        Real dt=Dot(dir, normal);
        if(Abs(dt) < (Real)1.e-6)
          continue;
        Vector origin = rays.getOrigin(i);
        Real t=(d-Dot(normal, origin))/dt;
        if(t>rays.getMinT(i))
          continue;
        Vector p(origin+dir*t);
        Vector vi(p-anchor);
        Real a1 = Dot(v1, vi);
        if (a1 < 0 || a1 > 1)
          continue;
        Real a2 = Dot(v2, vi);
        if (a2 < 0 || a2 > 1)
          continue;

        if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
        }
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        Vector dir = rays.getDirection(i);
        Real dt=Dot(dir, normal);
        if(Abs(dt) < (Real)1.e-6)
          continue;
        Vector origin = rays.getOrigin(i);
        Real t=(d-Dot(normal, origin))/dt;
        if(t>rays.getMinT(i))
          continue;
        Vector p(origin+dir*t);
        Vector vi(p-anchor);
        Real a1 = Dot(v1, vi);
        if (a1 < 0 || a1 > 1)
          continue;
        Real a2 = Dot(v2, vi);
        if (a2 < 0 || a2 > 1)
          continue;

        if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
        }
      }
      RayPacketData* data = rays.data;
#if USE_SCRATCHPAD == 2
      float* scratch1 = rays.getScratchpad<float>(0);
      float* scratch2 = rays.getScratchpad<float>(1);
#endif
      __m128 normalx = _mm_set1_ps(normal[0]);
      __m128 normaly = _mm_set1_ps(normal[1]);
      __m128 normalz = _mm_set1_ps(normal[2]);
      for(;i<e;i+=4){
        __m128 dx = _mm_load_ps(&data->direction[0][i]);
        __m128 dy = _mm_load_ps(&data->direction[1][i]);
        __m128 dz = _mm_load_ps(&data->direction[2][i]);
        __m128 dt = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, normalx), _mm_mul_ps(dy, normaly)), _mm_mul_ps(dz, normalz));

        //if(Abs(dt) < (Real)1.e-6)
        //continue;
        __m128 ox = _mm_load_ps(&data->origin[0][i]);
        __m128 oy = _mm_load_ps(&data->origin[1][i]);
        __m128 oz = _mm_load_ps(&data->origin[2][i]);
        __m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ox, normalx), _mm_mul_ps(oy, normaly)), _mm_mul_ps(oz, normalz));
        __m128 t = _mm_div_ps(_mm_sub_ps(_mm_set1_ps(d), dot), dt);

        __m128 hit = _mm_and_ps(_mm_cmplt_ps(t, _mm_load_ps(&data->minT[i])),
                                _mm_cmpgt_ps(t, _mm_set1_ps(T_EPSILON)));
        if(_mm_movemask_ps(hit) == 0)
          continue;

        __m128 px = _mm_add_ps(ox, _mm_mul_ps(dx, t));
        __m128 py = _mm_add_ps(oy, _mm_mul_ps(dy, t));
        __m128 pz = _mm_add_ps(oz, _mm_mul_ps(dz, t));

        __m128 vix = _mm_sub_ps(px, _mm_set1_ps(anchor[0]));
        __m128 viy = _mm_sub_ps(py, _mm_set1_ps(anchor[1]));
        __m128 viz = _mm_sub_ps(pz, _mm_set1_ps(anchor[2]));

        __m128 a1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(vix, _mm_set1_ps(v1[0])), _mm_mul_ps(viy, _mm_set1_ps(v1[1]))), _mm_mul_ps(viz, _mm_set1_ps(v1[2])));
        __m128 zero = _mm_setzero_ps();
        __m128 one = _mm_set1_ps(1.0f);
        hit = _mm_and_ps(hit, _mm_and_ps(_mm_cmpge_ps(a1, zero), _mm_cmple_ps(a1, one)));
        if(_mm_movemask_ps(hit) == 0)
          continue;

        __m128 a2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(vix, _mm_set1_ps(v2[0])), _mm_mul_ps(viy, _mm_set1_ps(v2[1]))), _mm_mul_ps(viz, _mm_set1_ps(v2[2])));
        hit = _mm_and_ps(hit, _mm_and_ps(_mm_cmpge_ps(a2, zero), _mm_cmple_ps(a2, one)));
        if(_mm_movemask_ps(hit) == 0)
          continue;

        rays.hitWithoutTminCheck(i, hit, t, getMaterial(), this, getTexCoordMapper());

#if USE_SCRATCHPAD == 1
        // Copy the barycentric coordinates to the scratch pad
        MANTA_ALIGN(16) float ra1[4];
        MANTA_ALIGN(16) float ra2[4];
        _mm_store_ps(ra1, a1);
        _mm_store_ps(ra2, a2);

        if(_mm_movemask_ps(hit) == 15){
          rays.scratchpad<Vector>(i+0) = Vector(ra1[0], ra2[0], 0);
          rays.scratchpad<Vector>(i+1) = Vector(ra1[1], ra2[1], 0);
          rays.scratchpad<Vector>(i+2) = Vector(ra1[2], ra2[2], 0);
          rays.scratchpad<Vector>(i+3) = Vector(ra1[3], ra2[3], 0);
        } else {
          int hit_mask = _mm_movemask_ps(hit);
          for(int j = 0; j < 4; ++j) {
            if (hit_mask & (1 << j))
              rays.scratchpad<Vector>(i+j) = Vector(ra1[j], ra2[j], 0);
          }
        }
#else
#if USE_SCRATCHPAD == 2
        if (_mm_movemask_ps(hit) == 15) {
          _mm_store_ps(&scratch1[i], a1);
          _mm_store_ps(&scratch2[i], a2);
        } else {
          maskedStore_ps(hit, (float*)&scratch1[i], a1);
          maskedStore_ps(hit, (float*)&scratch2[i], a2);
        }
#else
        if (_mm_movemask_ps(hit) == 15) {
          _mm_store_ps(&data->texCoords[0][i], a1);
          _mm_store_ps(&data->texCoords[1][i], a2);
          _mm_store_ps(&data->texCoords[2][i], _mm_setzero_ps());
        } else {
          maskedStore_ps(hit, (float*)&data->texCoords[0][i], a1);
          maskedStore_ps(hit, (float*)&data->texCoords[1][i], a2);
          maskedStore_ps(hit, (float*)&data->texCoords[2][i], _mm_setzero_ps());
        }
#endif
#endif
      }
      for(;i<rays.rayEnd;i++){
        Vector dir = rays.getDirection(i);
        Real dt=Dot(dir, normal);
        if(Abs(dt) < (Real)1.e-6)
          continue;
        Vector origin = rays.getOrigin(i);
        Real t=(d-Dot(normal, origin))/dt;
        if(t>rays.getMinT(i))
          continue;
        Vector p(origin+dir*t);
        Vector vi(p-anchor);
        Real a1 = Dot(v1, vi);
        if (a1 < 0 || a1 > 1)
          continue;
        Real a2 = Dot(v2, vi);
        if (a2 < 0 || a2 > 1)
          continue;

        if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
        }
      }
    }
#else
    for(int i = rays.begin(); i < rays.end(); i++){
      Vector dir = rays.getDirection(i);
      Real dt=Dot(dir, normal);
      if(Abs(dt) < (Real)1.e-6)
        continue;
      Vector origin = rays.getOrigin(i);
      Real t=(d-Dot(normal, origin))/dt;
      if(t>rays.getMinT(i))
        continue;
      Vector p(origin+dir*t);
      Vector vi(p-anchor);
      Real a1 = Dot(v1, vi);
      if (a1 < 0 || a1 > 1)
        continue;
      Real a2 = Dot(v2, vi);
      if (a2 < 0 || a2 > 1)
        continue;

      if(rays.hit(i, t, getMaterial(), this, getTexCoordMapper())){
#if USE_SCRATCHPAD == 1
          rays.scratchpad<Vector>(i) = Vector(a1, a2, 0);
#else
#if USE_SCRATCHPAD == 2
          rays.getScratchpad<Real>(0)[i] = a1;
          rays.getScratchpad<Real>(1)[i] = a2;
#else
          rays.setTexCoords(i, Vector(a1, a2, 0));
#endif
#endif
      }
    }
#endif
  }
}

void Parallelogram::computeTexCoords2(const RenderContext&,
                                      RayPacket& rays) const
{
#if USE_SCRATCHPAD == 1
  for(int i=rays.begin();i<rays.end();i++){
    rays.setTexCoords(i, rays.scratchpad<Vector>(i));
  }
#else
#if USE_SCRATCHPAD == 2
  float* a1 = rays.getScratchpad<float>(0);
  float* a2 = rays.getScratchpad<float>(1);
  for(int i=rays.begin();i<rays.end();i++){
    rays.setTexCoords(i, Vector(a1[i], a2[i], 0));
  }
#endif
#endif
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void Parallelogram::computeTexCoords3(const RenderContext&,
                                      RayPacket& rays) const
{
#if USE_SCRATCHPAD == 1
  for(int i=rays.begin();i<rays.end();i++){
    rays.setTexCoords(i, rays.scratchpad<Vector>(i));
  }
#else
#if USE_SCRATCHPAD == 2
  float* a1 = rays.getScratchpad<float>(0);
  float* a2 = rays.getScratchpad<float>(1);
  for(int i=rays.begin();i<rays.end();i++){
    rays.setTexCoords(i, Vector(a1[i], a2[i], 0));
  }
#endif
#endif
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void Parallelogram::getRandomPoints(Packet<Vector>& points,
                                    Packet<Vector>& normals,
                                    Packet<Real>& pdfs,
                                    const RenderContext& context,
                                    RayPacket& rays) const {
  // TODO(boulos): Change this code to only do work for active rays
  Packet<Real> r1;
  Packet<Real> r2;
  context.sample_generator->nextSeeds(context, r1, rays);
  context.sample_generator->nextSeeds(context, r2, rays);

#ifdef MANTA_SSE
  for (int i = 0; i < Packet<Vector>::MaxSize; i+=4) {
    _mm_store_ps(&points.vectordata[0][i],
                 _mm_add_ps(_mm_set1_ps(anchor[0]),
                            _mm_add_ps(_mm_mul_ps(_mm_load_ps(&r1.data[i]),
                                                  _mm_set1_ps(v1_unscaled[0])),
                                       _mm_mul_ps(_mm_load_ps(&r2.data[i]),
                                                  _mm_set1_ps(v2_unscaled[0])))));
    _mm_store_ps(&points.vectordata[1][i],
                 _mm_add_ps(_mm_set1_ps(anchor[1]),
                            _mm_add_ps(_mm_mul_ps(_mm_load_ps(&r1.data[i]),
                                                  _mm_set1_ps(v1_unscaled[1])),
                                       _mm_mul_ps(_mm_load_ps(&r2.data[i]),
                                                  _mm_set1_ps(v2_unscaled[1])))));
    _mm_store_ps(&points.vectordata[2][i],
                 _mm_add_ps(_mm_set1_ps(anchor[2]),
                            _mm_add_ps(_mm_mul_ps(_mm_load_ps(&r1.data[i]),
                                                  _mm_set1_ps(v1_unscaled[2])),
                                       _mm_mul_ps(_mm_load_ps(&r2.data[i]),
                                                  _mm_set1_ps(v2_unscaled[2])))));

    _mm_store_ps(&normals.vectordata[0][i],
                 _mm_set1_ps(this->normal[0]));
    _mm_store_ps(&normals.vectordata[1][i],
                 _mm_set1_ps(this->normal[1]));
    _mm_store_ps(&normals.vectordata[2][i],
                 _mm_set1_ps(this->normal[2]));

    _mm_store_ps(&pdfs.data[i], _mm_set1_ps(inv_area));
  }
#else
  for (int i = 0; i < Packet<Vector>::MaxSize; i++) {
    points.set(i, anchor + r1.get(i) * v1_unscaled + r2.get(i) * v2_unscaled);
  }
  for (int i = 0; i < Packet<Vector>::MaxSize; i++) {
    normals.set(i, this->normal);
  }
  for (int i = 0; i < Packet<Real>::MaxSize; i++) {
    pdfs.set(i, inv_area);
  }
#endif
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS2(Parallelogram, PrimitiveCommon, TexCoordMapper, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(Parallelogram);
}

void Parallelogram::readwrite(ArchiveElement* archive)
{
  MantaRTTI<PrimitiveCommon>::readwrite(archive, *this);
  MantaRTTI<TexCoordMapper>::readwrite(archive, *this);
  if(archive->reading() && archive->hasField("p0") && archive->hasField("p1") && archive->hasField("p2")){
    Vector p0, p1, p2;
    archive->readwrite("p0", p0);
    archive->readwrite("p1", p1);
    archive->readwrite("p2", p2);
    changeGeometry(p0, p1-p0, p2-p0);
  } else {
    archive->readwrite("anchor", anchor);
    archive->readwrite("v1", v1);
    archive->readwrite("v2", v2);
    archive->readwrite("normal", normal);
    archive->readwrite("offset", d);
    if(archive->reading()){
      double len = normal.normalize();
      d *= len;
    }
  }
}
