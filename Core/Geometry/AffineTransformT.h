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

#ifndef Manta_Core_AffineTransformT_h
#define Manta_Core_AffineTransformT_h

#include <Core/Geometry/VectorT.h>
#include <iosfwd>

namespace Manta {
  /**
   * A matrix class for affine transforms.  This saves storage and
   * some computation over a general 4x4 transformation.  For 3D, this
   * is a 3 row by 4 column matrix, and implicitly assume that a
   * fourth row would contain 0 0 0 1.
   *
   * Perhaps this could be generalized to support higher- and lower-
   * dimensional matrices, but a few things would need to be rethought,
   * such as rotation and the load_basis constructor.
   */
  template<typename T>
  class AffineTransformT {
  public:
    AffineTransformT() {
    }
    ~AffineTransformT() {
    }

    AffineTransformT(const AffineTransformT<T> &copy) {
      for(int i=0;i<3;i++)
        for(int j=0;j<4;j++)
          mat[i][j] = copy.mat[i][j];
    }
    AffineTransformT<T>& operator=(const AffineTransformT<T> &copy) {
      for(int i=0;i<3;i++)
        for(int j=0;j<4;j++)
          mat[i][j] = copy.mat[i][j];
      return *this;
    }

    // These methods set the current matrix
    void initWithIdentity();
    void initWithScale(const VectorT<T, 3>& scale);
    void initWithRotation(const VectorT<T, 3>& from, const VectorT<T, 3>& to);
    void initWithRotation(const VectorT<T, 3>& axis, T angleInRadians);
    void initWithTranslation(const VectorT<T, 3>& translation);
    void initWithBasis(const VectorT<T, 3>& v1, const VectorT<T, 3>& v2,
                       const VectorT<T, 3>& v3, const VectorT<T, 3>& p);

    // These methods premultiply the current matrix by the indicated transform
    void scale(const VectorT<T, 3>& scale);
    void rotate(const VectorT<T, 3>& from, const VectorT<T, 3>& to);
    void rotate(const VectorT<T, 3>& axis, T angleInRadians);
    void translate(const VectorT<T, 3>& translation);

    // These static methods return a new transform with the indicated transform
    static AffineTransformT<T> createScale(const VectorT<T, 3>& scale);
    static AffineTransformT<T> createRotation(const VectorT<T, 3>& from,
                                              const VectorT<T, 3>& to);
    static AffineTransformT<T> createRotation(const VectorT<T, 3>& axis,
                                              T angleInRadians);
    static AffineTransformT<T> createTranslation(const VectorT<T, 3>& translation);

    // Other methods
    AffineTransformT<T> inverse() const;
    void invert();

    // Const Accessors used by global multiplication operators.
    const T &operator() (unsigned int r, unsigned int c) const {
      return mat[r][c];
    }

    // Post-multiplication
    AffineTransformT<T> operator*(const AffineTransformT<T>& m) const;
    AffineTransformT<T>& operator*=(const AffineTransformT<T>& m);

    VectorT<T, 3> multiply_point(const VectorT<T, 3>& point) const;
    VectorT<T, 3> multiply_vector(const VectorT<T, 3>& vec) const;

    // Multiply by the transpose of the AffineTransformation, useful for
    // computations involving an inverse transpose.
    VectorT<T, 3> transpose_mult_point(const VectorT<T, 3>& point) const;
    VectorT<T, 3> transpose_mult_vector(const VectorT<T, 3>& vec) const;
  private:
    T mat[3][4];

    // Pre-multiplication.
    void pre_multiply(T m[3][3]);
  };

  template<typename T>
  std::ostream& operator<<(std::ostream& os,
                           const AffineTransformT<T>& trans );
}

#endif
