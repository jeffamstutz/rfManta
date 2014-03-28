
#include <Model/Materials/LitMaterial.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/LightSet.h>
#include <Interface/Context.h>
#include <Core/Persistent/ArchiveElement.h>

using namespace Manta;

LitMaterial::LitMaterial()
  : activeLights(0), localLights(0), localLightsOverrideGlobal(false)
{
}

LitMaterial::~LitMaterial()
{
}

void LitMaterial::preprocess(const PreprocessContext& context)
{
  if (context.proc != 0) {
    context.done();
    return;
  }
  PreprocessContext serial_context(context);
  serial_context.numProcs = 1;

  if(localLights){
    localLights->preprocess(serial_context);
    if(localLightsOverrideGlobal){
      activeLights = localLights;
    } else {
      activeLights = LightSet::merge(localLights, serial_context.globalLights);
    }
  } else {
    activeLights = serial_context.globalLights;
  }
  context.done();
}

namespace Manta {
  MANTA_REGISTER_CLASS(LitMaterial);
}

void LitMaterial::readwrite(ArchiveElement* archive)
{
  MantaRTTI<OpaqueShadower>::readwrite(archive, *this);
  if(archive->hasField("localLights")){
    archive->readwrite("localLights", localLights);
  } else {
    localLights = 0;
  }
  if(archive->hasField("localLightsOverrideGlobal")){
    archive->readwrite("localLightsOverrideGlobal", localLightsOverrideGlobal);
  } else {
    localLightsOverrideGlobal = false;
  }
}
