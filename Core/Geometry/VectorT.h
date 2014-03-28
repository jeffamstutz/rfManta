
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

  VectorT is made to be templated with floating point types.  If you
  use an integral type, you could get into trouble with certain
  functions that compute division.  Be warned!

*/

#ifndef Manta_Core_VectorT_h
#define Manta_Core_VectorT_h

#include <Core/Geometry/Vector.h>
#include <Core/Math/Expon.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/MiscMath.h>
#include <iosfwd>

namespace Manta {
  template<typename T, int Dim>
  class VectorT {
  public:
    typedef T ComponentType;

    VectorT() {
    }
#ifndef SWIG
    VectorT(T x) {
      data[0] = x;
    }
    VectorT(T x, T y) {
      typedef char unnamed[ Dim == 2 ? 1 : 0 ];
      data[0] = x; data[1] = y;
    }
    VectorT(T x, T y, T z) {
      typedef char unnamed[ Dim == 3 ? 1 : 0 ];
      data[0] = x; data[1] = y; data[2] = z;
    }
    VectorT(T x, T y, T z, T w) {
      typedef char unnamed[ Dim == 4 ? 1 : 0 ];
      data[0] = x; data[1] = y; data[2] = z; data[3] = w;
    }
#endif

    VectorT(const Vector& copy) {
      typedef char unnamed[ Dim == 3 ? 1 : 0 ];
      data[0] = copy[0];
      data[1] = copy[1];
      data[2] = copy[2];
    }

    // Copy from another vector of the same size.
    template< typename S >
    VectorT(const VectorT<S, Dim>& copy) {
      for(int i=0;i<Dim;i++)
        data[i] = static_cast<T>(copy[i]);
    }

    // Copy from same type and size.
    VectorT(const VectorT<T, Dim>& copy) {
      for(int i=0;i<Dim;i++)
        data[i] = copy[i];
    }

    VectorT(const T* data_in) {
      for(int i=0;i<Dim;i++)
        data[i] = data_in[i];
    }

#ifndef SWIG
    VectorT<T, Dim>& operator=(const VectorT<T, Dim>& copy) {
      for(int i=0;i<Dim;i++)
        data[i] = (T)copy[i];
      return *this;
    }
#endif

    ~VectorT() {
    }

    T x() const {
      return data[0];
    }
    T y() const {
      typedef char unnamed[ Dim >=2 ? 1 : 0 ];
      return data[1];
    }
    T z() const {
      typedef char unnamed[ Dim >= 3 ? 1 : 0 ];
      return data[2];
    }
#ifndef SWIG
    const T& operator[](int i) const {
      return data[i];
    }
    T& operator[] ( int i ) {
      return data[i];
    }

#else
    %extend {
      T& __getitem__( int i ) {
        return self->operator[](i);
      }
    }
#endif
    // One might be tempted to add an "operator &" function, but
    // that could lead to problems when you want an address to the
    // object rather than a pointer to data.
    const T* getDataPtr() const { return data; }

