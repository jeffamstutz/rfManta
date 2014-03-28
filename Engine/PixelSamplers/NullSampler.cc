
#include <Engine/PixelSamplers/NullSampler.h>
#include <Core/Color/RGBColor.h>
#include <Interface/Fragment.h>
#include <MantaSSE.h>

using namespace Manta;

PixelSampler* NullSampler::create(const vector<string>& args)
{
  return new NullSampler(args);
}

NullSampler::NullSampler(const vector<string>& /* args */)
{
}

NullSampler::~NullSampler()
{
}

void NullSampler::setupBegin(const SetupContext&, int)
{
}

void NullSampler::setupDisplayChannel(SetupContext&)
{
}

void NullSampler::setupFrame(const RenderContext&)
{
}

void NullSampler::renderFragment(const RenderContext&, Fragment& fragment)
{
#if MANTA_SSE
  if(((fragment.pixelBegin | fragment.pixelEnd) & 0x3) == 0){
    __m128 r = _mm_set1_ps( 0.3 );
    __m128 g = _mm_set1_ps( 0.6 );
    __m128 b = _mm_set1_ps( 0.9 );
    for(int i=fragment.pixelBegin; i < fragment.pixelEnd; i+=4){
      _mm_store_ps(&fragment.color[0][i], r);
      _mm_store_ps(&fragment.color[1][i], g);
      _mm_store_ps(&fragment.color[2][i], b);
    }
  } else
#endif /* MANTA_SSE */
  {
    for(int i=fragment.pixelBegin;i<fragment.pixelEnd;i++){
      fragment.color[0][i] = 0.3;
      fragment.color[1][i] = 0.6;
      fragment.color[2][i] = 0.9;
    }
  }
}
