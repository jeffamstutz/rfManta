#ifndef Manta_Interface_RandomNumberGenerator_h
#define Manta_Interface_RandomNumberGenerator_h

/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2003-2005
  Scientific Computing and Imaging Institute, University of Utah

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

/*
 */

#include <MantaSSE.h>
#include <Interface/Packet.h>
#include <Interface/RayPacket.h>
#include <Core/Exceptions/InternalError.h>

namespace Manta {
  class RandomNumberGenerator {
  public:
    virtual ~RandomNumberGenerator() {}

    virtual void seed(unsigned int val) = 0;

    // result = [0, 4294967295(UINT_MAX)]
    virtual unsigned int nextInt() = 0;

    virtual void nextIntPacket(Packet<unsigned int>& results, RayPacket& rays) {
      for (int i = rays.begin(); i < rays.end(); i++) {
        results.set(i, nextInt());
      }
    }
    // All the floating point interface return numbers in the range of
    // [0, 1).

    // This function can't be virtual, since it is a template member
    // function.
    template<class T>
    T next() { return 0; }

    // NOTE(boulos): Because random number generators in Core/Math
    // like CheapRNG rely on this function there must be an
    // implementation available to like the Manta_Core library.  On
    // Linux, it's okay to leave this undefined (and it'll just find
    // it in Manta_Interface.lib at runtime) but Mac OS X is picky and
    // wants libraries to be loadable on their own.  So for now, this
    // implementation is also in the header.
    virtual void nextPacket(Packet<float>& results, RayPacket& rays) {
#ifdef MANTA_SSE
      Packet<unsigned int> uints;
      nextIntPacket(uints, rays);
      int b = (rays.begin() + 3) & ~3;
      int e = rays.end() & ~3;
      if (b >= e) {
        for (int i = rays.begin(); i < rays.end(); i++) {
          results.set(i, (float)(2.3283062977608182E-10) * (float)uints.get(i));
        }
      } else {
        for (int i = rays.begin(); i < b; i++) {
          results.set(i, (float)(2.3283062977608182E-10) * (float)uints.get(i));
        }
        for (int i = b; i < e; i+= 4) {
          // NOTE(boulos): SSE lacks unsigned ints (great design), so
          // we instead do the following: Mask out the top bit (since
          // that's a signed number) but divide by 2/4294967296
          // instead
          static const float scale_float = (float)(2*2.3283062977608182E-10);
          _mm_store_ps(&results.data[i],
                       _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_load_si128((__m128i*)&uints.data[i]), 1)),
                                  _mm_set1_ps(scale_float)));
        }
        for (int i = e; i < rays.end(); i++) {
          results.set(i, (float)(2.3283062977608182E-10) * (float)uints.get(i));
        }
      }
#else
      for (int i = rays.begin(); i < rays.end(); i++) {
        results.set(i, nextFloat());
      }
#endif
    }

    virtual void nextPacket(Packet<double>& results, RayPacket& rays) {
      for (int i = rays.begin(); i < rays.end(); i++) {
        results.set(i, nextDouble());
      }
    }


    //Since these are not virtual, need to be careful that child
    //classes don't break. Inlining removes a sizable call
    //overhead.
    inline double nextDouble() {
      return ( (double)nextInt() * (1./4294967296.) );
    }

    // Due to the standard floating point rounding mode being to round to
    // nearest and not truncate down, for ints close to 2^32, when we divide by
    // 2^-32 we might get 1.
    // Once again: (2^32-k)*2^-32 == 1 for small values of k!
    //
    // We fix this by dividing by a slightly larger number so that the rounding
    // will end up going down. The actual floating point number in hex format
    // is 0x2F7FFFFF, which is equal to 1*2^-33*1.9999999
    // This is not an issue with doubles since it has enough precision.
    inline float nextFloat() {
      return ( (float)nextInt() * (float)(2.3283062977608182E-10) );
    }
    inline Real nextReal() {
      return ( (Real)nextInt() * (Real)(2.3283062977608182E-10) );
    }
  };

  // Don't put the implementations here, because you will get multiply
  // defined symbols.
  template<>
  float RandomNumberGenerator::next<float>();

  template<>
  double RandomNumberGenerator::next<double>();

} // end namespace Manta

#endif // #ifndef Manta_Interface_RandomNumberGenerator_h
