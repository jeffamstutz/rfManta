/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

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



#ifndef Math_Trig_h
#define Math_Trig_h 1

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2 // PI/2.0
#define M_PI_2 1.57079632679489661923
#endif

namespace Manta {

  const double Pi=M_PI;

#ifdef _WIN32

  inline double acosh(double x)
  {
    return (x<1) ? log(-1.0) : log(x+sqrt(x*x-1));
  }

#endif

  inline double Cos(double d)
  {
    return cos(d);
  }

  inline double Sin(double d)
  {
    return sin(d);
  }

  inline double Asin(double d)
  {
    return asin(d);
  }

  inline double Acos(double d)
  {
    return acos(d);
  }

  inline double Tan(double d)
  {
    return tan(d);
  }

  inline double Cot(double d)
  {
    return 1./tan(d);
  }

  inline double Atan(double d)
  {
    return atan(d);
  }

  inline double Atan2(double y, double x)
  {
    return atan2(y, x);
  }

  inline double DtoR(double d)
  {
    return d*(M_PI/180.);
  }

  inline double RtoD(double r)
  {
    return r*(180./M_PI);
  }

  inline double ACosh(double x)
  {
    return acosh(x);
  }

  inline void SinCos(double d, double &s, double &c) {
#if defined(_GNU_SOURCE) && !(__APPLE__)
      sincos(d, &s, &c);
#else
      s = Sin(d);
      c = Cos(d);
#endif
  }

/////////////////////////////////////////////////////
//
// Float versions
//

  inline float Sin(float d)
  {
    return sinf(d);
  }

  inline float Cos(float d)
  {
    return cosf(d);
  }

  inline float Tan(float d)
  {
    return tanf(d);
  }

  inline float Atan(float d)
  {
    return atanf(d);
  }

  inline float Atan2(float y, float x)
  {
    return atan2f(y, x);
  }

  // much faster approximate atan2f with |error| < 0.005
  // http://lists.apple.com/archives/perfoptimization-dev/2005/Jan/msg00051.html
  inline float fast_atan2f( float y, float x )
  {
    if ( x == 0.0f )
    {
      if ( y > 0.0f ) return static_cast<float>(M_PI_2);
        if ( y == 0.0f ) return 0.0f;
        return -static_cast<float>(M_PI_2);
    }
    float atan;
    float z = y/x;
    if ( fabsf( z ) < 1.0f )
    {
        atan = z/(1.0f + 0.28f*z*z);
        if ( x < 0.0f )
        {
          if ( y < 0.0f ) return atan - static_cast<float>(M_PI);
          return atan + static_cast<float>(M_PI);
        }
    }
    else
    {
      atan = static_cast<float>(M_PI_2) - z/(z*z + 0.28f);
      if ( y < 0.0f ) return atan - static_cast<float>(M_PI);
    }
    return atan;
  }

  inline float fast_atanf(float y) { return fast_atan2f(y, 1); }

  inline float Acos(float d)
  {
    return acosf(d);
  }

  inline float DtoR(float d)
  {
    return d*(float)(M_PI/180.0);
  }

  inline float RtoD(float r)
  {
    return r*(float)(180.0/M_PI);
  }

  inline void SinCos(float d, float &s, float &c) {
#if defined(_GNU_SOURCE) && !(__APPLE__)
      sincosf(d, &s, &c);
#else
      s = Sin(d);
      c = Cos(d);
#endif
  }
}

#endif
