/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

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



/*
 *  ErrnoException.h: Generic exception for internal errors
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1999
 *
 *  Copyright (C) 1999 SCI Group
 */

#include <Core/Exceptions/ErrnoException.h>
#include <sstream>
#include <iostream>
#include <cstring>

namespace Manta {

using namespace std;

ErrnoException::ErrnoException(const std::string& message, int err, const char* file, int line)
   : errno_(err)
{
   ostringstream str;
   const char* s = strerror(err);
   if(!s)
      s="(none)";
   str << "An ErrnoException was thrown.\n"
       << file << ":" << line << "\n"
       << message << " (errno=" << err << ": " << s << ")";
   message_ = str.str();

#ifdef EXCEPTIONS_CRASH
   std::cout << message_ << "\n";
#endif
}

ErrnoException::ErrnoException(const ErrnoException& copy)
   : message_(copy.message_), errno_(copy.errno_)
{
}

ErrnoException::~ErrnoException() throw()
{
}

const char* ErrnoException::message() const
{
   return message_.c_str();
}

const char* ErrnoException::type() const
{
   return "ErrnoException";
}

int ErrnoException::getErrno() const
{
   return errno_;
}

} // End namespace Manta
