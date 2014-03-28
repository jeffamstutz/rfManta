
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

/*

  Vector expects Real to be a floating point type.  If you use an
  integral type, you could get into trouble with certain functions
  that compute division.  Be warned!

*/

#ifndef Manta_Core_Vector_h
#define Manta_Core_Vector_h

#include <MantaTypes.h>
#include <Core/Math/Expon.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/MiscMath.h>
#include <Core/Persistent/MantaRTTI.h>
#include <iosfwd>

namespace Manta {
  template<typename T, int Dim> class VectorT;

  class Vector {
  public:
    typedef Real ComponentType;

    Vector() {
    }
    Vector(Real data_in[3]) {
      data[0] = data_in[0]; data[1] = data_in[1]; data[2] = data_in[2];
    }
    Vector(Real x, Real y, Real z) {
      data[0] = x; data[1] = y; data[2] = z;
    }

    Vector( const Vector& copy) {
      data[0] = copy.data[0];
      data[1] = copy.data[1];
      data[2] = copy.data[2];
    }

    // Copy from another vector of the same size, but of a templated type.
    template< typename S >
    Vector(const VectorT<S, 3>& copy) {
      data[0] = static_cast<Real>(copy[0]);
      data[1] = static_cast<Real>(copy[1]);
      data[2] = static_cast<Real>(copy[2]);
    }

#ifndef SWIG
    Vector& operator=(const Vector& copy) {
      data[0] = copy.data[0];
      data[1] = copy.data[1];
      data[2] = copy.data[2];
      return *this;
    }
#endif

    ~Vector() {
    }

    Real x() const {
      return data[0];
    }
    Real y() const {
      return data[1];
    }
    Real z() const {
      return data[2];
    }

#ifndef SWIG
    const Real& operator[] (int i) const {
      return data[i];
    }
    Real& operator[] ( int i ) {
      return data[i];
    }
#else
    %extend {
      Real __getitem__( int i ) {
        return self->operator[](i);
      }
      void __setitem__(int i,Real val) {
        self->operator[](i) = val;
      }
    }
#endif
    // One might be tempted to add an "operator &" function, but
    // that could lead to problems when you want an address to the
    // object rather than a pointer to data.
    const Real* getDataPtr() const { return data; }

    // + operator
    Vector operator+(const Vector& v) const {
      Vector result;
      result.data[0] = data[0] + v.data[0];
      result.data[1] = data[1] + v.data[1];
      result.data[2] = data[2] + v.data[2];
      return result;
    }
    Vector& operator+=(const Vector& v) {
      data[0] += v.data[0];
      data[1] += v.data[1];
      data[2] += v.data[2];
      return *this;
    }
    Vector operator+(Real s) const {
      Vector result;
      result.data[0] = data[0] + s;
      result.data[1] = data[1] + s;
      result.data[2] = data[2] + s;
      return result;
    }
    Vector& operator+=(Real s) {
      data[0] += s;
      data[1] += s;
      data[2] += s;
      return *this;
    }
    template<typename S >
    Vector operator+(S s) const
    {
      Real scalar = static_cast<Real>(s);
      Vector result(data[0] + scalar,
                    data[1] + scalar,
                    data[2] + scalar);
      return result;
    }
    template<typename S >
    Vector operator+=(S s)
    {
      Real scalar = static_cast<Real>(s);
      data[0] += scalar;
      data[1] += scalar;
      data[2] += scalar;
      return *this;
    }

    // Unary operator
    Vector operator-() const {
      Vector result;
      result.data[0] = -data[0];
      result.data[1] = -data[1];
      result.data[2] = -data[2];
      return result;
    }

