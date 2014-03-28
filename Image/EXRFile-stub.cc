
/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

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

// This file provides some stubs, so that others that call these
// functions will be able to link even though the Teem stuff isn't
// being built.

#include <Image/EXRFile.h>
#include <Image/SimpleImage.h>

#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>

#include <fstream>


extern"C"
bool isEXR( std::string const &filename )
{
  std::ifstream file(filename.c_str(), std::ios_base::binary);

  char b[4];
  file.read (b, sizeof (b));

  return !!file && b[0] == 0x76 && b[1] == 0x2f && b[2] == 0x31 && b[3] == 0x01;
}


extern "C" void writeEXR( Manta::Image const *image,
    std::string const &file_name, int which )
{
  throw Manta::OutputError( "EXR writing not supported by this build.  Perhaps \
      you need to add the path to the OpenEXR libraries.");
}


extern "C" Manta::Image *readEXR( const std::string &file_name )
{
  throw Manta::InputError( "EXR reading not supported by this build.  Perhaps \
      you need to add the path to the OpenEXR libraries.");

  return NULL;
}


extern "C" bool EXRSupported()
{
  return false;
}


