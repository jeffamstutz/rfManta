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

#include <Core/Util/AlignedAllocator.h>

#include <stdlib.h>
#include <errno.h>

// Helper function to get back an aligned memory address
void* Manta::allocateAligned(size_t size, size_t alignment)
{
  void* memptr;
#if defined( __APPLE__) || defined (__CYGWIN__)
  int return_code = ( ((memptr = malloc(size)) == NULL) ? ENOMEM : 0);
#elif defined( _WIN32 ) || defined( __WIN32__ ) || defined( WIN32 )
  int return_code = ( ((memptr = _aligned_malloc(size, alignment)) == NULL) ? ENOMEM : 0 );
#else
  int return_code = posix_memalign(&memptr, alignment, size);
#endif
  switch (return_code) {
  case 0:
    return memptr;
    break;
  case EINVAL:
    // Throw an exception?
    break;
  case ENOMEM:
    // Throw an exception?
    break;
  }
  return 0;
}

// Deallocate the aligned memory.
void  Manta::deallocateAligned(void* mem)
{
  if (mem)
    free(mem);
}



