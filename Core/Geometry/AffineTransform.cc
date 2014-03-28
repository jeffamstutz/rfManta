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
#include <Core/Geometry/AffineTransform.h>
#include <Core/Math/Trig.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/MiscMath.h>
#include <iostream>

// TODO: several of these could be faster if they become bottlenecks

namespace Manta {
  void AffineTransform::initWithIdentity()
  {
    for(int row = 0; row < 3; row++){
      for(int col = 0; col < 4; col++)
        mat[row][col] = 0;
      mat[row][row] = 1;
    }
  }

  bool AffineTransform::isIdentity() const
  {
    for(int row = 0; row < 3; row++){
      if(mat[row][row] != 1)
        return false;
      for(int col = 0; col < 4; col++){
        if(row != col && mat[row][col] != 0)
          return false;
      }
    }
    return true;
  }

  void AffineTransform::initWithScale(const Vector& s)
  {
    initWithIdentity();
    scale(s);
  }

  void AffineTransform::initWithRotation(const Vector& from,
                                         const Vector& to)
  {
    initWithIdentity();
    rotate(from, to);
  }

  void AffineTransform::initWithRotation(const Vector& axis,
                                         Real angleInRadians)
  {
    initWithIdentity();
    rotate(axis, angleInRadians);
  }

  void AffineTransform::initWithTranslation(const Vector& translation)
  {
    initWithIdentity();
    translate(translation);
  }

  /**
   * Each vector becomes a column in the resulting matrix, with the
   * point in the last column
   */
  void AffineTransform::initWithBasis(const Vector& v1,
                                      const Vector& v2,
                                      const Vector& v3,
                                      const Vector& p)
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

  void AffineTransform::scale(const Vector& scale)
  {
    for(int row = 0; row < 3; row++){
      Real s = scale[row];
      for(int col = 0; col <= 3; col++)
        mat[row][col] *= s;
    }
  }

