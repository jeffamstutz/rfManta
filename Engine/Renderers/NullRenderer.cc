
#include <Engine/Renderers/NullRenderer.h>
#include <Core/Color/RGBColor.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>
#include <Core/Util/Assert.h>
#include <MantaSSE.h>

using namespace Manta;

Renderer* NullRenderer::create(const vector<string>& args)
{
  return new NullRenderer(args);
}

NullRenderer::NullRenderer(const vector<string>&)
{
  color = Color(RGBColor(0.9, 0.6, 0.3));
}

NullRenderer::~NullRenderer()
{
}

void NullRenderer::setupBegin(const SetupContext&, int)
{
}

void NullRenderer::setupDisplayChannel(SetupContext&)
{
}

void NullRenderer::setupFrame(const RenderContext&)
{
}

void NullRenderer::traceEyeRays(const RenderContext&, RayPacket& rays)
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  for(int i=rays.begin();i<rays.end();i++){
    rays.setColor(i, color);
  }
}

void NullRenderer::traceRays(const RenderContext&, RayPacket& rays)
{
  for(int i=rays.begin();i<rays.end();i++){
    rays.setColor(i, color);
  }
#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = rays.rayEnd & (~3);
  RayPacketData* data = rays.data;
  if(b >= e){
    for(int i=rays.begin();i<rays.end();i++){
      rays.setColor(i, color);
    }
  } else {
    int i = rays.rayBegin;
    for(;i<b;i++){
      data->color[0][i] = color[0];
      data->color[1][i] = color[1];
      data->color[2][i] = color[2];
    }
    __m128 vec_red = _mm_set1_ps(color[0]);
    __m128 vec_green = _mm_set1_ps(color[1]);
    __m128 vec_blue = _mm_set1_ps(color[2]);
    for(;i<e;i+=4){
      _mm_store_ps(&data->color[0][i], vec_red);
      _mm_store_ps(&data->color[1][i], vec_green);
      _mm_store_ps(&data->color[2][i], vec_blue);
    }
    for(;i<rays.rayEnd;i++){
      data->color[0][i] = color[0];
      data->color[1][i] = color[1];
      data->color[2][i] = color[2];
    }
  }
#else
  for(int i=rays.begin();i<rays.end();i++){
    rays.setColor(i, color);
  }
#endif
}
