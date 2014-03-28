
#include <Engine/Shadows/NoShadows.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/RayPacket.h>

using namespace Manta;

ShadowAlgorithm* NoShadows::create(const vector<string>& args)
{
  return new NoShadows(args);
}

NoShadows::NoShadows()
{
}

NoShadows::NoShadows(const vector<string>& /*args*/)
{
}

NoShadows::~NoShadows()
{
}

void NoShadows::computeShadows(const RenderContext& context, StateBuffer& stateBuffer,
                               const LightSet* lights, RayPacket& sourceRays, RayPacket& shadowRays)
{
  int nlights = lights->numLights();

  // Compute the hit positions.
  sourceRays.computeHitPositions();
  sourceRays.computeFFNormals<true>( context );

  int j;
  if(stateBuffer.state == StateBuffer::Finished){
    return; // Shouldn't happen, but just in case...
  } else if(stateBuffer.state == StateBuffer::Start){
    j = 0;
  } else {
    // Continuing
    j = stateBuffer.i1;
  }

  // Compute the contribution for this light.
  int first = -1;
  int last = -1;
  do {
    lights->getLight(j)->computeLight(shadowRays, context, sourceRays);

    for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
      // Check to see if the light is on the front face.
      Vector dir = shadowRays.getDirection(i);
      if(Dot(dir, sourceRays.getFFNormal(i)) > 0) {
        // This is a version of resetHit that only sets the material
        // to NULL and doesn't require us to also modify the hit
        // distance which was set for us by the call to computeLight.
        shadowRays.setHitMaterial(i, NULL);

        last = i;
        if (first < 0)
          first = i;
      } else {
        shadowRays.maskRay(i);
      }
    }
    j++;
  } while(last == -1 && j < nlights);

  if (last >= 0)
    shadowRays.resize (first, last + 1);

  if(j == nlights){
    stateBuffer.state = StateBuffer::Finished;
  } else {
    stateBuffer.state = StateBuffer::Continuing;
    stateBuffer.i1 = j;
  }
}

string NoShadows::getName() const {
  return "noshadows";
}

string NoShadows::getSpecs() const {
  return "none";
}