    // - operator
    Vector operator-(const Vector& v) const {
      Vector result;
      result.data[0] = data[0] - v.data[0];
      result.data[1] = data[1] - v.data[1];
      result.data[2] = data[2] - v.data[2];
      return result;
    }
    Vector& operator-=(const Vector& v) {
      data[0] -= v.data[0];
      data[1] -= v.data[1];
      data[2] -= v.data[2];
      return *this;
    }
    Vector operator-(Real s) const {
      Vector result;
      result.data[0] = data[0] - s;
      result.data[1] = data[1] - s;
      result.data[2] = data[2] - s;
      return result;
    }
    Vector& operator-=(Real s) {
      data[0] -= s;
      data[1] -= s;
      data[2] -= s;
      return *this;
    }
    template<typename S >
    Vector operator-(S s) const
    {
      Real scalar = static_cast<Real>(s);
      Vector result(data[0] - scalar,
                    data[1] - scalar,
                    data[2] - scalar);
      return result;
    }
    template<typename S >
    Vector operator-=(S s)
    {
      Real scalar = static_cast<Real>(s);
      data[0] -= scalar;
      data[1] -= scalar;
      data[2] -= scalar;
      return *this;
    }

    // * operator
    Vector operator*(const Vector& v) const {
      Vector result;
      result.data[0] = data[0] * v.data[0];
      result.data[1] = data[1] * v.data[1];
      result.data[2] = data[2] * v.data[2];
      return result;
    }
    Vector& operator*=(const Vector& v) {
      data[0] *= v.data[0];
      data[1] *= v.data[1];
      data[2] *= v.data[2];
      return *this;
    }
    Vector operator*(Real s) const {
      Vector result;
      result.data[0] = data[0] * s;
      result.data[1] = data[1] * s;
      result.data[2] = data[2] * s;
      return result;
    }
    Vector& operator*=(Real s) {
      data[0] *= s;
      data[1] *= s;
      data[2] *= s;
      return *this;
    }
    template<typename S >
    Vector operator*(S s) const
    {
      Real scalar = static_cast<Real>(s);
      Vector result(data[0] * scalar,
                    data[1] * scalar,
                    data[2] * scalar);
      return result;
    }
    template<typename S >
    Vector operator*=(S s)
    {
      Real scalar = static_cast<Real>(s);
      data[0] *= scalar;
      data[1] *= scalar;
      data[2] *= scalar;
      return *this;
    }

    // / operator
    Vector operator/(const Vector& v) const {
      Vector result;
      result.data[0] = data[0] / v.data[0];
      result.data[1] = data[1] / v.data[1];
      result.data[2] = data[2] / v.data[2];
      return result;
    }
    Vector operator/(Real s) const {
      Real inv_s = 1/s;
      Vector result;
      result.data[0] = data[0] * inv_s;
      result.data[1] = data[1] * inv_s;
      result.data[2] = data[2] * inv_s;
      return result;
    }
    Vector& operator/=(Real s) {
      Real inv_s = 1/s;
      data[0] *= inv_s;
      data[1] *= inv_s;
      data[2] *= inv_s;
      return *this;
    }

    bool operator==(const Vector& right) const {
      return (data[0] == right.data[0] &&
              data[1] == right.data[1] &&
              data[2] == right.data[2]);
    }

    bool operator!=(const Vector& right) const {
      return (data[0] != right.data[0] ||
              data[1] != right.data[1] ||
              data[2] != right.data[2]);
    }

    Real length() const {
      Real sum = data[0]*data[0];
      sum += data[1]*data[1];
      sum += data[2]*data[2];
      return Sqrt(sum);
    }
    Real length2() const {
      Real sum = data[0]*data[0];
      sum += data[1]*data[1];
      sum += data[2]*data[2];
      return sum;
    }

