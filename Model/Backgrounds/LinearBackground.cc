
#include <Model/Backgrounds/LinearBackground.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/RayPacket.h>
#include <MantaTypes.h>

using namespace Manta;

LinearBackground::LinearBackground()
{
}

LinearBackground::LinearBackground(const Color& cup, const Color& cdown,
				   const Vector& in_up)
  : cup(cup), cdown(cdown), up(in_up)
{
  up.normalize();
}

LinearBackground::~LinearBackground()
{
}

void LinearBackground::preprocess(const PreprocessContext&)
{
}

void LinearBackground::shade(const RenderContext&, RayPacket& rays) const
{
  rays.normalizeDirections();
  for(int i=rays.begin();i<rays.end();i++){
    // So we want to do the computation for t as type Real, because
    // that is what all the other computation will be done, but we
    // would like to cast that to type ColorComponent, because we will
    // do operations on the color with this value.
    ColorComponent t = (Real)0.5 * (1 + Dot(rays.getDirection(i), up));
    rays.setColor(i, cup*t+cdown*(1-t));
  }
}

namespace Manta {
MANTA_DECLARE_RTTI_DERIVEDCLASS(LinearBackground, Background, ConcreteClass, readwriteMethod);
MANTA_REGISTER_CLASS(LinearBackground);
}

void LinearBackground::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Background>::readwrite(archive, *this);
  archive->readwrite("cup", cup);
  archive->readwrite("cdown", cdown);
  archive->readwrite("up", up);
}
