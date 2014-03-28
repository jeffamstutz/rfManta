
#ifndef Manta_Core_ExponSSE_h
#define Manta_Core_ExponSSE_h

#include <MantaSSE.h>

#include <cmath>

#ifdef MANTA_SSE
#include <emmintrin.h>
#include <Core/Util/Align.h>
#include <Core/Math/SSEDefs.h>
#endif // MANTA_SSE

namespace Manta {

  // A very fast log approximation that uses a precomputed table.  This is
  // faster and more accurate than the Logf variant below.  Only catch is that
  // you have to build and store the table...
  class ICSI_log {
  // http://www.icsi.berkeley.edu/pubs/techreports/TR-07-002.pdf
  // http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4741148&tag=1
  // Paper: "A Hardware-Independent Fast Logarithm Approximation with Adjustable Accuracy"
  // Authors: Vinyals, O.;   Friedland, G.

  public:
    // Size of table is 2^precision entries.  Default option of precision 15
    // uses 128KB for the table and has a max error of about 1.6e-5.  Each
    // increase in precision roughly halves the error.
    ICSI_log(int precision=15) {
      if (precision < 0) precision = 0;
      if (precision > 23) precision = 23;
      n = precision;

      lookupTable = new float[1<<n];
      fill_log_table();
    }

    ~ICSI_log() { delete[] lookupTable; }


    inline float log(const float val) const {
      union {
        float f;
        int   i;
      } val_union;

      val_union.f = val;

      /* extract exponent and mantissa (quantized) */
      const int exp = ((val_union.i >> 23) & 255) - 127;
      const int man = (val_union.i & 0x7FFFFF) >> (23 - n);

      /* exponent plus lookup refinement */
      return ((float)(exp) + lookupTable[man]) * 0.69314718055995f;
    }

  private:
    void fill_log_table() {
      // step along table elements and x-axis positions.
      // (start with extra half increment, so the steps intersect at their midpoints.)
      float oneToTwo = 1.0f + (1.0f / (float)( 1 <<(n + 1) ));
      int i;
      for(i = 0;  i < (1 << n);  ++i ) {
        // make y-axis value for table element
        lookupTable[i] = logf(oneToTwo) / 0.69314718055995f;
        oneToTwo += 1.0f / (float)( 1 << n );
      }
    }

    int n;
    float* lookupTable;
  };

  float Log2f(float val);

  inline float Logf(float val)
  {
    return Log2f(val) * 0.69314718055995f;
  }

#ifdef MANTA_SSE
  __m128 LogSSE(__m128 val);
#endif

  float ExpSeries(float val, int series);
  float ExpSeries(float val);
#ifdef MANTA_SSE
  __m128 ExpSeriesSSE(__m128 val);
#endif

  float Expf(float val, int series);
  float Expf(float val);
#ifdef MANTA_SSE
  // Unline the Expf functions, this code contains no branches.
  __m128 ExpSSE(__m128 val);
#endif

  float Pow2f(float val);
#ifdef MANTA_SSE
  __m128 Log2SSE(__m128 val);
  __m128 Pow2SSE(__m128 val);
#endif

  // Pow is computed a^b = e^(b*ln(a))
  inline float Powf(float a, float b)
  {
    return Expf(b*Logf(a));
  }

  inline float Powfv2(float a, float b)
  {
    return Pow2f(b*Log2f(a));
  }

#ifdef MANTA_SSE
  inline __m128 PowSSE(__m128 a, __m128 b)
  {
    return ExpSSE(_mm_mul_ps(b, LogSSE(a)));
  }

  inline __m128 PowSSEv2(__m128 a, __m128 b)
  {
    return Pow2SSE(_mm_mul_ps(b, Log2SSE(a)));
  }

  // This will unpack, call powf, and pack.
  __m128 PowSSEMathH(__m128 a, __m128 b);
  // The same, but uses a mask to avoid calls to powf.
  __m128 PowSSEMathH(__m128 a, __m128 b, __m128 mask);
  //
  __m128 ExpSSEMathH(__m128 val);
  __m128 ExpSSEMathH(__m128 val, __m128 mask);
  //
  __m128 LogSSEMathH(__m128 val);
  __m128 LogSSEMathH(__m128 val, __m128 mask);

#endif

} // end namespace Manta

#endif // #ifndef Manta_Core_ExponSSE_h