    // GCC and perhaps other compilers have a hard time optimizing
    // with additional levels of function calls (i.e. *this *= scale).
    // Because of this, we try to minimize the number of function
    // calls we make and do explicit inlining.
    Real normalize() {
      Real l = Sqrt(data[0]*data[0] +
                    data[1]*data[1] +
                    data[2]*data[2]);
      Real scale = 1/l;
      data[0] *= scale;
      data[1] *= scale;
      data[2] *= scale;
      return l;
    }
    Vector normal() const {
      Real l = Sqrt(data[0]*data[0] +
                    data[1]*data[1] +
                    data[2]*data[2]);
      Real scale = 1/l;
      Vector result;
      result.data[0] = data[0] * scale;
      result.data[1] = data[1] * scale;
      result.data[2] = data[2] * scale;
      return result;
    }
    Real minComponent() const {
      if (data[0] < data[1]) {
        if (data[0] < data[2])
          return data[0];
        else
          return data[2];
      } else {
        if (data[1] < data[2])
          return data[1];
        else
          return data[2];
      }
    }
    Real maxComponent() const {
      if (data[0] > data[1]) {
        if (data[0] > data[2])
          return data[0];
        else
          return data[2];
      } else {
        if (data[1] > data[2])
          return data[1];
        else
          return data[2];
      }
    }
    int indexOfMinComponent() const {
      if (data[0] < data[1]) {
        if (data[0] < data[2])
          return 0;
        else
          return 2;
      } else {
        if (data[1] < data[2])
          return 1;
        else
          return 2;
      }
    }
    int indexOfMaxComponent() const {
      if (data[0] > data[1]) {
        if (data[0] > data[2])
          return 0;
        else
          return 2;
      } else {
        if (data[1] > data[2])
          return 1;
        else
          return 2;
      }
    }

    Vector inverse() const {
      Vector result;
      result.data[0] = 1/data[0];
      result.data[1] = 1/data[1];
      result.data[2] = 1/data[2];
      return result;
    }

    Vector absoluteValue() const {
      Vector result;
      result.data[0] = Abs(data[0]);
      result.data[1] = Abs(data[1]);
      result.data[2] = Abs(data[2]);
      return result;
    }

    static Vector zero() {
      return Vector(0,0,0);
    }
    static Vector one() {
      return Vector(1,1,1);
    }

    Vector findPerpendicular() const {
      Vector result;
#if 0
      float a0 = Abs(data[0]);
      float a1 = Abs(data[1]);
      float a2 = Abs(data[2]);
      if(a0 < a1){
        if(a0 < a2){
          // 0 smallest
          result.data[0] = 0;
          result.data[1] = -data[2];
          result.data[2] = data[1];
        } else {
          // 2 smallest
          result.data[0] = -data[1];
          result.data[1] = data[0];
          result.data[2] = 0;
        }
      } else {
        if(a1 < a2){
          // 1 smallest
          result.data[0] = data[2];
          result.data[1] = 0;
          result.data[2] = -data[0];
        } else {
          // 2 smallest
          result.data[0] = -data[1];
          result.data[1] = data[0];
          result.data[2] = 0;
        }
      }
#else
      ComponentType d_2sqr = data[2] * data[2];
      ComponentType d_1sqr = data[1] * data[1];
      if ( d_2sqr + d_1sqr < ComponentType(1e-4)) {
         // use d_0
         result.data[0] = -data[2];
         result.data[1] = 0;
         result.data[2] = data[0];
      } else {
         result.data[0] = 0;
         result.data[1] = data[2];
         result.data[2] = -data[1];
      }
#endif
      return result;
    }

#ifndef SWIG // Swig balks about these functions, but when I rename
             // them it doesn't do anything.
    // Friend functions.  These will allow the functions to access
    // data without having to use a function (hopefully).
    template<typename S >
    friend Vector operator*(S s, const Vector& v);
//     template<typename S >
//     friend Vector operator*(const Vector& v, S s);
    friend Real   Dot(const Vector& v1, const Vector& v2);
    friend Vector Cross(const Vector& v1, const Vector& v2);
    friend Vector Min(const Vector& v1, const Vector& v2);
    friend Vector Max(const Vector& v1, const Vector& v2);
    friend Vector Interpolate(const Vector& v1, const Vector& v2, Real weight);
    friend Vector Interpolate(const Vector& v1, const Vector& v2, const Vector& weight);
#endif

