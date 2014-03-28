/*
  Swig wrapper file for CSAFE project.
  Carson Brownlee
*/

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

%module csafe
#pragma SWIG nowarn=512
#pragma SWIG nowarn=503
// #pragma SWIG nowarn=401

%include "std_string.i"
%include "std_vector.i"
%include "exception.i"


// Import mantainterface.i which is the "Manta Interface" swig file. This
// includes wrappers for interfaces which your code might implement
// as well as wrappers for the core utilites. 

// Wrappers for the manta implementation (Model/, Engine/, Image/, 
// etc.) is not contained in this file. Python scripts that use both
// your wrapped classes and the manta wrapped classes should import python 
// modules "manta" and "example" (your new module). See python/test.py.

%{
namespace Manta {
  class Group;
  class Mesh;
  class AccelerationStructure;
  class OpaqueShadower;
  class LitMaterial;
  class RGBAColorMap;
  //class AlignedAllocator<class T>;
};
%}
%import mantainterface.i

//namespace Manta {
//  %template(AlignedAllocator_Volume_float) Manta::AlignedAllocator<Manta::Volume<float> >;
//  %template(AlignedAllocator_Box4)  Manta::AlignedAllocator<Manta::Box4>;
//}
%warnfilter(401) Manta::Box4;
//%warnfilter(503) Volume;

%{
#include <Model/Materials/Volume.h>
#include <CDSWIGIFY.h>
#include <CDTest.h>
//#include <Core/Util/AlignedAllocator.h>
%}
//%ignore Manta::Volume;
//%ignore Manta::AlignedAllocator;
%ignore Manta::Box4;

%include <CDTest.h>
%include <Model/Materials/Volume.h>
%include <CDSWIGIFY.h>
 
%template(vector_ColorSlice) ::std::vector<ColorSlice>;

