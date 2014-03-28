
#include <Model/Backgrounds/ConstantBackground.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/RayPacket.h>
#include <MantaSSE.h>

using namespace Manta;

ConstantBackground::ConstantBackground(const Color& color)
  : bgcolor(color)
{
}

ConstantBackground::ConstantBackground()
{
}

ConstantBackground::~ConstantBackground()
{
}

void ConstantBackground::preprocess(const PreprocessContext&)
{
}

Color ConstantBackground::getValue() const {
  return bgcolor;
}

void ConstantBackground::setValue(Color new_color) {
  bgcolor = new_color;
}

void ConstantBackground::shade(const RenderContext&, RayPacket& rays) const
{
#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        rays.setColor(i, bgcolor);
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        rays.setColor(i, bgcolor);
      }
      RayPacketData* data = rays.data;
      for(;i<e;i+=4){
        _mm_store_ps(&data->color[0][i], _mm_set1_ps(bgcolor[0]));
        _mm_store_ps(&data->color[1][i], _mm_set1_ps(bgcolor[1]));
        _mm_store_ps(&data->color[2][i], _mm_set1_ps(bgcolor[2]));
      }
      for(;i<rays.rayEnd;i++){
        rays.setColor(i, bgcolor);
      }
    }
#else
    for(int i = rays.begin(); i < rays.end(); i++){
      rays.setColor(i, bgcolor);
    }
#endif
}

namespace Manta {
MANTA_DECLARE_RTTI_DERIVEDCLASS(ConstantBackground, Background, ConcreteClass, readwriteMethod);
MANTA_REGISTER_CLASS(ConstantBackground);
}

void ConstantBackground::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Background>::readwrite(archive, *this);
  archive->readwrite("color", bgcolor);
}