  void AffineTransform::rotate(const Vector& from_,
                               const Vector& to_)
  {
    // Modified from Moller/Hughes, Journal of Graphics Tools Vol. 4, No. 4
    // "Efficiently Building a Matrix to Rotate One Vector to Another"

    // Don't assume that to and from are normalized.
    Vector from = from_.normal();
    Vector to   = to_.normal();

    Vector v = Cross(from, to);
    Real e = Dot(from, to);
    if(e >= 1-(Real)(1.e-7)){
      // No rotation
      return;
    } else if (e <= -1+(Real)(1.e-7)){
      // 180 degree rotation
      Vector x = v.absoluteValue();
      int largest = x.indexOfMaxComponent();
      x = Vector(0, 0, 0);
      x[largest] = 1;

      Vector u = x-from;
      Vector v = x-to;
      Real c1 = 2/Dot(u, u);
      Real c2 = 2/Dot(v, v);
      Real c3 = c1 * c2 * Dot(u, v);
      Real mtx[3][3];
      for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
          mtx[i][j] = -c1 * u[i] * u[j] + c2 * v[i] * v[j] + c3 * v[i] * u[j];
        }
        mtx[i][i] += 1;
      }
      pre_multiply(mtx);
    } else {
      Real h = (Real)1/(1 + e);      /* optimization by Gottfried Chen */
      Real mtx[3][3];
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

  void AffineTransform::rotate(const Vector& axis,
                               Real angleInRadians)
  {
    Real rot[3][3];
    // From Foley and Van Dam, Pg 227
    // NOTE: Element 0,1 is wrong in the text!
    Real sintheta=Sin(angleInRadians);
    Real costheta=Cos(angleInRadians);
    Real ux=axis.x();
    Real uy=axis.y();
    Real uz=axis.z();
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

  void AffineTransform::translate(const Vector& translation)
  {
    for(int i=0;i<3;i++)
      mat[i][3] += translation[i];
  }

  AffineTransform AffineTransform::createIdentity()
  {
    AffineTransform result;
    result.initWithIdentity();
    return result;
  }

  AffineTransform AffineTransform::createScale(const Vector& scale)
  {
    AffineTransform result;
    result.initWithScale(scale);
    return result;
  }

  AffineTransform AffineTransform::createRotation(const Vector& from,
                                                  const Vector& to)
  {
    AffineTransform result;
    result.initWithRotation(from, to);
    return result;
  }

  AffineTransform AffineTransform::createRotation(const Vector& axis,
                                                  Real angleInRadians)
  {
    AffineTransform result;
    result.initWithRotation(axis, angleInRadians);
    return result;
  }

  AffineTransform AffineTransform::createTranslation(const Vector& translation)
  {
    AffineTransform result;
    result.initWithTranslation(translation);
    return result;
  }

  AffineTransform AffineTransform::inverse() const
  {
    AffineTransform result(*this);
    result.invert();
    return result;
  }

  void AffineTransform::invert()
  {

    Real a = mat[0][0];
    Real b = mat[0][1];
    Real c = mat[0][2];
    Real d = mat[1][0];
    Real e = mat[1][1];
    Real f = mat[1][2];
    Real g = mat[2][0];
    Real h = mat[2][1];
    Real i = mat[2][2];

    Real denom = c*d*h-c*e*g-b*d*i+b*f*g+a*e*i-a*f*h;

    Real inv_denom = (Real)(1)/denom;
    mat[0][0] = (e*i-f*h)*inv_denom;
    mat[0][1] = (c*h-b*i)*inv_denom;
    mat[0][2] = (b*f-c*e)*inv_denom;
    mat[1][0] = (f*g-d*i)*inv_denom;
    mat[1][1] = (a*i-c*g)*inv_denom;
    mat[1][2] = (c*d-a*f)*inv_denom;
    mat[2][0] = (d*h-e*g)*inv_denom;
    mat[2][1] = (b*g-a*h)*inv_denom;
    mat[2][2] = (a*e-b*d)*inv_denom;
    // These were double, but I changed them to type Real.  Do they need
    // to be double for some reason?
    Real x = mat[0][3];
    Real y = mat[1][3];
    Real z = mat[2][3];
    mat[0][3] = -(mat[0][0]*x + mat[0][1]*y + mat[0][2]*z);
    mat[1][3] = -(mat[1][0]*x + mat[1][1]*y + mat[1][2]*z);
    mat[2][3] = -(mat[2][0]*x + mat[2][1]*y + mat[2][2]*z);
  }

  AffineTransform AffineTransform::operator*(const AffineTransform& m) const
  {
    /*
     * Multiplication of two matrices.  They look like:
     * A: this, B: m, C: result
     * [A00 A01 A02 A03]  [B00 B01 B02 B03]   [C00 C01 C02 C03]
     * [A10 A11 A12 A13]  [B10 B11 B12 B13] = [C10 C11 C12 C13]
     * [A20 A21 A22 A23]  [B20 B21 B22 B23]   [C20 C21 C22 C23]
     * [  0   0   0   1]  [  0   0   0   1]   [  0   0   0   1]
     */
    AffineTransform result;
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
        Real sum = 0;
        for(int k=0;k<3;k++){
          sum += mat[i][k] * m.mat[k][j];
        }
        result.mat[i][j] = sum;
      }
      Real sum = mat[i][3];
      for(int k=0;k<3;k++)
        sum += mat[i][k] * m.mat[k][3];
      result.mat[i][3] = sum;
    }
    return result;
  }

  AffineTransform& AffineTransform::operator*=(const AffineTransform& m)
  {
    Real tmp[3][4];

    for(int row = 0; row < 3; row++){
      for(int col = 0; col < 4; col++){
        tmp[row][col] = mat[row][col];
      }
    }
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
        Real sum = 0;
        for(int k=0;k<3;k++){
          sum += tmp[i][k] * m.mat[k][j];
        }
        mat[i][j] = sum;
      }
      Real sum = mat[i][3];
      for(int k=0;k<3;k++)
        sum += tmp[i][k] * m.mat[k][3];
      mat[i][3] = sum;
    }
    return *this;
  }

  // Private helper function.
  void AffineTransform::pre_multiply( Real pre[3][3] ) {
    Real tmp[3][4];

    // Copy pre into tmp.
    for(int row = 0; row < 3; row++){
      for(int col = 0; col < 4; col++){
        tmp[row][col] = mat[row][col];
      }
    }

    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){
        Real sum = 0;
        for(int k=0;k<3;k++){
          sum += pre[i][k] * tmp[k][j];
        }
        mat[i][j] = sum;
      }
      Real sum = 0;
      for(int k=0;k<3;k++)
        sum += pre[i][k] * tmp[k][3];
      mat[i][3] = sum;
    }
  }

  Vector AffineTransform::multiply_point(const Vector& point) const {
    Vector result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[i][0] * point[0];
      result[i] += mat[i][1] * point[1];
      result[i] += mat[i][2] * point[2];
      result[i] += mat[i][3]; // point[3] == 1
    }
    return result;
  }
  Vector AffineTransform::multiply_point(const VectorT<Real, 4>& point) const {
    Vector result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[i][0] * point[0];
      result[i] += mat[i][1] * point[1];
      result[i] += mat[i][2] * point[2];
      result[i] += mat[i][3] * point[3];
    }
    // NOTE(boulos): Since points have a 4th component that's always one
    // we don't need the divide, right?
    return result;
  }
  Vector AffineTransform::multiply_vector(const Vector& vec) const {
    Vector result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[i][0] * vec[0];
      result[i] += mat[i][1] * vec[1];
      result[i] += mat[i][2] * vec[2];
      // vec[3] == 0
    }
    return result;
  }

  Vector AffineTransform::transpose_mult_point(const Vector& point) const {
    Vector result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[0][i] * point[0];
      result[i] += mat[1][i] * point[1];
      result[i] += mat[2][i] * point[2];
      //mat[3] (bottom row) is (0 0 0 1)
    }
    return result;
  }
  Vector AffineTransform::transpose_mult_vector(const Vector& vec) const {
    Vector result;
    for (int i=0;i<3;++i) {
      result[i]  = mat[0][i] * vec[0];
      result[i] += mat[1][i] * vec[1];
      result[i] += mat[2][i] * vec[2];
      // vec[3] == 0
    }
    return result;
  }

  std::ostream& operator<<( std::ostream& os, const AffineTransform& trans ) {
    for (int i=0;i<3;++i) {
      for (int j=0;j<4;++j) {
        os << trans(i,j) << " ";
      }
      os << std::endl;
    }
    return os;
  }

}
