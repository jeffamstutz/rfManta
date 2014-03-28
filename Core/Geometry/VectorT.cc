
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

#include <Core/Geometry/VectorT.h>
#include <iostream>

namespace Manta {
  template<typename T, int Dim>
  std::ostream &operator<< (std::ostream &os, const VectorT<T,Dim> &v) {
    for (int i=0;i<Dim;++i)
      os << v[i] << " ";
    return os;
  }

  template<typename T, int Dim>
  std::istream &operator>> (std::istream &is, VectorT<T,Dim> &v) {
    for (int i=0;i<Dim;++i)
      is >> v[i];
    return is;
  }

  // Explicit instantiations for ostream operators

  // 3D Vectors
  template std::ostream& operator<<(std::ostream& os, const VectorT<long double, 3>&);
  template std::ostream& operator<<(std::ostream& os, const VectorT<double, 3>&);
  template std::ostream& operator<<(std::ostream& os, const VectorT<float, 3>&);
  template std::ostream& operator<<(std::ostream& os, const VectorT<int, 3>&);

  // 2D Vectors
  template std::ostream& operator<<(std::ostream& os, const VectorT<double, 2>&);
  template std::ostream& operator<<(std::ostream& os, const VectorT<float, 2>&);
  template std::ostream& operator<<(std::ostream& os, const VectorT<int, 2>&);

  // Explicit instantiations for istream operators

  // 3D Vectors
  template std::istream& operator>>(std::istream& is, VectorT<double, 3>&);
  template std::istream& operator>>(std::istream& is, VectorT<float, 3>&);
  template std::istream& operator>>(std::istream& is, VectorT<int, 3>&);

  // 2D Vectors
  template std::istream& operator>>(std::istream& is, VectorT<double, 2>&);
  template std::istream& operator>>(std::istream& is, VectorT<float, 2>&);
  template std::istream& operator>>(std::istream& is, VectorT<int, 2>&);
}
