#ifndef MT_RNG_SIMD_H__
#define MT_RNG_SIMD_H__

#include <Core/Math/SSEDefs.h>

namespace Manta {

#ifdef MANTA_SSE
  struct MT_RNG_SIMD
  {
    sse_int_t state[ 624 ];
    int position;
  };

  inline void seed_MT_RNG_SIMD( MT_RNG_SIMD& that,
                                sse_int_t seed )
  {
    that.state[ 0 ] = seed;
    for ( that.position = 1; that.position < 624; ++that.position ) {
      union
      {
        sse_int_t partial;
        unsigned int ints[ 4 ];
      };
      partial = xor4i( that.state[ that.position - 1 ],
                       shift_right4int( that.state[ that.position - 1 ], 30 ) );
      ints[ 0 ] *= 0x6c078965;
      ints[ 1 ] *= 0x6c078965;
      ints[ 2 ] *= 0x6c078965;
      ints[ 3 ] *= 0x6c078965;
      that.state[ that.position ] = add4i( partial, set4i( that.position ) );
    }
  }

  inline sse_t next_MT_RNG_SIMD( MT_RNG_SIMD& that )
  {
    if ( that.position == 624 ) {
      static const sse_int_t magic = set4i( 0x9908b0df );
      for ( int index = 0; index < 227; ++index ) {
        sse_int_t mix = or4i( and4i( that.state[ index ], _mm_signbit_si128 ),
                              and4i( that.state[ index + 1 ], _mm_absmask_si128 ) );
        sse_int_t magic_mix = mask4i( cmp4_eq_i( and4i( mix, _mm_one_si128 ), _mm_one_si128 ),
                                      magic, _mm_zero_si128 );
        that.state[ index ] = xor4i( xor4i( that.state[ index + 397 ],
                                            shift_right4int( mix, 1 ) ),
                                     magic_mix );
      }
      for ( int index = 227; index < 623; ++index ) {
        sse_int_t mix = or4i( and4i( that.state[ index ], _mm_signbit_si128 ),
                              and4i( that.state[ index + 1 ], _mm_absmask_si128 ) );
        sse_int_t magic_mix = mask4i( cmp4_eq_i( and4i( mix, _mm_one_si128 ), _mm_one_si128 ),
                                      magic, _mm_zero_si128 );
        that.state[ index ] = xor4i( xor4i( that.state[ index - 227 ],
                                            shift_right4int( mix, 1 ) ),
                                     magic_mix );
      }
      sse_int_t mix = or4i( and4i( that.state[ 623 ], _mm_signbit_si128 ),
                            and4i( that.state[ 0 ], _mm_absmask_si128 ) );
      sse_int_t magic_mix = mask4i( cmp4_eq_i( and4i( mix, _mm_one_si128 ), _mm_one_si128 ),
                                    magic, _mm_zero_si128 );
      that.state[ 623 ] = xor4i( xor4i( that.state[ 396 ],
                                        shift_right4int( mix, 1 ) ),
                                 magic_mix );
      that.position = 0;
    }
    static const MANTA_ALIGN(16) sse_int_t magic2 = set4i( 0x9d2c5680 );
    static const MANTA_ALIGN(16) sse_int_t magic3 = set4i( 0xefc60000 );
    sse_int_t value = that.state[ that.position++ ];
    value = xor4i( value, shift_right4int( value, 11 ) );
    value = xor4i( value, and4i( shift_left4int( value, 7 ), magic2 ) );
    value = xor4i( value, and4i( shift_left4int( value, 15 ), magic3 ) );
    value = xor4i( value, shift_right4int( value, 18 ) );
    static const MANTA_ALIGN(16) sse_t scale = set4( 1.0f / 4294967296.0f );
    return add4( mul4( convert4_i2f( value ), scale ), _mm_one_half );
  }

#endif // #ifdef MANTA_SSE
  
} // end namespace Manta

#endif // #ifndef MT_RNG_SIMD_H__
