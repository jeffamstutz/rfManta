
#include <Model/Textures/Constant.h>
#include <Interface/RayPacket.h>

using namespace Manta;

template<>
void Constant<Color>::mapValues(Packet<Color>& results,
                                const RenderContext& context,
                                RayPacket& rays) const
{
#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = rays.rayEnd & (~3);
  if(b >= e){
    for(int i = rays.begin(); i < rays.end(); i++){
      results.set(i, value);
    }
  } else {
    int i = rays.rayBegin;
    for(;i<b;i++){
      results.set(i, value);
    }
    for(;i<e;i+=4){
      _mm_store_ps(&results.colordata[0][i], _mm_set1_ps(value[0]));
      _mm_store_ps(&results.colordata[1][i], _mm_set1_ps(value[1]));
      _mm_store_ps(&results.colordata[2][i], _mm_set1_ps(value[2]));
    }
    for(;i<rays.rayEnd;i++){
      results.set(i, value);
    }
  }
#else
  for(int i = rays.begin(); i < rays.end(); i++){
    results.set(i, value);
  }
#endif
}

template<>
void Constant<float>::mapValues(Packet<float>& results,
                                const RenderContext& context,
                                RayPacket& rays) const
{
#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = rays.rayEnd & (~3);
  if(b >= e){
    for(int i = rays.begin(); i < rays.end(); i++){
      results.set(i, value);
    }
  } else {
    int i = rays.rayBegin;
    for(;i<b;i++){
      results.set(i, value);
    }
    for(;i<e;i+=4){
      _mm_store_ps(&results.data[i], _mm_set1_ps(value));
    }
    for(;i<rays.rayEnd;i++){
      results.set(i, value);
    }
  }
#else
  for(int i = rays.begin(); i < rays.end(); i++){
    results.set(i, value);
  }
#endif
}
