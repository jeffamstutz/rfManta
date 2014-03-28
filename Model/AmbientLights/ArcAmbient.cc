
#include <Model/AmbientLights/ArcAmbient.h>
#include <Interface/RayPacket.h>
#include <Interface/InterfaceRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Math/Trig.h>
#include <sstream>


using namespace Manta;
using namespace std;

ArcAmbient::ArcAmbient()
{
}

ArcAmbient::ArcAmbient(const Color& cup, const Color& cdown,
                       const Vector& up)
  : cup(cup), cdown(cdown), up(up)
{
}

ArcAmbient::~ArcAmbient()
{
}

void ArcAmbient::preprocess(const PreprocessContext&)
{
}

void ArcAmbient::computeAmbient(const RenderContext& context,
                                RayPacket& rays,
                                ColorArray ambient) const
{
  rays.computeNormals<true>(context);
  for(int i=rays.begin();i<rays.end();i++){
    Real cosine = Dot(rays.getNormal(i), up);
    Real sine = 1-cosine*cosine;
    // This makes this safe from cases where your normal is too "long"
    // and cosine is greater than 1.  In that case you end up with
    // sine being NaN, which really sucks.
    sine = (sine > 0) ? Sqrt(sine): 0;
    // So we want to do the computation for w0 and w1 as type Real,
    // because that is what all the other computation will be done,
    // but we would like to cast that to type ColorComponent, because
    // we will do operations on the color with this value.
    ColorComponent w0, w1;
    if(cosine > 0){
      w0= sine/2;
      w1= (1 -  w0);
    } else {
      w1= sine/2;
      w0= (1 -  w1);
    }
    Color ambientLight = cup*w1 + cdown*w0;
    for(int j=0;j<Color::NumComponents;j++)
      ambient[j][i] = ambientLight[j];
  }
}

string ArcAmbient::toString() const {
  ostringstream out;
  out << "--------  ArcAmbient  ---------\n";
  RGB cu(cup.convertRGB()), cd(cdown.convertRGB());
  out << "(cup, cdown) = ";
  out << "("<<cu.r()<<", "<<cu.g()<<", "<<cu.b()<<"), ";
  out << "("<<cd.r()<<", "<<cd.g()<<", "<<cd.b()<<")\n";
  out << "up = "<<up<<"\n";
  return out.str();
}

namespace Manta {
MANTA_DECLARE_RTTI_DERIVEDCLASS(ArcAmbient, AmbientLight, ConcreteClass, readwriteMethod);
MANTA_REGISTER_CLASS(ArcAmbient);
}

void ArcAmbient::readwrite(ArchiveElement* archive)
{
  MantaRTTI<AmbientLight>::readwrite(archive, *this);
  archive->readwrite("cup", cup);
  archive->readwrite("cdown", cdown);
  archive->readwrite("up", up);
}
