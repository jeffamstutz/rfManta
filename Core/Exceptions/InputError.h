
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

// Abe Stephens, December 2005

#ifndef Manta_Core_InputError_h
#define Manta_Core_InputError_h

#include <Core/Exceptions/Exception.h>
#include <string>

namespace Manta {
  using namespace std;

  // Input Errors are thrown when a problem occurs reading or parsing some type
  // of input, such as a texture is encountered.
  class InputError : public Exception {
  public:
    InputError(const std::string&);
    InputError(const InputError&);
    virtual ~InputError() throw();
    virtual const char* message() const;
    virtual const char* type() const;
  protected:
  private:
    std::string message_;
    InputError& operator=(const InputError&);
  };
}

#endif
