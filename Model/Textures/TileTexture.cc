
/*
 * TileTexture.cc
 */

#include <Model/Textures/TileTexture.h>

#ifdef MANTA_SSE
namespace Manta {

template<>
void TileTexture<Color>::mapValues(Packet<Color>& results, const RenderContext& context, RayPacket& rays) const
{
    if(need_w)
      rays.computeTextureCoordinates3(context);
    else
      rays.computeTextureCoordinates2(context);

    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        Real vv1 = Dot(rays.getTexCoords(i), v1);
        Real vv2 = Dot(rays.getTexCoords(i), v2);
                Real vv1_=vv1;
        Real vv2_=vv2;
            int i1;
            int i2;
        if(vv1>0){
          i1 =(int)vv1;
            }
            else
            {
              int i11=-(int)(-vv1);
                  if(i11==vv1)
                    i1=i11;
                  else
                    i1=i11-1;
            }
            if(vv2>0){
           i2 =(int)vv2;
            }
            else
            {
              int i22=-(int)(-vv2);
                  if(i22==vv2)
                     i2=i22;
                  else
                     i2=i22-1;
            }
            Real ii1=vv1_-i1;
                Real ii2=vv2_-i2;
        int which = ((ii1<tile_gap)||(ii2<tile_gap));
        results.set(i, values[which]);
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        Real vv1 = Dot(rays.getTexCoords(i), v1);
        Real vv2 = Dot(rays.getTexCoords(i), v2);
                Real vv1_=vv1;
        Real vv2_=vv2;
        int i1;
            int i2;
        if(vv1>0){
          i1 =(int)vv1;
            }
            else
            {
               int i11=-(int)(-vv1);
                   if(i11==vv1)
                     i1=i11;
                   else
                    i1=i11-1;
           }
           if(vv2>0){
        i2 =(int)vv2;
           }
           else
           {
             int i22=-(int)(-vv2);
                 if(i22==vv2)
                   i2=i22;
                 else
                   i2=i22-1;
           }
            Real ii1=vv1_-i1;
                Real ii2=vv2_-i2;
        int which = ((ii1<tile_gap)||(ii2<tile_gap));
        results.set(i, values[which]);
      }
      RayPacketData* data = rays.data;
      // Set rounding modes to round toward -inf
      int old_csr = _mm_getcsr();
      _mm_setcsr((old_csr & ~_MM_ROUND_MASK) | _MM_ROUND_DOWN);
      for(;i<e;i+=4){
        __m128 tx = _mm_load_ps(&data->texCoords[0][i]);
        __m128 ty = _mm_load_ps(&data->texCoords[1][i]);
        __m128 tz = _mm_load_ps(&data->texCoords[2][i]);
        __m128 vv1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(tx, _mm_set1_ps(v1[0])), _mm_mul_ps(ty, _mm_set1_ps(v1[1]))), _mm_mul_ps(tz, _mm_set1_ps(v1[2])));
        __m128 vv2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(tx, _mm_set1_ps(v2[0])), _mm_mul_ps(ty, _mm_set1_ps(v2[1]))), _mm_mul_ps(tz, _mm_set1_ps(v2[2])));
        __m128i i1 = _mm_cvtps_epi32(vv1);
        __m128i i2 = _mm_cvtps_epi32(vv2);
        __m128i which = _mm_and_si128(_mm_add_epi32(i1, i2), _mm_set1_epi32(1));
        __m128i mask = _mm_cmpeq_epi32(which, _mm_setzero_si128());
        __m128 valuer = _mm_or_ps(_mm_and_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[0][0])),
                                 _mm_andnot_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[1][0])));
        _mm_store_ps(&results.colordata[0][i], valuer);
        __m128 valueg = _mm_or_ps(_mm_and_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[0][1])),
                                 _mm_andnot_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[1][1])));
        _mm_store_ps(&results.colordata[1][i], valueg);
        __m128 valueb = _mm_or_ps(_mm_and_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[0][2])),
                                 _mm_andnot_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[1][2])));
        _mm_store_ps(&results.colordata[2][i], valueb);
      }
      _mm_setcsr(old_csr);
      for(;i<rays.rayEnd;i++){
        Real vv1 = Dot(rays.getTexCoords(i), v1);
        Real vv2 = Dot(rays.getTexCoords(i), v2);
                Real vv1_=vv1;
        Real vv2_=vv2;
                int i1;
            int i2;
        if(vv1>0){
         i1 =(int)vv1;
            }
            else
            {
             int i11=-(int)(-vv1);
                 if(i11==vv1)
                  i1=i11;
                 else
                  i1=i11-1;
            }
            if(vv2>0){
         i2 =(int)vv2;
            }
            else
            {
             int i22=-(int)(-vv2);
                 if(i22==vv2)
                  i2=i22;
                 else
                  i2=i22-1;
            }
            Real ii1=vv1_-i1;
                Real ii2=vv2_-i2;
        int which = ((ii1<tile_gap)||(ii2<tile_gap));
        results.set(i, values[which]);
      }
    }
}

