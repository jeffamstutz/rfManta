
#include <Engine/Renderers/RayGen.h>
#include <Core/Color/RGBColor.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>
#include <Core/Util/Assert.h>
#include <Interface/Camera.h>
#include <Interface/Material.h>
#include <Interface/Object.h>
#include <Interface/Scene.h>
#include <MantaSSE.h>

using namespace Manta;

Renderer* RayGen::create(const vector<string>& args)
{
  return new RayGen(args);
}

RayGen::RayGen(const vector<string>&)
{
  color = Color(RGBColor(0.2, 0.9, 0.5));
}

RayGen::~RayGen()
{
}

void RayGen::setupBegin(const SetupContext&, int)
{
}

void RayGen::setupDisplayChannel(SetupContext&)
{
}

void RayGen::setupFrame(const RenderContext&)
{
}

void RayGen::traceEyeRays(const RenderContext& context, RayPacket& rays)
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  context.camera->makeRays(context, rays);
}

void RayGen::traceRays(const RenderContext&, RayPacket& rays)
{
}
