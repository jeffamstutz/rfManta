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

#ifndef Manta_Core_AffineTransform_h
#define Manta_Core_AffineTransform_h

#include <MantaTypes.h>
#include <Core/Geometry/Vector.h>
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
  class AffineTransform {
  public:
    AffineTransform() {
    }
    ~AffineTransform() {
    }

    AffineTransform(const AffineTransform &copy) {
      for(int i=0;i<3;i++)
        for(int j=0;j<4;j++)
          mat[i][j] = copy.mat[i][j];
    }
#ifndef SWIG
    AffineTransform& operator=(const AffineTransform &copy) {
      for(int i=0;i<3;i++)
        for(int j=0;j<4;j++)
          mat[i][j] = copy.mat[i][j];
      return *this;
    }
#endif

    // These methods set the current matrix
    void initWithIdentity();
    void initWithScale(const Vector& scale);
    void initWithRotation(const Vector& from, const Vector& to);
    void initWithRotation(const Vector& axis, Real angleInRadians);
    void initWithTranslation(const Vector& translation);
    void initWithBasis(const Vector& v1, const Vector& v2,
                       const Vector& v3, const Vector& p);

    // These methods premultiply the current matrix by the indicated transform
    void scale(const Vector& scale);
    void rotate(const Vector& from, const Vector& to);
    void rotate(const Vector& axis, Real angleInRadians);
    void translate(const Vector& translation);

    // These static methods return a new transform with the indicated transform
    static AffineTransform createIdentity();
    static AffineTransform createScale(const Vector& scale);
    static AffineTransform createRotation(const Vector& from,
                                          const Vector& to);
    static AffineTransform createRotation(const Vector& axis,
                                          Real angleInRadians);
    static AffineTransform createTranslation(const Vector& translation);

    // Other methods
    AffineTransform inverse() const;
    void invert();
    bool isIdentity() const;

    // Const Accessors used by global multiplication operators.
    const Real &operator() (unsigned int r, unsigned int c) const {
      return mat[r][c];
    }
    // Accessors used by global multiplication operators.
    Real &operator() (unsigned int r, unsigned int c) {
      return mat[r][c];
    }

    // Post-multiplication
    AffineTransform operator*(const AffineTransform& m) const;
    AffineTransform& operator*=(const AffineTransform& m);

    Vector multiply_point(const Vector& point) const;
    Vector multiply_point(const VectorT<Real, 4>& point) const;
    Vector multiply_vector(const Vector& vec) const;

    // Multiply by the transpose of the AffineTransformation, useful for
    // computations involving an inverse transpose.
    Vector transpose_mult_point(const Vector& point) const;
    Vector transpose_mult_vector(const Vector& vec) const;

    Real mat[3][4]; //TODO: Once AffineTransformation is Interpolable,
                    //make this private again!.
  private:

    // Pre-multiplication.
    void pre_multiply(Real m[3][3]);
  };

  std::ostream& operator<<( std::ostream& os, const AffineTransform& trans );
}

#endif
