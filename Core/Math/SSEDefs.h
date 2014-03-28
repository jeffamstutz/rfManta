// SSEDefs.h
// A comprehensive set of macros for SSE

#ifndef _MANTA_SSEDEFS_H_
#define _MANTA_SSEDEFS_H_

#include <MantaSSE.h>

//Placing this before MANTA_SSE ifdef so that non-sse code
//doesn't break when iostream isn't included.
#include <iostream>

#ifdef MANTA_SSE
#include <xmmintrin.h>
#include <emmintrin.h>
#include <Core/Util/Align.h>
#include <Core/Geometry/vecdefs.h>
#include <Core/Geometry/Vector.h>
#include <Parameters.h>

typedef __m128 sse_t;
typedef __m128i sse_int_t;

//add to these macros as necessary.
#define or4 _mm_or_ps
#define or4i _mm_or_si128
#define and4 _mm_and_ps
#define and4i _mm_and_si128
#define andnot4 _mm_andnot_ps
#define andnot4i _mm_andnot_si128
#define xor4 _mm_xor_ps
#define xor4i _mm_xor_si128
#define mul4 _mm_mul_ps
#define add4 _mm_add_ps
#define add4i _mm_add_epi32
#define sub4 _mm_sub_ps
#define min4 _mm_min_ps
#define max4 _mm_max_ps
#define set4 _mm_set_ps1
#define set44 _mm_set_ps
#define set4i _mm_set1_epi32
#define set44i _mm_set_epi32
#define set4l _mm_set1_epi64x // See comments below
#define unpacklo _mm_unpacklo_ps // (a,b) => [a0, b0, a1, b1]
#define unpackhi _mm_unpackhi_ps // (a,b) => [a2, b2, a3, b3]
#define zero4 _mm_setzero_ps
#define zero4i _mm_setzero_si128
#define getmask4 _mm_movemask_ps
#define maskmove4i _mm_maskmoveu_si128 // destination need not be aligned
#define cmp4_ge _mm_cmpge_ps
#define cmp4_le _mm_cmple_ps
#define cmp4_gt _mm_cmpgt_ps
#define cmp4_lt _mm_cmplt_ps
#define cmp4_eq _mm_cmpeq_ps
#define cmp4_eq_i _mm_cmpeq_epi32
#define load44 _mm_load_ps
#define load44i _mm_load_si128
#define store44 _mm_store_ps
#define store44i _mm_store_si128
// The cast_x2y are more like reinterpret casts.  For convertions of
// types use the convert4 functions below.
#define cast4_i2f _mm_castsi128_ps
#define cast4_f2i _mm_castps_si128
#define convert4_f2i _mm_cvttps_epi32
#define convert4_i2f _mm_cvtepi32_ps
#define sqrt4 _mm_sqrt_ps
#define shift_right4int _mm_srli_epi32 // >>
#define shift_left4int _mm_slli_epi32  // <<

namespace std {
  std::ostream& operator<<(std::ostream& os, __m128);
  std::ostream& operator<<(std::ostream& os, __m128i);
}

namespace Manta
{
  union sse_union
  {
    sse_t sse;
    float f[4];
  };

  union sse_int_union
  {
    __m128i ssei;
    int i[4];
  };

#ifndef MANTA_SSE_GCC
  // Stores 1 64 bit int twice in 4 32 bit ints
  //
  // [high32, low32, high32, low32]
  //
  // I'm not sure about the high versus low, but the code does what
  // GCC does (for example, when copying 64 bit pointers).
  static inline
  __m128i _mm_set1_epi64x(long long val)
  {
    const int high = (0xFFFFFFFF00000000LL & val) >> 32 ;
    const int low  = (        0xFFFFFFFFLL & val);
    return _mm_set_epi32(high, low, high, low);
  }

#endif

#ifndef MANTA_SSE_CAST
  // Define some casting functions if the intrinsics don't exist
  static inline __m128i _mm_castps_si128(__m128  val) { union { __m128 f; __m128i i;} c; c.f = val; return c.i; }
  static inline __m128  _mm_castsi128_ps(__m128i val) { union { __m128 f; __m128i i;} c; c.i = val; return c.f; }
  static inline __m128d _mm_castps_pd   (__m128  val) { union { __m128 f; __m128d d;} c; c.f = val; return c.d; }
  static inline __m128  _mm_castpd_ps   (__m128d val) { union { __m128 f; __m128d d;} c; c.d = val; return c.f; }
#endif

