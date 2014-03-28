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

#ifndef _CORE_UTIL_ALIGNCLASS__H
#define _CORE_UTIL_ALIGNCLASS__H

#include <stddef.h> // for size_t definition
#include <Core/Util/Assert.h>
#include <Core/Exceptions/InternalError.h>

namespace Manta {

  // Helper function to get back an aligned memory address
  void* allocateAligned(size_t size, size_t alignment);
  // Deallocate the aligned memory.
  void  deallocateAligned(void* mem);

  // Inherit from this class if you want to be able to call new and
  // delete on an object.
  template<class ParentT, size_t Alignment = 16>
  class AlignedAllocator {
  public:
    void * operator new (size_t size)
    {
      ASSERT(size == sizeof (ParentT));
      return allocateAligned(size, Alignment);
    }
    void operator delete (void * mem)
    {
      deallocateAligned(mem);
    }
    void * operator new[] (size_t size)
    {
      // Only the first item in the array will be aligned.  If you
      // structure is not a multiple of the alignment size, you will
      // have unaligned addresses for subsequent elements in the array.
      if (size > sizeof(ParentT))
        if (sizeof(ParentT) % Alignment != 0)
          throw InternalError("Attempting to allocate an array of objects that cannot be aligned.  Please make sizeof(ParentT) a multiple of Alignment");
      return allocateAligned(size, Alignment);
    }
    void operator delete[](void * mem)
    {
      deallocateAligned(mem);
    }
  };

}  // end namespace Manta

#endif

