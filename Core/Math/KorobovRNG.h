/* This is licensed under the MIT license:

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

#include <MantaTypes.h>
#include <Interface/RandomNumberGenerator.h>
#include <MantaSSE.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Math/SSEDefs.h>

#include <climits>

namespace Manta {

  class KorobovRNG: public RandomNumberGenerator {
  public:
  protected:

    char padding0 [128];

    // The single index and depth for a "ray"
    unsigned int sample_index;
    unsigned int sample_depth;

    // A packet of sample indices and depths.  Since we assume these
    // are either identical (depth) or derived (index + offset per
    // ray) we only store one.
    unsigned int packet_index;
    unsigned int packet_depth;
    unsigned int a;
    unsigned int num_spp;
    double inv_spp;
    static const unsigned int MAX_SAMPLE_DEPTH = 64;

    unsigned int g_table[MAX_SAMPLE_DEPTH];

    char padding1 [128];
  public:
    // Your seed value is up to the fates.  You should call seed.
    KorobovRNG(unsigned int a_val, unsigned int num_samples) :
      a(a_val), num_spp(num_samples) {
      // Compute g_table
      a %= num_spp;
      g_table[0] = a;
      for (unsigned int i = 1; i < MAX_SAMPLE_DEPTH; i++) {
        g_table[i] = (a * g_table[i-1]) % num_spp;
      }
      // Compute 1/spp
      inv_spp = 1./static_cast<double>(num_spp);

      // Initialize some stuff to 0
      sample_index = 0;
      sample_depth = 0;

      packet_index = 0;
      packet_depth = 0;
    }

    static void create(RandomNumberGenerator*& rng,
                       unsigned int a,
                       unsigned int num_spp) {
      rng = new KorobovRNG(a, num_spp);
    }

    virtual void seed(unsigned int seed_val) {
      sample_index = 0;
      sample_depth = 0;

      packet_index = 0;
      packet_depth = 0;
    }

    virtual void nextPacket(Packet<float>& results) {
      for (int i = 0; i < Packet<float>::MaxSize; i++) {
        float sample = fmod(inv_spp * (( (packet_index + i) * g_table[packet_depth]) % num_spp), 1.0);
        results.set(i, sample);
      }
      if (packet_depth >= MAX_SAMPLE_DEPTH) {
        packet_depth %= MAX_SAMPLE_DEPTH;
      }
      packet_depth++;
    }

    virtual unsigned int nextInt() {
      return static_cast<int>(nextDouble() * UINT_MAX);
    }

    virtual double nextDouble() {
      // TODO(boulos): Add padding (note the sample_depth++)
      throw InternalError("Korobov::nextDouble is for suckers");
      return fmod(inv_spp *((sample_index*g_table[sample_depth++])%num_spp),
                  1.0);
    }

    virtual float nextFloat() {
      return nextDouble();
    }
  }; // end class KorobovRNG

} // end namespace Manta
