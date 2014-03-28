
#include <Model/Lights/DirectionalLight.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Interface/InterfaceRTTI.h>
#include <MantaSSE.h>

using namespace Manta;

DirectionalLight::DirectionalLight()
{
}

DirectionalLight::DirectionalLight(const Vector& direction, const Color& color)
  : direction(direction.normal()), color(color)
{
}

DirectionalLight::~DirectionalLight()
{
}

void DirectionalLight::preprocess(const PreprocessContext&)
{
}

void DirectionalLight::computeLight(RayPacket& destRays,
                                    const RenderContext &context,
                                    RayPacket& sourceRays) const
{
  // Don't need hit positions
  //sourceRays.computeHitPositions();
#ifdef MANTA_SSE
    int b = (sourceRays.rayBegin + 3) & (~3);
    int e = sourceRays.rayEnd & (~3);
    if(b >= e){
      for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
        destRays.setColor(i, color);
        destRays.setDirection(i, direction);
        destRays.overrideMinT(i, MAXT);
      }
    } else {
      int i = sourceRays.rayBegin;
      for(;i<b;i++){
        destRays.setColor(i, color);
        destRays.setDirection(i, direction);
        destRays.overrideMinT(i, MAXT);
      }
      //RayPacketData* sourceData = sourceRays.data;
      RayPacketData* destData = destRays.data;
      for(;i<e;i+=4){
        _mm_store_ps(&destData->color[0][i], _mm_set1_ps(color[0]));
        _mm_store_ps(&destData->color[1][i], _mm_set1_ps(color[1]));
        _mm_store_ps(&destData->color[2][i], _mm_set1_ps(color[2]));
        _mm_store_ps(&destData->direction[0][i], _mm_set1_ps(direction[0]));
        _mm_store_ps(&destData->direction[1][i], _mm_set1_ps(direction[1]));
        _mm_store_ps(&destData->direction[2][i], _mm_set1_ps(direction[2]));
        _mm_store_ps(&destData->minT[i], _mm_set1_ps(MAXT));
      }
      for(;i<sourceRays.rayEnd;i++){
        destRays.setColor(i, color);
        destRays.setDirection(i, direction);
        destRays.overrideMinT(i, MAXT);
      }
    }
#else
    for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
      destRays.setColor(i, color);
      destRays.setDirection(i, direction);
      destRays.overrideMinT(i, MAXT);
    }
#endif
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(DirectionalLight, Light, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(DirectionalLight);
}

void DirectionalLight::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Light>::readwrite(archive, *this);
  archive->readwrite("direction", direction);
  archive->readwrite("color", color);
}
