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

#ifndef Manta_Endian_h
#define Manta_Endian_h

#include <Core/Geometry/VectorT.h>
#include <MantaTypes.h>
#include <Core/Color/ColorSpace.h>

namespace Manta {

  ///////////////////////////////////////////////////////////////////////////
  // Swap the endianness of a variable.
  template< typename T >
  inline T endian_swap( const T &in ) {
    T out;
    char const * const pin  = (char *)&in;
    char * const pout = (char *)&out;
    
    for (unsigned int i=0;i<sizeof(T);++i) {
      pout[i] = pin[sizeof(T)-1-i];
    }
    
    return out;
  }
  
  // Check the endianness of this machine.
  inline bool is_big_endian() {
    
    unsigned int x = 0x00112233;
    char * const p = (char *)&x;
    
    return (p[0] == 0x00);
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Specializations.
  template<>
  inline VectorT<Real,3> endian_swap( const VectorT<Real, 3> &in ) {
    VectorT<Real,3> out;
    for (int i=0;i<3;++i) {
      out[i] = endian_swap( in[i] );
    }
    return out;
  }

  template<>
  inline ColorSpace<RGBTraits> endian_swap( const ColorSpace<RGBTraits> &in ) {
    ColorSpace<RGBTraits> out;
    const int size = (ColorSpace<RGBTraits>::NumComponents);
    for (int i=0;i<size;++i) {
      out[i] = endian_swap( in[i] );
    }
    return out;
  }
};
  
#endif
