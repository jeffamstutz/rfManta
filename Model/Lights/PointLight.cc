
#include <Model/Lights/PointLight.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Interface/InterfaceRTTI.h>
#include <MantaSSE.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/PrimaryRaysOnly.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <iostream>
using namespace std;
using namespace Manta;

PointLight::PointLight()
{
}

PointLight::PointLight(const Vector& position, const Color& color)
  : position(position), color(color)
{
}

PointLight::~PointLight()
{
}

void PointLight::preprocess(const PreprocessContext&)
{
}

void PointLight::computeLight(RayPacket& destRays, const RenderContext &context,
                              RayPacket& sourceRays) const
{
  sourceRays.computeHitPositions();
#ifdef MANTA_SSE
    int b = (sourceRays.rayBegin + 3) & (~3);
    int e = sourceRays.rayEnd & (~3);
    if(b >= e){
      for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
        destRays.setColor(i, color);
        destRays.setDirection(i, position - sourceRays.getHitPosition(i));
        destRays.overrideMinT(i, 1 - T_EPSILON);
      }
    } else {
      int i = sourceRays.rayBegin;
      for(;i<b;i++){
        destRays.setColor(i, color);
        destRays.setDirection(i, position - sourceRays.getHitPosition(i));
        destRays.overrideMinT(i, 1 - T_EPSILON);
      }
      RayPacketData* sourceData = sourceRays.data;
      RayPacketData* destData = destRays.data;
      for(;i<e;i+=4){
        _mm_store_ps(&destData->color[0][i], _mm_set1_ps(color[0]));
        _mm_store_ps(&destData->color[1][i], _mm_set1_ps(color[1]));
        _mm_store_ps(&destData->color[2][i], _mm_set1_ps(color[2]));
        _mm_store_ps(&destData->direction[0][i], _mm_sub_ps(_mm_set1_ps(position[0]), _mm_load_ps(&sourceData->hitPosition[0][i])));
        _mm_store_ps(&destData->direction[1][i], _mm_sub_ps(_mm_set1_ps(position[1]), _mm_load_ps(&sourceData->hitPosition[1][i])));
        _mm_store_ps(&destData->direction[2][i], _mm_sub_ps(_mm_set1_ps(position[2]), _mm_load_ps(&sourceData->hitPosition[2][i])));
        _mm_store_ps(&destData->minT[i], _mm_set1_ps(1 - T_EPSILON));
      }
      for(;i<sourceRays.rayEnd;i++){
        destRays.setColor(i, color);
        destRays.setDirection(i, position - sourceRays.getHitPosition(i));
        destRays.overrideMinT(i, 1 - T_EPSILON);
      }
    }
#else
    for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
      destRays.setColor(i, color);
      destRays.setDirection(i, position - sourceRays.getHitPosition(i));
      destRays.overrideMinT(i, 1 - T_EPSILON);
    }
#endif
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(PointLight, Light, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(PointLight);
}

void PointLight::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Light>::readwrite(archive, *this);
  archive->readwrite("position", position);
  archive->readwrite("color", color);
}

