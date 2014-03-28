
#include <Interface/Renderer.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>

using namespace Manta;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::traceRays(const RenderContext& context, RayPacket& rays, Real cutoff)
{
  traceRays(context, rays);
}

