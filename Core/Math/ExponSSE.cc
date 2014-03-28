#include <Core/Math/ExponSSE.h>

#include <cmath>
#include <stdio.h>

// Don't set this outside the range [1,5]
#ifndef SERIES_ITERATIONS
#define SERIES_ITERATIONS 3
#endif

#ifndef NUM_LN_SERIES
#define NUM_LN_SERIES SERIES_ITERATIONS
#endif

#ifndef NUM_EXP_SERIES
#define NUM_EXP_SERIES SERIES_ITERATIONS
#endif

#ifdef MANTA_SSE
// TODO: Replace all these horrible log approximations (bad performance and
// accuracy) with the more efficient method used in Logf/Log2f
__m128 Manta::LogSSE(__m128 val)
{
  // float zOverZ  = (val-1)/(val+1);
  __m128 zOverZ = _mm_div_ps(_mm_sub_ps(val, _mm_set_ps1(1.f)),
                             _mm_add_ps(val, _mm_set_ps1(1.f)));
  // float sum = 1;
  __m128 sum = _mm_set_ps1(1.f);
#if (NUM_LN_SERIES >= 2)
  // float zOverZ2 = zOverZ*zOverZ;
  __m128 zOverZ2 = _mm_mul_ps(zOverZ, zOverZ);
  // float mult = zOverZ2;
  __m128 mult = zOverZ2;
  // sum += 0.33333333f * mult; // (1/3) * zOverZ^2
  sum = _mm_add_ps(sum, _mm_mul_ps(mult, _mm_set_ps1(0.333333333f)));
#endif
#if (NUM_LN_SERIES >= 3)
  // mult *= zOverZ2;
  mult = _mm_mul_ps(mult, zOverZ2);
  // sum += 0.2f * mult; // (1/5) * zOverZ^4
  sum = _mm_add_ps(sum, _mm_mul_ps(mult, _mm_set_ps1(0.2f)));
#endif
#if (NUM_LN_SERIES >= 4)
  // mult *= zOverZ2;
  mult = _mm_mul_ps(mult, zOverZ2);
  // sum += 0.14285714f * mult; // (1/7) * zOverZ^6
  sum = _mm_add_ps(sum, _mm_mul_ps(mult, _mm_set_ps1(0.14285714f)));
#endif
#if (NUM_LN_SERIES >= 5)
  // mult *= zOverZ2;
  mult = _mm_mul_ps(mult, zOverZ2);
  // sum += 0.11111111f * mult; // (1/9) * zOverZ^8
  sum = _mm_add_ps(sum, _mm_mul_ps(mult, _mm_set_ps1(0.11111111f)));
#endif
  // return 2.f * zOverZ * sum;

  // I'm arranging the multiplies, so that 2*zOverZ could be scheduled
  // earlier, since it is invariant on sum. (2*(zOverZ*sum)) would
  // have to wait for sum to be computed before it could do
  // multiply by 2.
  return _mm_mul_ps(_mm_mul_ps(_mm_set_ps1(2.f), zOverZ), sum);
}
#endif // #ifdef MANTA_SSE

// The math for all of this came from this web page:
//
// http://en.wikipedia.org/wiki/Exponential_function#Formal_definition
//
// e^x = sum(n=0..inf)((x^n)/n!)
//
// This series doesn't converge very quickly for numbers greater
// than 1, so you should use the Expf function below.
float Manta::ExpSeries(float val, int series)
{
  float sum = 1;
  float xpow = 1;
  float fact = 1;
  for(int i = 1; i < series; ++i) {
    fact *= i;
    xpow *= val;
    sum += xpow/fact;
  }
  return sum;
}

float Manta::ExpSeries(float val)
{
  float sum = 1 + val; // series 0, 1
#if (NUM_EXP_SERIES >= 2)
  float xpow = val*val;
  sum += xpow * 0.5f; // x^2/2!
#endif
#if (NUM_EXP_SERIES >= 3)
  xpow *= val;
  sum += xpow * 0.16666666667f; // x^3/3!
#endif
#if (NUM_EXP_SERIES >= 4)
  xpow *= val;
  sum += xpow * 0.041666666667f; // x^4/4!
#endif
#if (NUM_EXP_SERIES >= 5)
  xpow *= val;
  sum += xpow * 0.0083333333333f; // x^5/5!
#endif
  return sum;
}

