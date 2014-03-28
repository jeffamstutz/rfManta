#include <Model/Lights/AreaLight.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Interface/InterfaceRTTI.h>
#include <MantaSSE.h>
#include <Interface/Primitive.h>
#include <iostream>

using namespace Manta;

AreaLight::AreaLight()
{
}

AreaLight::AreaLight(Primitive* primitive, const Color& color)
  : primitive(primitive), color(color) {
}

AreaLight::~AreaLight() {
}

void AreaLight::preprocess(const PreprocessContext&) {
}

void AreaLight::computeLight(RayPacket& destRays, const RenderContext &context,
                             RayPacket& sourceRays) const
{
  sourceRays.computeHitPositions();
  Packet<Vector> positions;
  Packet<Vector> normals;
  Packet<Real> pdfs;
  primitive->getRandomPoints(positions, normals, pdfs, context, sourceRays);

#ifdef MANTA_SSE
  int b = (sourceRays.rayBegin + 3) & (~3);
  int e = sourceRays.rayEnd & (~3);
  if(b >= e){
    for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
      // Generate a point on the light
      Vector dir = positions.get(i) - sourceRays.getHitPosition(i);
      Real len = dir.normalize();
      Real cosine = -Dot(dir, normals.get(i));
      destRays.setDirection(i, dir);
      destRays.overrideMinT(i, len - T_EPSILON);
      if (cosine <= Real(0)) {
        // Wrong side of the light or we shouldn't count anyway
        destRays.setColor(i, Color::black());
      } else {
        // pdf = (dist^2 / cosine) * inv_area, which we assume is in
        // pdfs(i)
        Real solid_pdf = pdfs.get(i) * (len * len);
        destRays.setColor(i, (cosine / solid_pdf) * color);
      }
    }
  } else {
    int i = sourceRays.rayBegin;
    for(;i<b;i++){
      // Generate a point on the light
      Vector dir = positions.get(i) - sourceRays.getHitPosition(i);
      Real len = dir.normalize();
      Real cosine = -Dot(dir, normals.get(i));
      destRays.setDirection(i, dir);
      destRays.overrideMinT(i, len - T_EPSILON);
      if (cosine <= Real(0)) {
        // Wrong side of the light
        destRays.setColor(i, Color::black());
      } else {
        // pdf = (dist^2 / cosine) * inv_area, which we assume is in
        // pdfs(i)
        Real solid_pdf = pdfs.get(i) * (len * len);
        destRays.setColor(i, (cosine / solid_pdf) * color);
      }
    }
    RayPacketData* sourceData = sourceRays.data;
    RayPacketData* destData = destRays.data;
    for(;i<e;i+=4){

      __m128 dir_x = _mm_sub_ps(_mm_load_ps(&positions.vectordata[0][i]),
                                _mm_load_ps(&sourceData->hitPosition[0][i]));
      __m128 dir_y = _mm_sub_ps(_mm_load_ps(&positions.vectordata[1][i]),
                                _mm_load_ps(&sourceData->hitPosition[1][i]));
      __m128 dir_z = _mm_sub_ps(_mm_load_ps(&positions.vectordata[2][i]),
                                _mm_load_ps(&sourceData->hitPosition[2][i]));

      __m128 len2 = dot4(dir_x, dir_y, dir_z,
                         dir_x, dir_y, dir_z);
      __m128 len = _mm_sqrt_ps(len2);

      dir_x = _mm_div_ps(dir_x, len);
      dir_y = _mm_div_ps(dir_y, len);
      dir_z = _mm_div_ps(dir_z, len);

      __m128 cosine = dot4(dir_x, dir_y, dir_z,
                           _mm_load_ps(&normals.vectordata[0][i]),
                           _mm_load_ps(&normals.vectordata[1][i]),
                           _mm_load_ps(&normals.vectordata[2][i]));
      cosine = xor4(cosine, _mm_signbit);

      _mm_store_ps(&destData->direction[0][i],
                   dir_x);
      _mm_store_ps(&destData->direction[1][i],
                   dir_y);
      _mm_store_ps(&destData->direction[2][i],
                   dir_z);

      _mm_store_ps(&destData->minT[i], _mm_sub_ps(len, _mm_set1_ps(T_EPSILON)));

      // If the cosine < 0 || ignoreEmittedLight != 0
      __m128 black_light = _mm_cmple_ps(cosine, _mm_setzero_ps());

      if (_mm_movemask_ps(black_light) == 0xf) {
        // All black
        _mm_store_ps(&destData->color[0][i], _mm_setzero_ps());
        _mm_store_ps(&destData->color[1][i], _mm_setzero_ps());
        _mm_store_ps(&destData->color[2][i], _mm_setzero_ps());
      } else {
        __m128 solid_pdf = _mm_div_ps(_mm_mul_ps(_mm_load_ps(&pdfs.data[i]),
                                                 len2),
                                      cosine);
        for (unsigned int c = 0; c < 3; c++) {
          _mm_store_ps(&destData->color[c][i],
                       _mm_andnot_ps(black_light, _mm_div_ps(_mm_set1_ps(color[c]), solid_pdf)));
        }
      }
    }
    for(;i<sourceRays.rayEnd;i++){
      // Generate a point on the light
      Vector dir = positions.get(i) - sourceRays.getHitPosition(i);
      Real len = dir.normalize();
      Real cosine = -Dot(dir, normals.get(i));
      destRays.setDirection(i, dir);
      destRays.overrideMinT(i, len - T_EPSILON);
      if (cosine <= Real(0)) {
        // Wrong side of the light
        destRays.setColor(i, Color::black());
      } else {
        // pdf = (dist^2 / cosine) * inv_area, which we assume is in
        // pdfs(i)
        Real solid_pdf = pdfs.get(i) * (len * len);
        destRays.setColor(i, (cosine / solid_pdf) * color);
      }
    }
  }
#else
  for(int i = sourceRays.begin(); i < sourceRays.end(); i++) {
    // Generate a point on the light
    Vector dir = positions.get(i) - sourceRays.getHitPosition(i);
    Real len = dir.normalize();
    Real cosine = -Dot(dir, normals.get(i));
    destRays.setDirection(i, dir);
    destRays.overrideMinT(i, len - T_EPSILON);
    if (cosine <= Real(0)) {
      // Wrong side of the light
      destRays.setColor(i, Color::black());
    } else {
      // pdf = (dist^2 / cosine) * inv_area, which we assume is in
      // pdfs(i)
      Real solid_pdf = pdfs.get(i) * (len * len);
      destRays.setColor(i, (cosine / solid_pdf) * color);
    }
  }
#endif
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(AreaLight, Light, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(AreaLight);
}

void AreaLight::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Light>::readwrite(archive, *this);
  archive->readwrite("primitive", primitive);
  archive->readwrite("color", color);
}