    // + operator
    VectorT<T, Dim> operator+(const VectorT<T, Dim>& v) const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] + v.data[i];
      return result;
    }
    VectorT<T, Dim>& operator+=(const VectorT<T, Dim>& v) {
      for(int i=0;i<Dim;i++)
        data[i] += v.data[i];
      return *this;
    }
    VectorT<T, Dim> operator+(T s) const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] + s;
      return result;
    }
    VectorT<T, Dim>& operator+=(T s) {
      for(int i=0;i<Dim;i++)
        data[i] += s;
      return *this;
    }
    template<typename S >
    VectorT<T, Dim> operator+(S s) const
    {
      Real scalar = static_cast<T>(s);
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] + scalar;
      return result;
    }
    template<typename S >
    VectorT<T, Dim>& operator+=(S s)
    {
      Real scalar = static_cast<T>(s);
      for(int i=0;i<Dim;i++)
        data[i] += scalar;
      return *this;
    }

    // Unary operator
    VectorT<T, Dim> operator-() const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = -data[i];
      return result;
    }

    // - operator
    VectorT<T, Dim> operator-(const VectorT<T, Dim>& v) const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] - v.data[i];
      return result;
    }
    VectorT<T, Dim>& operator-=(const VectorT<T, Dim>& v) {
      for(int i=0;i<Dim;i++)
        data[i] -= v.data[i];
      return *this;
    }
    VectorT<T, Dim> operator-(T s) const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] - s;
      return result;
    }
    VectorT<T, Dim> operator-=(T s) {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        data[i] -= s;
      return *this;
    }
    template<typename S >
    VectorT<T, Dim> operator-(S s) const
    {
      Real scalar = static_cast<T>(s);
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] - scalar;
      return result;
    }
    template<typename S >
    VectorT<T, Dim>& operator-=(S s)
    {
      Real scalar = static_cast<T>(s);
      for(int i=0;i<Dim;i++)
        data[i] -= scalar;
      return *this;
    }

    // * operator
    VectorT<T, Dim> operator*(const VectorT<T, Dim>& v) const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] * v.data[i];
      return result;
    }
    VectorT<T, Dim>& operator*=(const VectorT<T, Dim>& v) {
      for(int i=0;i<Dim;i++)
        data[i] *= v.data[i];
      return *this;
    }
    VectorT<T, Dim> operator*(T s) const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] * s;
      return result;
    }
    VectorT<T, Dim>& operator*=(T s) {
      for(int i=0;i<Dim;i++)
        data[i] *= s;
      return *this;
    }
    template<typename S >
    VectorT<T, Dim> operator*(S s) const
    {
      Real scalar = static_cast<T>(s);
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] * scalar;
      return result;
    }
    template<typename S >
    VectorT<T, Dim>& operator*=(S s)
    {
      Real scalar = static_cast<T>(s);
      for(int i=0;i<Dim;i++)
        data[i] *= scalar;
      return *this;
    }

    // / operator
    VectorT<T, Dim> operator/(const VectorT<T, Dim>& v) const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] / v.data[i];
      return result;
    }
    VectorT<T, Dim> operator/(T s) const {
      T inv_s = 1/s;
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] * inv_s;
      return result;
    }
    VectorT<T, Dim>& operator/=(T s) {
      T inv_s = 1/s;
      for(int i=0;i<Dim;i++)
        data[i] *= inv_s;
      return *this;
    }

    bool operator==(const VectorT<T, Dim>& right) const {
      for (int i = 0; i < Dim; ++i)
        if(data[i] != right.data[i])
          return false;
      return true;
    }

    bool operator!=(const VectorT<T, Dim>& right) const {
      for (int i = 0; i < Dim; ++i)
        if(data[i] != right.data[i])
          return true;
      return false;
    }

    T length() const {
      T sum = 0;
      for(int i=0;i<Dim;i++)
        sum += data[i]*data[i];
      return Sqrt(sum);
    }
    T length2() const {
      T sum = 0;
      for(int i=0;i<Dim;i++)
        sum += data[i]*data[i];
      return sum;
    }

    // GCC and perhaps other compilers have a hard time optimizing
    // with additional levels of function calls (i.e. *this *= scale).
    // Because of this, we try to minimize the number of function
    // calls we make and do explicit inlining.
    T normalize() {
      T sum = 0;
      for(int i=0;i<Dim;i++)
        sum += data[i]*data[i];
      T l = Sqrt(sum);
      T scale = 1/l;
      for(int i=0;i<Dim;i++)
        data[i] *= scale;
      return l;
    }
    VectorT<T, Dim> normal() const {
      T sum = 0;
      for(int i=0;i<Dim;i++)
        sum += data[i]*data[i];
      T l = Sqrt(sum);
      T scale = 1/l;
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = data[i] * scale;
      return result;
    }
    T minComponent() const {
      T min = data[0];
      for(int i=1;i<Dim;i++)
        if(data[i] < min)
          min = data[i];
      return min;
    }
    T maxComponent() const {
      T max = data[0];
      for(int i=1;i<Dim;i++)
        if(data[i] > max)
          max = data[i];
      return max;
    }
    int indexOfMinComponent() const {
      int idx = 0;
      for(int i=1;i<Dim;i++)
        if(data[i] < data[idx])
          idx = i;
      return idx;
    }
    int indexOfMaxComponent() const {
      int idx = 0;
      for(int i=1;i<Dim;i++)
        if(data[i] > data[idx])
          idx = i;
      return idx;
    }

    VectorT<T, Dim> inverse() const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = 1/data[i];
      return result;
    }
    VectorT<T, Dim> absoluteValue() const {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = Abs(data[i]);
      return result;
    }

    static VectorT<T, Dim> zero() {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = 0;
      return result;
    }
    static VectorT<T, Dim> one() {
      VectorT<T, Dim> result;
      for(int i=0;i<Dim;i++)
        result.data[i] = 1;
      return result;
    }

