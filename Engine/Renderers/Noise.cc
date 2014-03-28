
#include <Engine/Renderers/Noise.h>
#include <Core/Color/GrayColor.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Math/Trig.h>
#include <Core/Util/Args.h>
#include <Core/Util/Assert.h>
#include <Interface/Context.h>
#include <Interface/FrameState.h>
#include <Interface/RayPacket.h>
#include <Core/Math/SSEDefs.h>
#include <Core/Math/Noise.h>

using namespace Manta;

Renderer* Noise::create(const vector<string>& args)
{
  return new Noise(args);
}

Noise::Noise(const vector<string>& args)
{
  sse = false;
  rate=0.1;
  for(size_t i=0;i<args.size();i++){
    string arg = args[i];
    if(arg == "-rate"){
      if(!getDoubleArg(i, args, rate))
	throw IllegalArgument("Moire rate", i, args);
    } else if(arg == "-sse"){
      sse = true;
    } else {
      throw IllegalArgument("Moire", i, args);
    }
  }
#ifndef MANTA_SSE
  if (sse) {
    sse = false;
  }
#endif
}

Noise::~Noise()
{
}

void Noise::setupBegin(const SetupContext&, int)
{
}

void Noise::setupDisplayChannel(SetupContext&)
{
}

void Noise::setupFrame(const RenderContext&)
{
}

void Noise::traceEyeRays(const RenderContext& context, RayPacket& rays)
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  float phase = context.frameState->frameTime * rate;
  RayPacketData* data = rays.data;
  if(sse){
#ifdef MANTA_SSE
    __m128 z = _mm_set1_ps(phase);
    for(int i=rays.begin();i<rays.end();i+=4){
      __m128 x = _mm_mul_ps(_mm_load_ps(&data->image[0][i]), _mm_set1_ps(25));
      __m128 y = _mm_mul_ps(_mm_load_ps(&data->image[1][i]), _mm_set1_ps(25));
      __m128 sval = ScalarNoiseSSE(x, y, z);
      __m128 gray = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(0.5), sval), _mm_set1_ps(0.5));
      _mm_store_ps(&data->color[0][i], gray);
      _mm_store_ps(&data->color[1][i], gray);
      _mm_store_ps(&data->color[2][i], gray);
    }
#endif
  } else {
    for(int i=rays.begin();i<rays.end();i++){
      float x = data->image[0][i]*25;
      float y = data->image[1][i]*25;
      float sval = ScalarNoise(Vector(x, y, phase));
      float gray = sval * 0.5 + 0.5;
      data->color[0][i] = gray;
      data->color[1][i] = gray;
      data->color[2][i] = gray;
    }
  }
}

void Noise::traceRays(const RenderContext& context, RayPacket& rays)
{
  // This will not usually be used...
  traceEyeRays(context, rays);
}