template<>
void TileTexture<float>::mapValues(Packet<float>& results, const RenderContext& context, RayPacket& rays) const
{
    if(need_w)
      rays.computeTextureCoordinates3(context);
    else
      rays.computeTextureCoordinates2(context);
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        Real vv1 = Dot(rays.getTexCoords(i), v1);
        Real vv2 = Dot(rays.getTexCoords(i), v2);
                Real vv1_=vv1;
        Real vv2_=vv2;
                int i1;
            int i2;
        if(vv1>0){
         i1 =(int)vv1;
            }
            else
            {
             int i11=-(int)(-vv1);
                 if(i11==vv1)
                  i1=i11;
                 else
                  i1=i11-1;
            }
            if(vv2>0){
         i2 =(int)vv2;
            }
            else
            {
             int i22=-(int)(-vv2);
                 if(i22==vv2)
                  i2=i22;
                 else
                  i2=i22-1;
            }
            Real ii1=vv1_-i1;
                Real ii2=vv2_-i2;
        int which = ((ii1<tile_gap)||(ii2<tile_gap));
        results.set(i, values[which]);
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        Real vv1 = Dot(rays.getTexCoords(i), v1);
        Real vv2 = Dot(rays.getTexCoords(i), v2);
                Real vv1_=vv1;
        Real vv2_=vv2;
                int i1;
            int i2;
        if(vv1>0){
         i1 =(int)vv1;
            }
            else
            {
             int i11=-(int)(-vv1);
                 if(i11==vv1)
                  i1=i11;
                 else
                  i1=i11-1;
            }
            if(vv2>0){
         i2 =(int)vv2;
            }
            else
            {
             int i22=-(int)(-vv2);
                 if(i22==vv2)
                  i2=i22;
                 else
                  i2=i22-1;
            }
            Real ii1=vv1_-i1;
                Real ii2=vv2_-i2;
        int which = ((ii1<tile_gap)||(ii2<tile_gap));
        results.set(i, values[which]);
      }
      RayPacketData* data = rays.data;
      // Set rounding modes to round toward -inf
      int old_csr = _mm_getcsr();
      _mm_setcsr((old_csr & ~_MM_ROUND_MASK) | _MM_ROUND_DOWN);
      for(;i<e;i+=4){
        __m128 tx = _mm_load_ps(&data->texCoords[0][i]);
        __m128 ty = _mm_load_ps(&data->texCoords[1][i]);
        __m128 tz = _mm_load_ps(&data->texCoords[2][i]);
        __m128 vv1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(tx, _mm_set1_ps(v1[0])), _mm_mul_ps(ty, _mm_set1_ps(v1[1]))), _mm_mul_ps(tz, _mm_set1_ps(v1[2])));
        __m128 vv2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(tx, _mm_set1_ps(v2[0])), _mm_mul_ps(ty, _mm_set1_ps(v2[1]))), _mm_mul_ps(tz, _mm_set1_ps(v2[2])));
        __m128i i1 = _mm_cvtps_epi32(vv1);
        __m128i i2 = _mm_cvtps_epi32(vv2);
        __m128i which = _mm_and_si128(_mm_add_epi32(i1, i2), _mm_set1_epi32(1));
        __m128i mask = _mm_cmpeq_epi32(which, _mm_setzero_si128());
        __m128 value = _mm_or_ps(_mm_and_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[0])),
                                 _mm_andnot_ps((__m128)_mm_castsi128_ps(mask), _mm_set1_ps(values[1])));
        _mm_store_ps(&results.data[i], value);
      }
      _mm_setcsr(old_csr);
      for(;i<rays.rayEnd;i++){
        Real vv1 = Dot(rays.getTexCoords(i), v1);
        Real vv2 = Dot(rays.getTexCoords(i), v2);
                Real vv1_=vv1;
        Real vv2_=vv2;
                int i1;
            int i2;
        if(vv1>0){
         i1 =(int)vv1;
            }
            else
            {
             int i11=-(int)(-vv1);
                 if(i11==vv1)
                  i1=i11;
                 else
                  i1=i11-1;
            }
            if(vv2>0){
         i2 =(int)vv2;
            }
            else
            {
             int i22=-(int)(-vv2);
                 if(i22==vv2)
                  i2=i22;
                 else
                  i2=i22-1;
            }
            Real ii1=vv1_-i1;
                Real ii2=vv2_-i2;
        int which = ((ii1<tile_gap)||(ii2<tile_gap));
        results.set(i, values[which]);
      }
    }
}

} // end namespace Manta
#endif


