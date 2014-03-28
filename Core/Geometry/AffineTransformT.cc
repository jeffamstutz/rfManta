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
#include <Core/Geometry/AffineTransformT.h>
#include <Core/Math/Trig.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/MiscMath.h>
#include <iostream>

// TODO: several of these could be faster if they become bottlenecks

namespace Manta {
  template<typename T>
  void AffineTransformT<T>::initWithIdentity()
  {
    for(int row = 0; row < 3; row++){
      for(int col = 0; col < 4; col++)
        mat[row][col] = 0;
      mat[row][row] = 1;
    }
  }

  template<typename T>
  void AffineTransformT<T>::initWithScale(const VectorT<T, 3>& s)
  {
    initWithIdentity();
    scale(s);
  }

  template<typename T>
  void AffineTransformT<T>::initWithRotation(const VectorT<T, 3>& from,
                                             const VectorT<T, 3>& to)
  {
    initWithIdentity();
    rotate(from, to);
  }

  template<typename T>
  void AffineTransformT<T>::initWithRotation(const VectorT<T, 3>& axis,
                                             T angleInRadians)
  {
    initWithIdentity();
    rotate(axis, angleInRadians);
  }

  template<typename T>
  void AffineTransformT<T>::initWithTranslation(const VectorT<T, 3>& translation)
  {
    initWithIdentity();
    translate(translation);
  }

  /**
   * Each vector becomes a column in the resulting matrix, with the
   * point in the last column
   */
  template<typename T>
  void AffineTransformT<T>::initWithBasis(const VectorT<T, 3>& v1,
                                          const VectorT<T, 3>& v2,
                                          const VectorT<T, 3>& v3,
                                          const VectorT<T, 3>& p)
  {
    for(int i=0;i<3;i++)
      mat[i][0] = v1[i];
    for(int i=0;i<3;i++)
      mat[i][1] = v2[i];
    for(int i=0;i<3;i++)
      mat[i][2] = v3[i];
    for(int i=0;i<3;i++)
      mat[i][3] = p[i];
  }

  template<typename T>
  void AffineTransformT<T>::scale(const VectorT<T, 3>& scale)
  {
    for(int row = 0; row < 3; row++){
      T s = scale[row];
      for(int col = 0; col <= 3; col++)
        mat[row][col] *= s;
    }
  }