  // Keep GCC from complaining about not using these variables.
#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif

  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_eps = _mm_set_ps1(T_EPSILON);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_one = _mm_set_ps1(1.f);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_int_t _mm_one_si128 = _mm_set1_epi32(1);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_int_t _mm_two_si128 = _mm_set1_epi32(2);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_int_t _mm_four_si128 = _mm_set1_epi32(4);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_int_t _mm_inv_one_si128 = _mm_set1_epi32(~1);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_zero = _mm_setzero_ps();
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_int_t _mm_zero_si128 =_mm_setzero_si128();
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_one_half = _mm_set_ps1(0.5f);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_two = _mm_set_ps1(2.f);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_infty = _mm_set_ps1(static_cast<float>(1e308));
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_minus_infty = _mm_set_ps1(static_cast<float>(-1e308));
  static const VARIABLE_IS_NOT_USED int _mm_intabsmask = 0x7fffffff;
  static const VARIABLE_IS_NOT_USED int _mm_intsignbit = 0x80000000;
  static const VARIABLE_IS_NOT_USED int _mm_inttruemask = 0xffffffff;
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_absmask = _mm_set_ps1((float&)_mm_intabsmask);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_signbit = _mm_set_ps1((float&)_mm_intsignbit);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_t _mm_true = _mm_set_ps1((float&)_mm_inttruemask);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_int_t _mm_absmask_si128 = set4i(_mm_intabsmask);
  static const VARIABLE_IS_NOT_USED MANTA_ALIGN(16) sse_int_t _mm_signbit_si128 = set4i(_mm_intsignbit);

  /*! return v0 + t*(v1-v0) */
  inline sse_t lerp4(const sse_t t, const sse_t v0, const sse_t v1)
  {
    return add4(v0,mul4(t,sub4(v1,v0)));
  }

  inline sse_t dot4(const sse_t &ox, const sse_t &oy, const sse_t &oz,
                    const sse_t &vx, const sse_t &vy, const sse_t &vz)
  {
    return _mm_add_ps(_mm_add_ps(_mm_mul_ps(vx,ox),
                                 _mm_mul_ps(vy,oy)),
                      _mm_mul_ps(vz,oz));
  }

  //equivalent to mask ? newD : oldD
  inline sse_t mask4(const sse_t &mask, const sse_t &newD, const sse_t &oldD)
  {
    return or4(and4(mask,newD),andnot4(mask,oldD));
  }

  //equivalent to ~mask ? newD : oldD
  inline sse_t masknot4(const sse_t &mask, const sse_t &newD, const sse_t &oldD)
  {
    return or4(andnot4(mask,newD),and4(mask,oldD));
  }

  inline sse_int_t mask4i(const sse_int_t &mask, const sse_int_t &newD, const sse_int_t &oldD)
  {
    return or4i(and4i(mask,newD),andnot4i(mask,oldD));
  }

  inline sse_t abs4(const sse_t &v)
  {
    return andnot4(_mm_signbit,v);
  }

  inline sse_t accurateReciprocal(const sse_t v)
  {
    const sse_t rcp = _mm_rcp_ps(v);
    return _mm_sub_ps(_mm_add_ps(rcp,rcp),_mm_mul_ps(_mm_mul_ps(rcp,rcp),v));
  }

