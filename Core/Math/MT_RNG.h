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

#ifndef __MT19937_H__
#define __MT19937_H__

#include <Interface/RandomNumberGenerator.h>

namespace Manta {

// Period parameters
#define MT_RNG_N 624
#define MT_RNG_M 397

class MT_RNG : public RandomNumberGenerator {
  char padding0[128];
  unsigned int mt[MT_RNG_N]; // the array for the state vector
  int mti; // mti==MT_RNG_N+1 means mt[MT_RNG_N] is not initialized
  char padding1[128];
public:
  MT_RNG(): mti(MT_RNG_N+1) {}
  static void create(RandomNumberGenerator*& rng ) { rng = new MT_RNG(); }
  void seed(unsigned int seed);
  unsigned int nextInt();
  
  void test_print(); // prints some random numbers to the console
};

} // end namespace Manta

#endif // __MT19937_H__