  template<typename T>
  void AffineTransformT<T>::rotate(const VectorT<T, 3>& from_,
                                   const VectorT<T, 3>& to_)
  {
    // Modified from Moller/Hughes, Journal of Graphics Tools Vol. 4, No. 4
    // "Efficiently Building a Matrix to Rotate One Vector to Another"

    // Don't assume that to and from are normalized.
    VectorT<T,3> from = from_.normal();
    VectorT<T,3> to   = to_.normal();

    VectorT<T, 3> v = Cross(from, to);
    T e = Dot(from, to);
    if(e > 1-(T)(1.e-9)){
      // No rotation
      return;
    } else if (e < -1+(T)(1.e-9)){
      // 180 degree rotation
      VectorT<T, 3> x = v.absoluteValue();
      int largest = x.indexOfMaxComponent();
      x = VectorT<T, 3>(0, 0, 0);
      x[largest] = 1;

      VectorT<T, 3> u = x-from;
      VectorT<T, 3> v = x-to;
      T c1 = 2/Dot(u, u);
      T c2 = 2/Dot(v, v);
      T c3 = c1 * c2 * Dot(u, v);
      T mtx[3][3];
      for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
          mtx[i][j] = -c1 * u[i] * u[j] + c2 * v[i] * v[j] + c3 * v[i] * u[j];
        }
        mtx[i][i] += 1;
      }
      pre_multiply(mtx);
    } else {
      T h = (T)1/(1 + e);      /* optimization by Gottfried Chen */
      T mtx[3][3];
      mtx[0][0] = e + h * v[0] * v[0];
      mtx[0][1] = h * v[0] * v[1] - v[2];
      mtx[0][2] = h * v[0] * v[2] + v[1];

      mtx[1][0] = h * v[0] * v[1] + v[2];
      mtx[1][1] = e + h * v[1] * v[1];
      mtx[1][2] = h * v[1] * v[2] - v[0];

      mtx[2][0] = h * v[0] * v[2] - v[1];
      mtx[2][1] = h * v[1] * v[2] + v[0];
      mtx[2][2] = e + h * v[2] * v[2];
      pre_multiply(mtx);
    }
  }

  template<typename T>
  void AffineTransformT<T>::rotate(const VectorT<T, 3>& axis,
                                   T angleInRadians)
  {
    T rot[3][3];
    // From Foley and Van Dam, Pg 227
    // NOTE: Element 0,1 is wrong in the text!
    T sintheta=Sin(angleInRadians);
    T costheta=Cos(angleInRadians);
    T ux=axis.x();
    T uy=axis.y();
    T uz=axis.z();
    rot[0][0]=ux*ux+costheta*(1-ux*ux);
    rot[0][1]=ux*uy*(1-costheta)-uz*sintheta;
    rot[0][2]=uz*ux*(1-costheta)+uy*sintheta;

    rot[1][0]=ux*uy*(1-costheta)+uz*sintheta;
    rot[1][1]=uy*uy+costheta*(1-uy*uy);
    rot[1][2]=uy*uz*(1-costheta)-ux*sintheta;

    rot[2][0]=uz*ux*(1-costheta)-uy*sintheta;
    rot[2][1]=uy*uz*(1-costheta)+ux*sintheta;
    rot[2][2]=uz*uz+costheta*(1-uz*uz);

    pre_multiply(rot);
  }

  template<typename T>
  void AffineTransformT<T>::translate(const VectorT<T, 3>& translation)
  {
    for(int i=0;i<3;i++)
      mat[i][3] += translation[i];
  }

  template<typename T>
  static AffineTransformT<T> createScale(const VectorT<T, 3>& scale)
  {
    AffineTransformT<T> result;
    result.initWithScale(scale);
    return result;
  }

  template<typename T>
  static AffineTransformT<T> createRotation(const VectorT<T, 3>& from,
                                            const VectorT<T, 3>& to)
  {
    AffineTransformT<T> result;
    result.initWithRotation(from, to);
    return result;
  }

  template<typename T>
  static AffineTransformT<T> createRotation(const VectorT<T, 3>& axis,
                                            T angleInRadians)
  {
    AffineTransformT<T> result;
    result.initWithRotation(axis, angleInRadians);
    return result;
  }

  template<typename T>
  static AffineTransformT<T> createTranslation(const VectorT<T, 3>& translation)
  {
    AffineTransformT<T> result;
    result.initWithTranslation(translation);
    return result;
  }

  template<typename T>
  AffineTransformT<T> AffineTransformT<T>::inverse() const
  {
    AffineTransformT<T> result(*this);
    result.invert();
    return result;
  }

  template<typename T>
  void AffineTransformT<T>::invert()
  {

    T a = mat[0][0];
    T b = mat[0][1];
    T c = mat[0][2];
    T d = mat[1][0];
    T e = mat[1][1];
    T f = mat[1][2];
    T g = mat[2][0];
    T h = mat[2][1];
    T i = mat[2][2];

    T denom = c*d*h-c*e*g-b*d*i+b*f*g+a*e*i-a*f*h;

    T inv_denom = (T)(1)/denom;
    mat[0][0] = (e*i-f*h)*inv_denom;
    mat[0][1] = (c*h-b*i)*inv_denom;
    mat[0][2] = (b*f-c*e)*inv_denom;
    mat[1][0] = (f*g-d*i)*inv_denom;
    mat[1][1] = (a*i-c*g)*inv_denom;
    mat[1][2] = (c*d-a*f)*inv_denom;
    mat[2][0] = (d*h-e*g)*inv_denom;
    mat[2][1] = (b*g-a*h)*inv_denom;
    mat[2][2] = (a*e-b*d)*inv_denom;
    // These were double, but I changed them to type T.  Do they need
    // to be double for some reason?
    T x = mat[0][3];
    T y = mat[1][3];
    T z = mat[2][3];
    mat[0][3] = -(mat[0][0]*x + mat[0][1]*y + mat[0][2]*z);
    mat[1][3] = -(mat[1][0]*x + mat[1][1]*y + mat[1][2]*z);
    mat[2][3] = -(mat[2][0]*x + mat[2][1]*y + mat[2][2]*z);
  }

  template<typename T>
  AffineTransformT<T> AffineTransformT<T>::operator*(const AffineTransformT<T>& m) const
  {
    /*
     * Multiplication of two matrices.  They look like:
     * A: this, B: m, C: result
     * [A00 A01 A02 A03]  [B00 B01 B02 B03]   [C00 C01 C02 C03]
     * [A10 A11 A12 A13]  [B10 B11 B12 B13] = [C10 C11 C12 C13]
     * [A20 A21 A22 A23]  [B20 B21 B22 B23]   [C20 C21 C22 C23]
     * [  0   0   0   1]  [  0   0   0   1]   [  0   0   0   1]
     */
    AffineTransformT<T> result;
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
        T sum = 0;
        for(int k=0;k<3;k++){
          sum += mat[i][k] * m.mat[k][j];
        }
        result.mat[i][j] = sum;
      }
      T sum = mat[i][3];
      for(int k=0;k<3;k++)
        sum += mat[i][k] * m.mat[k][3];
      result.mat[i][3] = sum;
    }
    return result;
  }

  template<typename T>
  AffineTransformT<T>& AffineTransformT<T>::operator*=(const AffineTransformT<T>& m)
  {
    T tmp[3][4];

    for(int row = 0; row < 3; row++){
      for(int col = 0; col < 4; col++){
        tmp[row][col] = mat[row][col];
      }
    }
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
        T sum = 0;
        for(int k=0;k<3;k++){
          sum += tmp[i][k] * m.mat[k][j];
        }
        mat[i][j] = sum;
      }
      T sum = mat[i][3];
      for(int k=0;k<3;k++)
        sum += tmp[i][k] * m.mat[k][3];
      mat[i][3] = sum;
    }
    return *this;
  }

  // Private helper function.
  template<typename T>
  void AffineTransformT<T>::pre_multiply( T pre[3][3] ) {
    T tmp[3][4];

    // Copy pre into tmp.
    for(int row = 0; row < 3; row++){
      for(int col = 0; col < 4; col++){
        tmp[row][col] = mat[row][col];
      }
    }

    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
        T sum = 0;
        for(int k=0;k<3;k++){
          sum += pre[i][k] * tmp[k][j];
        }
        mat[i][j] = sum;
      }
      T sum = 0;
      for(int k=0;k<3;k++)
        sum += pre[i][k] * tmp[k][3];
      mat[i][3] = sum;
    }
  }

  template<typename T>
  VectorT<T, 3>
  AffineTransformT<T>::multiply_point(const VectorT<T, 3>& point) const {
    VectorT<T, 3> result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[i][0] * point[0];
      result[i] += mat[i][1] * point[1];
      result[i] += mat[i][2] * point[2];
      result[i] += mat[i][3]; // point[3] == 1
    }
    return result;
  }

  template<typename T>
  VectorT<T, 3>
  AffineTransformT<T>::multiply_vector(const VectorT<T, 3>& vec) const {
    VectorT<T, 3> result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[i][0] * vec[0];
      result[i] += mat[i][1] * vec[1];
      result[i] += mat[i][2] * vec[2];
      // vec[3] == 0
    }
    return result;
  }

  template<typename T>
  VectorT<T, 3>
  AffineTransformT<T>::transpose_mult_point(const VectorT<T, 3>& point) const {
    VectorT<T, 3> result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[0][i] * point[0];
      result[i] += mat[1][i] * point[1];
      result[i] += mat[2][i] * point[2];
      //mat[3] (bottom row) is (0 0 0 1)
    }
    return result;
  }

  template<typename T>
  VectorT<T, 3>
  AffineTransformT<T>::transpose_mult_vector(const VectorT<T, 3>& vec) const {
    VectorT<T, 3> result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[0][i] * vec[0];
      result[i] += mat[1][i] * vec[1];
      result[i] += mat[2][i] * vec[2];
      // vec[3] == 0
    }
    return result;
  }
  
  template<typename T>
  std::ostream& operator<<(std::ostream& os,
                           const AffineTransformT<T>& trans )
  {
    for (int i=0;i<3;++i) {
      for (int j=0;j<4;++j) {
        os << trans(i,j) << " ";
      }
      os << std::endl;
    }
    return os;
  }


  // Static Instances.
  template class AffineTransformT<double>;
  template std::ostream& operator<<(std::ostream& os, const AffineTransformT<double>& trans);

  template class AffineTransformT<float>;
  template std::ostream& operator<<(std::ostream& os, const AffineTransformT<float>& trans);

}
