
#include <Engine/Shadows/NoDirect.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/RayPacket.h>

using namespace Manta;

ShadowAlgorithm* NoDirect::create(const vector<string>& args)
{
  return new NoDirect(args);
}

NoDirect::NoDirect()
{
}

NoDirect::NoDirect(const vector<string>& /*args*/)
{
}

NoDirect::~NoDirect()
{
}

void NoDirect::computeShadows(const RenderContext& context, StateBuffer& stateBuffer,
                               const LightSet* lights, RayPacket& sourceRays, RayPacket& shadowRays)
{
  shadowRays.resize(0, 0);
  stateBuffer.state = StateBuffer::Finished;
}

string NoDirect::getName() const {
  return "nodirect";
}

string NoDirect::getSpecs() const {
  return "none";
}