#ifdef MANTA_SSE
__m128 Manta::ExpSeriesSSE(__m128 val)
{
  // float sum = 1 + val; // series 0, 1
  __m128 sum = _mm_add_ps(_mm_set_ps1(1.f), val);
#if (NUM_EXP_SERIES >= 2)
  // float xpow = val * val;
  __m128 xpow = _mm_mul_ps(val, val);
  // sum += xpow * 0.5f;  // x^2/2!
  sum = _mm_add_ps(sum, _mm_mul_ps(xpow, _mm_set_ps1(0.5f)));
#endif
#if (NUM_EXP_SERIES >= 3)
  // xpow *= val;
  xpow = _mm_mul_ps(xpow, val);
  // sum += xpow * 0.16666666667f; // x^3/3!
  sum = _mm_add_ps(sum, _mm_mul_ps(xpow, _mm_set_ps1(0.16666666667f)));
#endif
#if (NUM_EXP_SERIES >= 4)
  // xpow *= val;
  xpow = _mm_mul_ps(xpow, val);
  // sum += xpow * 0.041666666667f; // x^4/4!
  sum = _mm_add_ps(sum, _mm_mul_ps(xpow, _mm_set_ps1(0.041666666667f)));
#endif
#if (NUM_EXP_SERIES >= 5)
  // xpow *= val;
  xpow = _mm_mul_ps(xpow, val);
  // sum += xpow * 0.0083333333333f; // x^5/5!
  sum = _mm_add_ps(sum, _mm_mul_ps(xpow, _mm_set_ps1(0.0083333333333f)));
#endif
  return sum;
}
#endif

// The problem with the ExpSeries is that it is slow to converge for
// numbers larger than one.  This employs a trick that we can
// exploit with regard to how floating point numbers are
// represented to speed up convergence.
//
// Taken from:
// http://en.wikipedia.org/wiki/Exponential_function#Computing_exp.28x.29_for_real_x
//
// y = m*2^n = e^x
// n = floor(x/log(2)) // note this is the natural log
// u = x - n * log(2)

// The number u is small and in the range of (0, log(2)), so the
// exp series will converge quickly.
//
// m = e^u = ExpSeries(u)
//
// Since n is a power of 2 we can stuff this into the exponent
// portion of the floating point number.  We can do all this with
// bit twiddling.

float Manta::Expf(float val, int series)
{
  float log2_val = logf(2);
  float n = floorf(val/log2_val);
  float u = val - n*log2_val;
  union {
    float f;
    int   i;
  } m;
  m.f = ExpSeries(u, series);
  int nInt = (int)n + 127;
  // We can run into problems if the exponent is sufficiently small
  // such that it causes an underflow.  This test caps it off.
  if (nInt < 0) nInt = 0;
  unsigned int exp_bits = nInt << 23;
  unsigned int exp_bits_masked = exp_bits & 0x7F800000;
  unsigned int m_bits = m.i;
  unsigned int m_bits_masked = m_bits & 0x807FFFFF;
  union {
    float        f;
    unsigned int i;
  } exp_val2_bits;
  exp_val2_bits.i = m_bits_masked | exp_bits_masked;
  float exp_val2 = exp_val2_bits.f;
  return exp_val2;
}

float Manta::Expf(float val)
{
  float n = floorf(val*1.44269503997304722501f); // 1/logf(2)
  float u = val - n*0.693147181f; // (logf(2))
  union {
    float f;
    int   i;
  } m;
  m.f = ExpSeries(u);
  int nInt = (int)n + 127;
  if (nInt < 0) nInt = 0;
  unsigned int exp_bits = nInt << 23;
  unsigned int exp_bits_masked = exp_bits & 0x7F800000;
  unsigned int m_bits = m.i;
  unsigned int m_bits_masked = m_bits & 0x807FFFFF;
  union {
    float        f;
    unsigned int i;
  } exp_val2_bits;
  exp_val2_bits.i = m_bits_masked | exp_bits_masked;
  float exp_val2 = exp_val2_bits.f;
  return exp_val2;
}

