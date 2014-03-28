/*
  For more information, please see: http://software.sci.utah.edu
 
  The MIT License
 
  Copyright (c) 2005
  Scientific Computing and Imaging Institute
 
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

#include <Core/Util/ThreadStorage.h>
#include <Interface/Context.h>
#include <cstdlib>

using namespace Manta;

ThreadStorage::Allocator::Allocator( const RenderContext &context, Token token_ )
  : storage( *context.storage_allocator ), proc( context.proc ), token( token_ ) { }

ThreadStorage::ThreadStorage( int num_procs_ ) :
  num_procs( num_procs_ ),
  requested( 0 )
{
  // Pad the array up to a cache line.
  size_t bytes = (num_procs_*sizeof(char *));
  bytes = (bytes % 128) ? ((bytes/128)+1)*128 : bytes;
  
  // Allocate array of pointers.
  storage = (char **)malloc(bytes);

  // Initialize all of the pointers to zero.
  for (int i=0;i<num_procs;++i) {
    storage[i] = 0;
  }
};

ThreadStorage::~ThreadStorage() {

  // Free the local memory for each thread.
  for (int i=0;i<num_procs;++i) {
    if (storage[i])
      free( storage[i] );
  }  
  free( storage );
}

// Called by each thread to allocate its memory. Will reallocate if necessary.
void ThreadStorage::allocateStorage( int proc ) {

  // Delete the storage if it exists.
  if (storage[proc])
    free( storage[proc] );

  // Allocate aligned storage.
  storage[proc] = (char *)malloc(requested);

  if (storage[proc] == 0)
    throw InternalError( "Could not allocate thread local memory");
}

