/* A C++-program for MT19937: Adapted to c++ by James Bigler  */


/*  genrand() generates one pseudorandom unsigned integer (32bit) */
/* which is uniformly distributed among 0 to 2^32-1  for each     */
/* call. seed(seed) set initial values to the working area    */
/* of 624 words. Before genrand(), seed(seed) must be         */
/* called once. (seed is any 32-bit integer except for 0).        */
/*   Coded by Takuji Nishimura, considering the suggestions by    */
/* Topher Cooper and Marc Rieffel in July-Aug. 1997.              */

/* This library is free software; you can redistribute it and/or   */
/* modify it under the terms of the GNU Library General Public     */
/* License as published by the Free Software Foundation; either    */
/* version 2 of the License, or (at your option) any later         */
/* version.                                                        */
/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of  */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.            */
/* See the GNU Library General Public License for more details.    */
/* You should have received a copy of the GNU Library General      */
/* Public License along with this library; if not, write to the    */
/* Free Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA   */ 
/* 02111-1307  USA                                                 */

/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.       */
/* Any feedback is very welcome. For any question, comments,       */
/* see http://www.math.keio.ac.jp/matumoto/emt.html or email       */
/* matumoto@math.keio.ac.jp                                        */

#include <stdio.h> // used only for test_print
#include <Core/Math/MT_RNG.h>
#include <MantaSSE.h>
#include <Core/Math/SSEDefs.h>

using namespace Manta;

/* Period parameters */  
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)


/* initializing the array with a NONZERO seed */
void MT_RNG::seed(unsigned int seed)
{
  /* setting initial seeds to mt[MT_RNG_N] using         */
  /* the generator Line 25 of Table 1 in          */
  /* [KNUTH 1981, The Art of Computer Programming */
  /*    Vol. 2 (2nd Ed.), pp102]                  */

  //To speed things up, we manually unroll 4 times. MT_RNG_N is
  //divisable by 4, so this is ok.
  ASSERT(MT_RNG_N/4 == 0);

  mt[0] = seed;
  mt[1] = 69069*seed;
  mt[2] = 69069*69069*seed;
  mt[3] = 69069*69069*69069*seed;

  //Note: writing this loop in sse with _mm_mullo_epi32 actually ends up
  //being 2x slower than the non-sse version below because the
  //_mm_mullo_epi32 instruction becomes many assembly instructions.
  for (mti=4; mti<MT_RNG_N; mti+=4) {
    mt[mti]   = (69069*69069*69069*69069) * mt[mti-4];
    mt[mti+1] = (69069*69069*69069*69069) * mt[mti-3];
    mt[mti+2] = (69069*69069*69069*69069) * mt[mti-2];
    mt[mti+3] = (69069*69069*69069*69069) * mt[mti-1];
  }
}

unsigned int MT_RNG::nextInt()
{
  unsigned int y;
  static unsigned int mag01[2]={0x0, MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */
  
  if (mti >= MT_RNG_N) { /* generate MT_RNG_N words at one time */
    int kk;
    
    if (mti == MT_RNG_N+1)   /* if seed() has not been called, */
      seed(4357); /* a default initial seed is used   */
    
    for (kk=0;kk<MT_RNG_N-MT_RNG_M;kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+MT_RNG_M] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    for (;kk<MT_RNG_N-1;kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+(MT_RNG_M-MT_RNG_N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    y = (mt[MT_RNG_N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
    mt[MT_RNG_N-1] = mt[MT_RNG_M-1] ^ (y >> 1) ^ mag01[y & 0x1];
    
    mti = 0;
  }
  
  y = mt[mti++];
  y ^= TEMPERING_SHIFT_U(y);
  y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
  y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
  y ^= TEMPERING_SHIFT_L(y);
  
  return y; 
}

void MT_RNG::test_print() { 
  int j;

  seed(4357); // any nonzero integer can be used as a seed
  for (j=0; j<1000; j++) {
    printf("%5f ", nextDouble());
    if (j%8==7) printf("\n");
  }
  printf("\n");
}