  inline sse_t uberAccurateReciprocal(const sse_t v)
  {
    sse_t rcp = _mm_rcp_ps(v);
    rcp = _mm_sub_ps(_mm_add_ps(rcp,rcp),_mm_mul_ps(_mm_mul_ps(rcp,rcp),v));
    rcp = _mm_sub_ps(_mm_add_ps(rcp,rcp),_mm_mul_ps(_mm_mul_ps(rcp,rcp),v));
    rcp = _mm_sub_ps(_mm_add_ps(rcp,rcp),_mm_mul_ps(_mm_mul_ps(rcp,rcp),v));
    return rcp;
  }

  inline sse_t oneOver(const sse_t v)
  {
    const sse_t rcp = _mm_rcp_ps(v);
    return _mm_sub_ps(_mm_add_ps(rcp,rcp),_mm_mul_ps(_mm_mul_ps(rcp,rcp),v));
  }
  inline sse_t oneOver4(const sse_t v)
  {
    const sse_t rcp = _mm_rcp_ps(v);
    return _mm_sub_ps(_mm_add_ps(rcp,rcp),_mm_mul_ps(_mm_mul_ps(rcp,rcp),v));
  }

  inline sse_t accurateReciprocalSqrt(const sse_t v)
  {
    const sse_t rcp_sqrt = _mm_rsqrt_ps(v);
    const sse_t one_point_five = _mm_set_ps1(1.5f);
    return _mm_mul_ps(rcp_sqrt, _mm_sub_ps(one_point_five, _mm_mul_ps(_mm_one_half, _mm_mul_ps(_mm_mul_ps(rcp_sqrt,rcp_sqrt),v))));
  }


  inline sse_t reciprocal(const sse_t v)
  {
    return _mm_rcp_ps(v);
  }

  inline sse_t lin(const sse_t &base,
                   float u,const sse_t du,
                   float v, const sse_t dv)
  {
    return _mm_add_ps(_mm_add_ps(base,_mm_mul_ps(_mm_set_ps1(u),du)),
                      _mm_mul_ps(_mm_set_ps1(v),dv));
  }

  inline sse_t lin(const sse_t &base,
                   const sse_t &u,const sse_t du,
                   const sse_t &v, const sse_t dv)
  {
    return _mm_add_ps(_mm_add_ps(base,_mm_mul_ps(u,du)),
                      _mm_mul_ps(v,dv));
  }


  inline sse_t dot4(const sse_t &a, const sse_t &b)
  {
    const sse_t xyzw = _mm_mul_ps(a,b);
    const sse_t zwxy = _mm_shuffle_ps(xyzw,xyzw,_MM_SHUFFLE(1,0,3,2));
    const sse_t xz_yw_zx_wy = _mm_add_ps(zwxy,xyzw);
    const sse_t wy_zx_yw_xz = _mm_shuffle_ps(xz_yw_zx_wy,xz_yw_zx_wy,_MM_SHUFFLE(0,1,2,3));
    const sse_t res = _mm_add_ps(xz_yw_zx_wy,wy_zx_yw_xz);
    return res;
  }

  inline float dot1(const sse_t &a, const sse_t &b)
  {
    const sse_t d = dot4(a,b);
    return (float&)d;
  }

  inline void normalize(sse_t &v)
  {
    const sse_t dot = dot4(v,v);
    v = _mm_mul_ps(v, _mm_rsqrt_ps(dot));
  };

  inline float sqrLength(const sse_t &a)
  {
    return dot1(a,a);
  }

  inline float length(sse_t a)
  {
    const sse_t d = dot4(a,a);
    const sse_t v = sqrt4(d);
    return (float &)v;
  }

  // Get horizontal minimum of all components
  inline float min4f(sse_t t)
  {
    // a = (t0, t0, t1, t1)
    sse_t a = unpacklo(t,t);
    // b = (t2, t2, t3, t3)
    sse_t b = unpackhi(t,t);
    // c = (min(t0,t2), min(t0, t2), min(t1, t3), min(t1, t3))
    sse_t c = min4(a, b);
    // The movehl will move the high 2 values to the low 2 values.
    // This will allow us to compare min(t0,t2) with min(t1, t3).
    sse_t min = _mm_min_ss(c, _mm_movehl_ps(c, c));
    // Return the first value.
    return *((float*)&min);
  }

