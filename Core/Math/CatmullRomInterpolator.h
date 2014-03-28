
/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

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

#ifndef __CATMULLROMINTERPOLATOR_H__
#define __CATMULLROMINTERPOLATOR_H__

#include <MantaTypes.h>

namespace Manta {

  template< typename PointIterator, typename PointType >
  void catmull_rom_interpolate( const PointIterator &P_iter, Real t, PointType &result ) {

    PointType &P_i   = *(P_iter);   // "Pi"
    PointType &P_im1 = *(P_iter-1); // "Pi minus 1"
    PointType &P_ip1 = *(P_iter+1); // "Pi plus  1"
    PointType &P_ip2 = *(P_iter+2); // "Pi plus  2"

    Real t2 = t*t;
    Real t3 = t*t2;

    PointType r0 = (P_i  * 2);
    PointType r1 = (P_im1*-1 + P_ip1) * t;
    PointType r2 = (P_im1* 2 + P_i*-5 + P_ip1* 4 -P_ip2) * t2;
    PointType r3 = (P_im1*-1 + P_i* 3 + P_ip1*-3 +P_ip2) * t3;
    
    result = (r0 + r1)*(Real)0.5 + (r2 + r3)*(Real)0.5;
  }
  
  
};

#endif

