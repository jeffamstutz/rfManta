/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2006
  Scientific Computing and Imaging Institute, University of Utah.

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

#include <Model/Readers/V3C1.h>

#include <Core/Util/Endian.h>
#include <Core/Exceptions/InputError.h>

#include <Core/Containers/Array1.h>
#include <Core/Exceptions/ErrnoException.h>

#include <string>
#include <errno.h>
#include <stdio.h>

using namespace std;

using namespace Manta;

namespace V3c1 {
  size_t fread_64(void *ptr, size_t size, size_t nitems, FILE *stream);
};

///////////////////////////////////////////////////////////////////////////////
// Endian swapping for V3C1 structs.
namespace Manta {
  template<>
  inline V3C1Triangle endian_swap( const V3C1Triangle &in ) {
    
    V3C1Triangle out;
    
    out.vertex[0] = endian_swap( in.vertex[0] );
    out.vertex[1] = endian_swap( in.vertex[1] );
    out.vertex[2] = endian_swap( in.vertex[2] );
    out.color = endian_swap( in.color );

    return out;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
extern "C"
void Manta::v3c1_load_triangles( Array1<V3C1Triangle> &array, const string &prefix ) {
  
  // Attempt to open the input file.
  FILE *file;
  if ((file = fopen( prefix.c_str(), "rb" )) == 0) {
    throw ErrnoException( "Error opening .v3c1 file.", errno, __FILE__, __LINE__);
  }
  
  // Determine input file size.
  fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

  // Check file size and allocate input buffer.
  if ((file_size % sizeof( V3C1Triangle )) != 0) {
    throw InputError( "Error .v3c1 file wrong size." );
  }
  
  size_t array_size = file_size / sizeof( V3C1Triangle );
  array.resize( array_size );

  // Read in the datafile.
  size_t total_read = V3c1::fread_64( &array[0], sizeof(V3C1Triangle),
                                array_size, file );

  if (total_read != array_size) {
    throw InputError( "Did not read expected number of triangles from file." );
  }

  // Check endianness and flip if necessary.
  if (is_big_endian()) {

    for (size_t i=0;i<array_size;++i) {
      array[i] = endian_swap( array[i] );
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
extern "C"
void Manta::v3c1_load_normals  ( Array1<V3C1Normal>   &array, const string &prefix ) {

  throw InputError( "Not finished" );
}

///////////////////////////////////////////////////////////////////////////////
// FREAD 64  FREAD 64  FREAD 64  FREAD 64  FREAD 64  FREAD 64  FREAD 64  FREAD 
///////////////////////////////////////////////////////////////////////////////
namespace V3c1 {
  size_t fread_64(void *ptr, size_t size, size_t nitems, FILE *stream)
  {
    long long chunkSize = 0x7fffffff;
    long long remaining = size*nitems;
    while (remaining > 0) {
      long long toRead = remaining>chunkSize?chunkSize:remaining;
      long long result;
      if ((result = fread(ptr, toRead, 1, stream)) != 1) {
        fprintf(stderr, "read error: %lld != %lld",
                toRead, result);
        return 0;
      }
      remaining -= toRead;
      ptr = (char*)ptr + toRead;
    }
    return nitems;
  }
}
