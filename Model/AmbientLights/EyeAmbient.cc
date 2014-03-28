#include <Model/AmbientLights/EyeAmbient.h>
#include <Interface/RayPacket.h>
#include <Interface/InterfaceRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <MantaSSE.h>
#include <sstream>

using namespace Manta;
using namespace std;

EyeAmbient::EyeAmbient()
{
}

EyeAmbient::EyeAmbient(const Color& color)
  : color(color)
{
}

EyeAmbient::~EyeAmbient()
{
}

void EyeAmbient::preprocess(const PreprocessContext&)
{
}

void EyeAmbient::computeAmbient(const RenderContext& context,
                                     RayPacket& rays,
                                     ColorArray ambient) const
{
  static const Real minLighting = static_cast<Real>(0.5);
  rays.normalizeDirections();
  rays.computeFFNormals<true>(context);
  for(int i = rays.begin(); i < rays.end(); i++) {
    const Vector normal = rays.getFFNormal(i);
    const Vector eyeDir = rays.getDirection(i);
    ColorComponent cos_theta = -Dot(eyeDir, normal)*(1-minLighting) + minLighting ;
    for(int k = 0; k < Color::NumComponents;k++)
      ambient[k][i] = color[k]*cos_theta;
  }
  return;
}

string EyeAmbient::toString() const {
  ostringstream out;
  out << "--------  EyeAmbient  ---------\n";
  RGB c(color.convertRGB());
  out << "color = ("<<c.r()<<", "<<c.g()<<", "<<c.b()<<")\n";
  return out.str();
}

namespace Manta {
MANTA_DECLARE_RTTI_DERIVEDCLASS(EyeAmbient, AmbientLight, ConcreteClass, readwriteMethod);
MANTA_REGISTER_CLASS(EyeAmbient);
}

void EyeAmbient::readwrite(ArchiveElement* archive)
{
  MantaRTTI<AmbientLight>::readwrite(archive, *this);
  archive->readwrite("color", color);
}