#if 0
    //un-privatized for speed.
  private:
#endif
    T data[Dim];

    // Do not use this function!!!!  Use getDataPtr() instead.
    //      const T *operator &() const { return data; }
  };


  // Two multiplication by a scalar operators.
  template<typename T, int Dim, typename S >
  inline VectorT<T, Dim> operator*(S s, const VectorT<T, Dim>& v)
  {
    T scalar = static_cast<T>(s);
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = scalar * v[i];
    return result;
  }

  template<typename T, int Dim, typename S >
  inline VectorT<T, Dim> operator+(S s, const VectorT<T, Dim>& v)
  {
    T scalar = static_cast<T>(s);
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = scalar + v[i];
    return result;
  }

  template<typename T, int Dim, typename S >
  inline VectorT<T, Dim> operator-(S s, const VectorT<T, Dim>& v)
  {
    T scalar = static_cast<T>(s);
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = scalar - v[i];
    return result;
  }

  template<typename T, int Dim>
  inline T Dot(const VectorT<T, Dim>& v1, const VectorT<T, Dim>& v2)
  {
    T result = 0;
    for(int i=0;i<Dim;i++)
      result += v1[i] * v2[i];
    return result;
  }

  // Cross product is only defined for R3
  template<typename T>
  inline VectorT<T, 3> Cross(const VectorT<T, 3>& v1, const VectorT<T, 3>& v2)
  {
    return VectorT<T, 3>(v1[1]*v2[2] - v1[2]*v2[1],
                         v1[2]*v2[0] - v1[0]*v2[2],
                         v1[0]*v2[1] - v1[1]*v2[0]);
  }

  template<typename T, int Dim>
  inline VectorT<T, Dim> Min(const VectorT<T, Dim>& v1,
                             const VectorT<T, Dim>& v2)
  {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = Min(v1[i], v2[i]);
    return result;
  }

  template<typename T, int Dim>
  inline VectorT<T, Dim> Max(const VectorT<T, Dim>& v1,
                             const VectorT<T, Dim>& v2)
  {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = Max(v1[i], v2[i]);
    return result;
  }

  template<typename T, int Dim>
  inline VectorT<T, Dim> Interpolate(const VectorT<T, Dim>& p1,
                                     const VectorT<T, Dim>& p2,
                                     T weight)
  {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = Interpolate(p1[i], p2[i], weight);
    return result;
  }

  template<typename T, int Dim>
  inline VectorT<T, Dim> Interpolate(const VectorT<T, Dim>& p1,
                                     const VectorT<T, Dim>& p2,
                                     const VectorT<T, Dim>& weight)
  {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = Interpolate(p1[i], p2[i], weight[i]);
    return result;
  }

  template<typename T, int Dim>
  std::ostream& operator<< (std::ostream& os, const VectorT<T,Dim>& v);

  template<typename T, int Dim>
  std::istream& operator>> (std::istream& is, VectorT<T,Dim>& v);

  template<typename T, int Dim>
    inline Manta::VectorT<T, Dim> Sqrt(const Manta::VectorT<T, Dim>& v1)
  {
    Manta::VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result[i] = Sqrt(v1[i]);
    return result;
  }
}

#endif