  // Get horizontal maximum of all components
  inline float max4f(sse_t t)
  {
    // a = (t0, t0, t1, t1)
    sse_t a = unpacklo(t,t);
    // b = (t2, t2, t3, t3)
    sse_t b = unpackhi(t,t);
    // c = (max(t0,t2), max(t0, t2), max(t1, t3), max(t1, t3))
    sse_t c = max4(a, b);
    // The movehl will move the high 2 values to the low 2 values.
    // This will allow us to compare max(t0,t2) with max(t1, t3).
    sse_t max = _mm_max_ss(c, _mm_movehl_ps(c, c));
    // Return the first value.
    return *((float*)&max);
  }

  // Get horizontal minimum of the frist 3 components
  inline float min3f(sse_t t)
  {
    // a = (t0, t0, t1, t1), you might be tempted to make this a movelh,
    // but you need t1 to be in the 2 index in order to use movehl
    // later.
    sse_t a = unpacklo(t,t);
    // b = (t2, t3, t2, t3)
    sse_t b = _mm_movehl_ps(t,t);
    // c = (min(t0,t2), min(t0, t3), min(t1, t2), min(t1, t3))
    sse_t c = min4(a, b);
    // The movehl will move the high 2 values to the low 2 values.
    // This will allow us to compare t1 with min(t0, t2).
    sse_t min = _mm_min_ss(c, _mm_movehl_ps(a, a));
    // Return the first value.
    return *((float*)&min);
  }

  // Get horizontal maximum of the frist 3 components
  inline float max3f(sse_t t)
  {
    // a = (t0, t0, t1, t1), you might be tempted to make this a movelh,
    // but you need t1 to be in the 2 index in order to use movehl
    // later.
    sse_t a = unpacklo(t,t);
    // b = (t2, t3, t2, t3)
    sse_t b = _mm_movehl_ps(t,t);
    // c = (max(t0,t2), max(t0, t3), max(t1, t2), max(t1, t3))
    sse_t c = max4(a, b);
    // The movehl will move the high 2 values to the low 2 values.
    // This will allow us to compare t1 with max(t0, t2).
    sse_t max = _mm_max_ss(c, _mm_movehl_ps(a, a));
    // Return the first value.
    return *((float*)&max);
  }

  inline float simd_component(sse_t t, int offset)
  {
    MANTA_ALIGN(16)
      float f[4];
    _mm_store_ps(f,t);
    return f[offset];
  }

  inline void simd_cerr(sse_t t)
  {
    std::cerr << t << std::endl;
  }

  inline Vec3f as_Vec3f(sse_t t)
  {
    MANTA_ALIGN(16)
      float f[4];
    _mm_store_ps(f,t);
    return Vec3f(f[2], f[1], f[0]);
  }

  inline Vector as_Vector(sse_t t)
  {
    MANTA_ALIGN(16)
      float f[4];
    _mm_store_ps(f,t);
    return Vector(f[2], f[1], f[0]);
  }

  inline int count_nonzeros(sse_t t)
  {
    int mask = _mm_movemask_ps(t);
    // Isolate each bit with a mask, then shift it down to the one's
    // spot.  Look, ma.  No branches!
    int nonzeros = (((mask & (1 << 0)) >> 0) +
                    ((mask & (1 << 1)) >> 1) +
                    ((mask & (1 << 2)) >> 2) +
                    ((mask & (1 << 3)) >> 3));
    return nonzeros;
  }

