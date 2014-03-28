
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Interface/RayPacket.h>
#include <Interface/InterfaceRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <MantaSSE.h>
#include <sstream>

using namespace Manta;
using namespace std;

ConstantAmbient::ConstantAmbient()
{
}

ConstantAmbient::ConstantAmbient(const Color& color)
  : color(color)
{
}

ConstantAmbient::~ConstantAmbient()
{
}

void ConstantAmbient::preprocess(const PreprocessContext&)
{
}

void ConstantAmbient::computeAmbient(const RenderContext&,
                                     RayPacket& rays,
                                     ColorArray ambient) const
{
#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        for(int j=0;j<Color::NumComponents;j++)
          ambient[j][i] = color[j];
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        for(int j=0;j<Color::NumComponents;j++)
          ambient[j][i] = color[j];
      }
      for(;i<e;i+=4){
        _mm_store_ps(&ambient[0][i], _mm_set1_ps(color[0]));
        _mm_store_ps(&ambient[1][i], _mm_set1_ps(color[1]));
        _mm_store_ps(&ambient[2][i], _mm_set1_ps(color[2]));
      }
      for(;i<rays.rayEnd;i++){
        for(int j=0;j<Color::NumComponents;j++)
          ambient[j][i] = color[j];
      }
    }
#else
    for(int i = rays.begin(); i < rays.end(); i++){
      for(int j=0;j<Color::NumComponents;j++)
        ambient[j][i] = color[j];
    }
#endif
}

string ConstantAmbient::toString() const {
  ostringstream out;
  out << "--------  ConstantAmbient  ---------\n";
  RGB c(color.convertRGB());
  out << "color = ("<<c.r()<<", "<<c.g()<<", "<<c.b()<<")\n";
  return out.str();
}

namespace Manta {
MANTA_DECLARE_RTTI_DERIVEDCLASS(ConstantAmbient, AmbientLight, ConcreteClass, readwriteMethod);
MANTA_REGISTER_CLASS(ConstantAmbient);
}

void ConstantAmbient::readwrite(ArchiveElement* archive)
{
  MantaRTTI<AmbientLight>::readwrite(archive, *this);
  archive->readwrite("color", color);
}