#ifdef MANTA_SSE
// Unline the Expf functions, this code contains no branches.
__m128 Manta::ExpSSE(__m128 val)
{
  // float log2_val = logf(2); // = 0.693147181f
  // float n = floorf(val/log2_val);
  __m128 n = floorSSE(_mm_div_ps(val, _mm_set_ps1(0.693147181f)));
  // float u = val - n*log2_val;
  __m128 u = _mm_sub_ps(val, _mm_mul_ps(n, _mm_set_ps1(0.693147181f)));
  __m128 m = ExpSeriesSSE(u);
  // int nInt = (int)n + 127;
  __m128i nInt = _mm_add_epi32(_mm_cvttps_epi32(n), _mm_set1_epi32(127));
  // if (nInt < 0) nInt = 0;
  __m128i lessThanZero = _mm_cmplt_epi32(nInt, _mm_setzero_si128());
  nInt = _mm_andnot_si128(lessThanZero, nInt);
  // unsigned int exp_bits = nInt << 23;
  __m128i exp_bits = _mm_slli_epi32(nInt, 23);
  // unsigned int exp_bits_masked = exp_bits & 0x7F800000;
  __m128i exp_bits_masked = _mm_and_si128(_mm_set1_epi32(0x7F800000),exp_bits);
  // unsigned int m_bits = float2bits(m);
  __m128i m_bits = _mm_castps_si128(m);
  // unsigned int m_bits_masked = m_bits & 0x807FFFFF;
  __m128i m_bits_masked = _mm_andnot_si128(_mm_set1_epi32(0x7F800000), m_bits);
  // unsigned int exp_val2_bits = m_bits_masked | exp_bits_masked;
  __m128i exp_val2_bits = _mm_or_si128(m_bits_masked, exp_bits_masked);
  // float exp_val2 = bits2float(exp_val2_bits);
  __m128 exp_val2 = _mm_castsi128_ps(exp_val2_bits);
  return exp_val2;
}
#endif

// const float pow_correction_factor = .33979f;
const float pow_correction_factor = 0.33971f;
const float log_correction_factor = 0.346607f;

const bool print_stuff = false;

// Approximate Log2 that's almost 5x faster with max error < 0.007
// http://www.flipcode.com/archives/Fast_log_Function.shtml
float Manta::Log2f(float val)
{
  union {
    float f;
    int   i;
  } val_union;
  val_union.f = val;

  const int log_2 = ((val_union.i >> 23) & 255) - 128;
  val_union.i &= ~(255 << 23);
  val_union.i += 127 << 23;

  val_union.f = ((-1.0f/3) * val_union.f + 2) * val_union.f - 2.0f/3;

  return val_union.f + log_2;
}

float Manta::Pow2f(float val)
{
  float PowBodge=pow_correction_factor;
  union {
    float f;
    int   i;
  } x;
  float y=val-floorf(val);
  y=(y-y*y)*PowBodge;

  float expon = val+127;
  //  printf("expon = %f, i = %f, y = %f\n", expon, i, y);
  if (expon < 1) {
    //printf("expon = %f, val = %f, y = %f\n", expon, val, y);
    expon = 1;
  }
  x.f=expon-y;
  //  printf("x = "); printBinary(float2bits(x)); printf("\n");
  x.f*= (1<<23); //pow(2,23);
  x.i = (int)x.f;
  if (print_stuff) printf("myPow2(%g)= %g\n", val, x.f);
  return x.f;
}

#ifdef MANTA_SSE
// TODO: replace with all around better Log2f method.
__m128 Manta::Log2SSE(__m128 val)
{
  __m128 LogBodge = _mm_set1_ps(log_correction_factor);
  __m128 x = _mm_cvtepi32_ps(_mm_castps_si128(val));
  x = _mm_mul_ps(x, _mm_set1_ps(1.f/(1<<23)));
  x = _mm_sub_ps(x, _mm_set1_ps(127.f));
  __m128 y = _mm_sub_ps(x, floorSSE(x));
  y = _mm_mul_ps(LogBodge, _mm_sub_ps(y, _mm_mul_ps(y, y)));
  return _mm_add_ps(x,y);
}

