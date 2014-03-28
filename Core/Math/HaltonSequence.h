#ifndef MANTA_CORE_MATH_HALTON
#define MANTA_CORE_MATH_HALTON

/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2006
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

#include <MantaTypes.h>
#include <Core/Geometry/VectorT.h>

namespace Manta {

  template< typename T, int S >
  class HaltonSequence {
  public:
    HaltonSequence( int n = 1 ) : next_value( n ) { };

    // Generate the next number in the sequence.
    void next( VectorT<T,S>& p ) {
      static const int halton_sequence_primes[] = { 2, 3, 5, 7 };      
      for (int i=0;i<S;++i) {
        p[i] = folded_radical_inverse( next_value, halton_sequence_primes[i] );
      }
      ++next_value;
    }
  
  private:

    inline Real radical_inverse( int n, int b ) {
    
      Real val = 0;
      Real inv_b = 1 / (Real)b;
      Real inv_bi = inv_b;
    
      while (n > 0) {
        int di = n % b;
        val += di * inv_bi;
        n /= b;
        inv_bi *= inv_b;
      }
    
      return val;
    };
  
    inline Real folded_radical_inverse( int n, int b ) {

      Real val = 0;
      Real inv_b = 1 / (Real)b;
      Real inv_bi = inv_b;
      int offset = 0;
    
      while ((val+b*inv_bi) != val) {
        int d = ((n+offset) % b);
        val += d * inv_bi;
        n /= b;
        inv_bi *= inv_b;
        ++offset;
      }
    
      return val;
    };

    // Private members.
    int next_value;
  };

};


#endif
