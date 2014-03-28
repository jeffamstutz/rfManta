#ifndef Manta_Util_ThreadStorage__H
#define Manta_Util_ThreadStorage__H

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

// Abe Stephens

#include <Core/Exceptions/InternalError.h>
#include <Core/Util/Assert.h>
#include <vector>

namespace Manta {
  using std::vector;

  class RenderContext;
  
  ///////////////////////////////////////////////////////////////////////////
  // Thread local storage allocation.
  // Storage will be allocated after setupDisplayChannel but before the next
  // call to setupFrame.
  class ThreadStorage {
  private:
    // Actual storage.
    char ** storage;
    int     num_procs;

    // Requested storage.
    size_t requested;
       
  public:

    class Allocator;
    
    ///////////////////////////////////////////////////////////////////////////
    // Token is returned by an allocation request.
    // 
    // Tokens are shared by all of the threads. Individual storage is accessed
    // using a token and proc number.
    class Token {
      friend class ThreadStorage;
      friend class Allocator;
    public:
      Token() { };
    private:
      size_t offset;
      size_t size;
    protected:
      Token( size_t offset_, size_t size_ ) : offset( offset_ ), size( size_ ) {  };
    };    

    ///////////////////////////////////////////////////////////////////////////
    // Allocator
    // Allocator are passed to overloaded new operators to allocate and
    // initalize per thread storage.
    class Allocator {
    public:
      Allocator( const RenderContext &context, Token token_ );
      Allocator( ThreadStorage &storage_, int proc_, Token token_ )
        : storage( storage_ ), proc( proc_ ), token( token_ ) {  };

      // Attempt to allocate the specified number of bytes.
      void *check_fit( size_t bytes ) const {
        if (bytes > token.size)
          throw InternalError( "Requested number of bytes greater than allocated.");

        return storage.get( proc, token );
      }

      void *operator *() const { return storage.get( proc, token ); }
      
    private:
      ThreadStorage &storage;
      int            proc;
      Token          token;
    };
    
    ///////////////////////////////////////////////////////////////////////////
    // Constructor.
    ThreadStorage( int num_procs_ );
    ThreadStorage(  );
    ~ThreadStorage();

    Token requestStorage( size_t size, size_t aligned ) {

      // Determine how many bytes needed.
      size_t bytes = size;
      
      // Find the next aligned offset.
      size_t offset = requested +
        ((requested % aligned) ? ((requested/aligned)+1)*aligned : 0);

      requested = offset + bytes;

      return Token( offset, bytes );
    }

    // Obtain an accessor to the storage. This is only possible after it has been allocated.
    void *get( int proc, const Token &token ) {

      ASSERT(storage[proc]);
      if (storage[proc]) {

        ASSERT(token.offset+token.size <= requested)
        return storage[proc]+token.offset;
      }
      return NULL;
    }
    
    // Called by each thread to allocate its memory. Will reallocate if necessary.
    void allocateStorage( int proc );

  };
};

#endif

