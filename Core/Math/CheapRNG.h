/*
 * CheapRNG
 *
 * Cheap refers to the cost to store and to seed the random number.
 * The speed is about twice as slow as the Mersenne Twister, however
 * seeding is several orders of magnitude faster.  This is useful if
 * you want to reseed for only a relatively few samples and you just
 * want a reasonably random sequence.
 *
 * The period for this RNG is 2^32 with ranges of [0..4294967295] (or
 * [0..2^32-1]).  Consequently the floating point results are
 * [0..1].  If you want an exclusive interval [0..1) divide genuint()
 * by 4294967296.0.
 *
 * The algorithms are taken from chapter 7 of Numerical Recipies in C:
 * The Art of Scientific Computing.
 *
 * Author: James Bigler
 * Date: June 16, 2005
 *
 * This is licensed under the MIT license:

  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  SCI Institute, University of Utah

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

 */

// All these values are u_int32_t to make sure we have an unsigned 32
// bit integer, because the computation relies on the overflow
// properties of integer multiplication.

// For the Real type
#include <MantaTypes.h>
#include <Interface/RandomNumberGenerator.h>
#include <MantaSSE.h>
#include <Core/Math/SSEDefs.h>
#include <Core/Util/AlignedAllocator.h>

namespace Manta {

  class CheapRNG: public RandomNumberGenerator,
                  public AlignedAllocator<CheapRNG> {
  public:
    // You need to make sure that this is 32 bits or you are in
    // trouble.  It also needs to be public to initialize the static
    // members.
    typedef unsigned int uint32;
  protected:

    char padding0 [128];
    uint32 val;
#ifdef MANTA_SSE
    __m128i val_sse;
#endif
    char padding1 [128];
  public:
    // Your seed value is up to the fates.  You should call seed.
    CheapRNG() {}
    static void create(RandomNumberGenerator*& rng ) { 
      rng = new CheapRNG(); 
      rng->seed(0);
    }

    void seed(unsigned int seed_val) {
      val = seed_val;
#ifdef MANTA_SSE
      // TODO(boulos): Find a better way to seed this.  Normally you
      // could just call nextInt() but all that will do for this RNG
      // is make the sequences out of sync by one iteration... So you
      // want offsets.
      val_sse = _mm_set_epi32(seed_val,
                              seed_val + 1,
                              seed_val,
                              seed_val + 1);
#endif
    }

    unsigned int nextInt() {
      val = 1664525*val + 1013904223;
      return val;
    }

    void nextIntPacket(Packet<unsigned int>& results, RayPacket& rays) {
#ifdef MANTA_SSE
      for (int i = 0; i < Packet<unsigned int>::MaxSize; i+=4) {
        val_sse = _mm_add_epi32(_mm_set_epi32(2531011, 10395331, 13737667, 1),
                                _mm_mullo_epi32(val_sse, _mm_set_epi32(214013, 17405, 214013, 69069)));
        _mm_store_si128((__m128i*)&results.data[i], val_sse);
      }
#else
      for (int i = 0; i < Packet<unsigned int>::MaxSize; i++) {
        results.set(i, nextInt());
      }
#endif
    }
  }; // end class CheapRNG

} // end namespace Manta