  inline int count_zeros(sse_t t)
  {
    // You could use this if your compiler isn't stupid.
    // return 4-count_nonzeros(t);

    // But most compilers are stupid, so we'll manually inline it:
    int mask = _mm_movemask_ps(t);
    // Isolate each bit with a mask, then shift it down to the one's
    // spot.  Look, ma.  No branches!
    int nonzeros = (((mask & (1 << 0)) >> 0) +
                    ((mask & (1 << 1)) >> 1) +
                    ((mask & (1 << 2)) >> 2) +
                    ((mask & (1 << 3)) >> 3));
    return 4-nonzeros;
  }

  // The sse code below is based off of this algorithm

  //   float frac = d-(int)d;
  //   if (frac < 0)
  //     return frac+1;
  //   else
  //     return frac;

  inline __m128 fracSSE(__m128 val)
  {
    __m128 fract_val = _mm_sub_ps(val, _mm_cvtepi32_ps(_mm_cvttps_epi32(val)));
    return _mm_add_ps(fract_val, _mm_and_ps(_mm_cmplt_ps(fract_val, _mm_set_ps1(0.f)), _mm_set_ps1(1.f)));
  }

  // This is based off of:
  //
  // floor(val) = val - frac(val);
  inline __m128 floorSSE(__m128 val)
  {
    __m128 fract_val = _mm_sub_ps(val, _mm_cvtepi32_ps(_mm_cvttps_epi32(val)));
    return _mm_sub_ps(val, _mm_add_ps(fract_val, _mm_and_ps(_mm_cmplt_ps(fract_val, _mm_set_ps1(0.f)), _mm_set_ps1(1.f))));
  }

  // Packed 32 bit integer multiplication with truncation of the upper
  // halves of the results.  This produces the same resaults as
  // (int)(int * int).  It looks as though this function will be
  // included in SSE4, but the rest of us need to use it now.
  //
  // I found this code on this web page:
  // http://www.intel.com/cd/ids/developer/asmo-na/eng/254761.htm?page=4
  // There isn't any indication that we can't simply include it in our
  // own code.
  inline __m128i _mm_mullo_epi32( __m128i a, __m128i b )
  {
#if 1
    __m128i t0;
    __m128i t1;

    t0 = _mm_mul_epu32(a,b);
    t1 = _mm_mul_epu32( _mm_shuffle_epi32( a, 0xB1 ),
                        _mm_shuffle_epi32( b, 0xB1 ) );

    t0 = _mm_shuffle_epi32( t0, 0xD8 );
    t1 = _mm_shuffle_epi32( t1, 0xD8 );

    return _mm_unpacklo_epi32( t0, t1 );
#else
    // Here's a version that Bigler came up with that is faster on
    // some systems like the Opteron.
    __m128i a_lo = _mm_unpacklo_epi32(a, a);   // a0a0a1a1
    __m128i a_hi = _mm_unpackhi_epi32(a, a);   // a2a2a3a3
    __m128i b_lo = _mm_unpacklo_epi32(b, b);   // b0b0b1b1
    __m128i b_hi = _mm_unpackhi_epi32(b, b);   // b2b2b3b3
    __m128i lo   = _mm_mul_epu32(a_lo, b_lo);  // a0b0l a0b0h a1b1l a1b1h
    __m128i hi   = _mm_mul_epu32(a_hi, b_hi);  // a2b2l a2b2h a3b3l a3b3h
    __m128i mix_lo = _mm_unpacklo_epi32(lo, hi); // a0b0l a2b2l a0b0h a2b2h
    __m128i mix_hi = _mm_unpackhi_epi32(lo, hi); // a1b1l a3b3l a1b1h a3b3h
    __m128i product = _mm_unpacklo_epi32(mix_lo, mix_hi); // a0b0l a1b1l a2b2l a3b3l

    return product;
#endif
  }

  // For trig functions like sin and cos, see TrigSSE.h

  // For functions that compute pow, exp, log, exp2, and log2, see ExponSSE.h
};

#endif  //#ifdef MANTA_SSE
#endif