    void readwrite(ArchiveElement*);
    //private:
    Real data[3];

    // Do not use this function!!!!  Use getDataPtr() instead.
    //      const Real *operator &() const { return data; }
  };


  ///////////////////////////////////////////////////////
  // Outside of the class (ie. Manta namespace)

  // Scalar * Vector
  template<typename S >
  inline Vector operator*(S s, const Vector& v)
  {
    Real scalar = static_cast<Real>(s);
    Vector result(scalar * v.data[0],
                  scalar * v.data[1],
                  scalar * v.data[2]);
    return result;
  }

  template<typename S >
  inline Vector operator+(S s, const Vector& v)
  {
    Real scalar = static_cast<Real>(s);
    Vector result(scalar + v.data[0],
                  scalar + v.data[1],
                  scalar + v.data[2]);
    return result;
  }

  template<typename S >
  inline Vector operator-(S s, const Vector& v)
  {
    Real scalar = static_cast<Real>(s);
    Vector result(scalar - v.data[0],
                  scalar - v.data[1],
                  scalar - v.data[2]);
    return result;
  }

  inline Real Dot(const Vector& v1, const Vector& v2)
  {
    return (v1.data[0] * v2.data[0] +
            v1.data[1] * v2.data[1] +
            v1.data[2] * v2.data[2]);
  }

  // Cross product is only defined for R3
  inline Vector Cross(const Vector& v1, const Vector& v2)
  {
    return Vector(v1.data[1]*v2.data[2] - v1.data[2]*v2.data[1],
                  v1.data[2]*v2.data[0] - v1.data[0]*v2.data[2],
                  v1.data[0]*v2.data[1] - v1.data[1]*v2.data[0]);
  }

  inline Vector Min(const Vector& v1, const Vector& v2)
  {
    Vector result(Min(v1.data[0], v2.data[0]),
                  Min(v1.data[1], v2.data[1]),
                  Min(v1.data[2], v2.data[2]));
    return result;
  }

  inline Vector Max(const Vector& v1, const Vector& v2)
  {
    Vector result(Max(v1.data[0], v2.data[0]),
                  Max(v1.data[1], v2.data[1]),
                  Max(v1.data[2], v2.data[2]));
    return result;
  }

  inline Vector Interpolate(const Vector& v1, const Vector& v2, Real weight)
  {
    Vector result(Interpolate(v1.data[0], v2.data[0], weight),
                  Interpolate(v1.data[1], v2.data[1], weight),
                  Interpolate(v1.data[2], v2.data[2], weight));
    return result;
  }

  inline Vector Interpolate(const Vector& v1, const Vector& v2, const Vector& weight)
  {
    Vector result(Interpolate(v1.data[0], v2.data[0], weight[0]),
                  Interpolate(v1.data[1], v2.data[1], weight[1]),
                  Interpolate(v1.data[2], v2.data[2], weight[2]));
    return result;
  }

  inline Manta::Vector Sqrt(const Manta::Vector& v1)
  {
    return Manta::Vector(Sqrt(v1.data[0]), Sqrt(v1.data[1]), Sqrt(v1.data[2]));
  }

  inline Vector projectToSphere(Real x, Real y, Real radius)
  {
    x /= radius;
    y /= radius;
    Real rad2 = x*x+y*y;
    if(rad2 > 1){
      Real rad = Sqrt(rad2);
      x /= rad;
      y /= rad;
      return Vector(x,y,0);
    } else {
      Real z = Sqrt(1-rad2);
      return Vector(x,y,z);
    }
  }

  std::ostream& operator<< (std::ostream& os, const Vector& v);
  std::istream& operator>> (std::istream& is,       Vector& v);

  MANTA_DECLARE_RTTI_BASECLASS(Vector, ConcreteClass, readwriteMethod_lightweight);
}

#endif
