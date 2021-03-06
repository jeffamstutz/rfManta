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
 *  StringUtil.h: some useful string functions
 *
 *  Written by:
 *   Michael Callahan
 *   Department of Computer Science
 *   University of Utah
 *   April 2001
 *
 *  Copyright (C) 2001 SCI Group
 */

#ifndef SCI_Core_StringUtil_h
#define SCI_Core_StringUtil_h 1

#include <string>
#include <vector>

namespace Manta {
  using std::string;
  using std::vector;

  bool string_to_int(const string &str, int &result);
  bool string_to_double(const string &str, double &result);
  bool string_to_unsigned_long(const string &str, unsigned long &res);

  string to_string(int val);
  string to_string(unsigned int val);
  string to_string(unsigned long val);
  string to_string(double val);

  string string_toupper(string);
  string string_tolower(string);

  int StrCaseCmp(const char* s1, const char* s2);

//////////
// Remove directory name
  string basename(const string &path);

//////////
// Return directory name
  string pathname(const string &path);

// Split a string into multiple parts, separated by the character sep
  vector<string> split_string(const string& str, char sep);

/////////
// C++ify a string, turn newlines into \n, use \t, \r, \\ \", etc.
  string string_Cify(const string &str);

//////////
// Unsafe cast from string to char *, used to export strings to C functions.
  char* ccast_unsafe(const string &str);

// replaces all occurances of 'substr' in 'str' with 'replacement'
  string replace_substring(string str, 
                                  const string &substr, 
                                  const string &replacement);

// Returns true if 'str' ends with the string 'substr'
  bool ends_with(const string &str, const string &substr);


} // End namespace Manta

#endif
