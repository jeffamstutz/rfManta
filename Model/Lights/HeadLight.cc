

#include <Model/Lights/HeadLight.h>
#include <Interface/Context.h>
#include <Interface/Camera.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/Vector.h>

using namespace Manta;

void HeadLight::computeLight(RayPacket& destRays,
                             const RenderContext &context,
                             RayPacket& sourceRays) const
{
  // Determine the camera position.
  Vector camera = context.camera->getPosition();
  Vector up     = context.camera->getUp();

  // Determine light position.
  Vector position = camera + (up * offset);

  sourceRays.computeHitPositions();
  for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
    destRays.setColor(i, color);
    destRays.setDirection(i, position - sourceRays.getHitPosition(i));
    destRays.overrideMinT(i, 1);
  }
}
