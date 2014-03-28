

#ifndef MANTA_MODEL_V3C1__H
#define MANTA_MODEL_V3C1__H


/*
 For more information, please see: http://software.sci.utah.edu
 
 The MIT License
 
 Copyright (c) 2005
 Silicon Graphics Inc. Mountain View California.
 
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
#include <Core/Color/Color.h>
#include <Core/Geometry/VectorT.h>

#include <Core/Util/Assert.h>
#include <Core/Containers/Array1.h>

#include <string>

namespace Manta {

  /////////////////////////////////////////////////////////////////////////////
  // Binary formats for triangles, normals, and texture coords read from
  // .v3c1 files.
  /////////////////////////////////////////////////////////////////////////////

  struct V3C1Triangle {
    VectorT<float,3> vertex[3];  // 9 floats.
    Color            color;      // 3 floats.
  };

  struct V3C1TriangleEdge {
    VectorT<float,3> v;
    Color payload;
    VectorT<float,3> edge1;
    VectorT<float,3> edge2;
  };
  
  struct V3C1Normal {
    VectorT<float,3> normal[3];  // 9 floats.

    VectorT<float,3>& operator[] ( int i ) { return normal[i]; };
    const VectorT<float,3>& operator[] ( int i ) const { return normal[i]; };    
  };

  struct V3C1TextureCoord {
    VectorT<float,2> coord[3];
    
    VectorT<float,2> &operator[] ( int i ) { return coord[i]; };
    const VectorT<float,2> &operator[] ( int i ) const { return coord[i]; };    
  };

  /////////////////////////////////////////////////////////////////////////////
  // Loading functions.
  /////////////////////////////////////////////////////////////////////////////

  // Load data into arrays of corresponding type and correct endianness if
  // necessary.
  
  extern "C" void v3c1_load_triangles( Array1<V3C1Triangle> &array, const std::string &prefix );
  extern "C" void v3c1_load_normals  ( Array1<V3C1Normal>   &array, const std::string &prefix );
    
};

#endif
