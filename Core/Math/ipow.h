
#ifndef Manta_Core_ipow_h
#define Manta_Core_ipow_h

#include <MantaSSE.h>
#include <Core/Math/SSEDefs.h>

namespace Manta {
  inline double ipow(double x, int p)
  {
    double result=1;
    while(p){
      if(p&1)
        result*=x;
      x*=x;
      p>>=1;
    }
    return result;
  }

  inline float ipow(float x, int p)
  {
    float result=1;
    while(p){
      if(p&1)
        result*=x;
      x*=x;
      p>>=1;
    }
    return result;
  }

#ifdef MANTA_SSE
  inline __m128 ipow(__m128 x, int p) {
    __m128 result = _mm_set1_ps(1.f);
    while (p) {
      if (p&1)
        result = _mm_mul_ps(result, x);
      x = _mm_mul_ps(x, x);
      p >>= 1;
    }
    return result;
  }
#endif

}

#endif