__m128 Manta::Pow2SSE(__m128 val)
{
  __m128 PowBodge = _mm_set1_ps(pow_correction_factor);
  __m128 y = _mm_sub_ps(val, floorSSE(val));
  y = _mm_mul_ps(PowBodge, _mm_sub_ps(y, _mm_mul_ps(y, y)));
  __m128 expon = _mm_add_ps(val, _mm_set1_ps(127.f));
  __m128 mask = _mm_cmplt_ps(expon, _mm_set1_ps(1.f));
  expon = _mm_or_ps(_mm_and_ps   (mask, _mm_set1_ps(1.f)),
                    _mm_andnot_ps(mask, expon));
  __m128 x = _mm_sub_ps(expon, y);
  x = _mm_mul_ps(x, _mm_set1_ps(1<<23));
//   __m128i int_result = _mm_cvttps_epi32(x);
//   __m128 result;
//   _mm_store_si128((__m128i*)&result, int_result);
  return _mm_castsi128_ps(_mm_cvttps_epi32(x));
}

// This will unpack, call powf, and pack.
__m128 Manta::PowSSEMathH(__m128 a, __m128 b)
{
  // Unpack the values
#if 1
  float* base_unpack = (float*)(&a);
  float* expon_unpack = (float*)(&b);
#else
  MANTA_ALIGN(16) float base_unpack[4];
  MANTA_ALIGN(16) float expon_unpack[4];
  _mm_store_ps(base_unpack, a);
  _mm_store_ps(expon_unpack, b);
#endif
  MANTA_ALIGN(16) float result_unpack[4];
  // Do the computation
  for(unsigned int i = 0; i < 4; ++i)
    result_unpack[i] = powf(base_unpack[i], expon_unpack[i]);
  // Pack the results
  return _mm_load_ps(result_unpack);
}

// The same, but uses a mask to avoid calls to powf.
__m128 Manta::PowSSEMathH(__m128 a, __m128 b, __m128 mask)
{
  // Unpack the values
  float* base_unpack = (float*)(&a);
  float* expon_unpack = (float*)(&b);
  MANTA_ALIGN(16) float result_unpack[4];
  int intmask = _mm_movemask_ps(mask);
  // Do the computation
  for(unsigned int i = 0; i < 4; ++i)
    if (intmask & (1 << i))
      result_unpack[i] = powf(base_unpack[i], expon_unpack[i]);
  // Pack the results
  return _mm_load_ps(result_unpack);
}

// This will unpack, call expf, and pack.
__m128 Manta::ExpSSEMathH(__m128 val)
{
  // Unpack the values
  float* val_unpack = (float*)(&val);
  MANTA_ALIGN(16) float result_unpack[4];
  // Do the computation
  for(unsigned int i = 0; i < 4; ++i)
    result_unpack[i] = expf(val_unpack[i]);
  // Pack the results
  return _mm_load_ps(result_unpack);
}

// The same, but uses a mask to avoid calls to expf.
__m128 Manta::ExpSSEMathH(__m128 val, __m128 mask)
{
  // Unpack the values
  float* val_unpack = (float*)(&val);
  MANTA_ALIGN(16) float result_unpack[4];
  // Do the computation
  int intmask = _mm_movemask_ps(mask);
  for(unsigned int i = 0; i < 4; ++i)
    if (intmask & (1 << i))
      result_unpack[i] = expf(val_unpack[i]);
  // Pack the results
  return _mm_load_ps(result_unpack);
}

// This will unpack, call logf, and pack.
__m128 Manta::LogSSEMathH(__m128 val)
{
  // Unpack the values
  float* val_unpack = (float*)(&val);
  MANTA_ALIGN(16) float result_unpack[4];
  // Do the computation
  for(unsigned int i = 0; i < 4; ++i)
    result_unpack[i] = logf(val_unpack[i]);
  // Pack the results
  return _mm_load_ps(result_unpack);
}

// The same, but uses a mask to avoid calls to logf.
__m128 Manta::LogSSEMathH(__m128 val, __m128 mask)
{
  // Unpack the values
  float* val_unpack = (float*)(&val);
  MANTA_ALIGN(16) float result_unpack[4];
  // Do the computation
  int intmask = _mm_movemask_ps(mask);
  for(unsigned int i = 0; i < 4; ++i)
    if (intmask & (1 << i))
      result_unpack[i] = logf(val_unpack[i]);
  // Pack the results
  return _mm_load_ps(result_unpack);
}
#endif

