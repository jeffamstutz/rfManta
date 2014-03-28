
#include <Engine/Renderers/Moire.h>
#include <Core/Color/GrayColor.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Math/Trig.h>
#include <Core/Util/Args.h>
#include <Core/Util/Assert.h>
#include <Interface/Context.h>
#include <Interface/FrameState.h>
#include <Interface/RayPacket.h>

using namespace Manta;

Renderer* Moire::create(const vector<string>& args)
{
  return new Moire(args);
}

Moire::Moire(const vector<string>& args)
{
  rate=2;
  cycles = 100;
  for(size_t i=0;i<args.size();i++){
    string arg = args[i];
    if(arg == "-rate"){
      if(!getDoubleArg(i, args, rate))
	throw IllegalArgument("Moire rate", i, args);
    } else if(arg == "-cycles"){
      if(!getDoubleArg(i, args, cycles))
	throw IllegalArgument("Moire cycles", i, args);
    } else {
      throw IllegalArgument("Moire", i, args);
    }
  }
}

Moire::~Moire()
{
}

void Moire::setupBegin(const SetupContext&, int)
{
}

void Moire::setupDisplayChannel(SetupContext&)
{
}

void Moire::setupFrame(const RenderContext&)
{
}

void Moire::traceEyeRays(const RenderContext& context, RayPacket& rays)
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  double phase = context.frameState->frameTime * M_PI;
  double cycles = 100;
  for(int i=rays.begin();i<rays.end();i++){
    Real ix = rays.getImageCoordinates(i, 0);
    Real iy = rays.getImageCoordinates(i, 1);
    Real dist2 = ix*ix + iy*iy;
    Real val = Cos(dist2*2*M_PI*cycles+phase)/2+0.5;
    rays.setColor(i, Color(GrayColor(val)));
  }
}

void Moire::traceRays(const RenderContext& context, RayPacket& rays)
{
  // This will not usually be used...
  traceEyeRays(context, rays);
}
