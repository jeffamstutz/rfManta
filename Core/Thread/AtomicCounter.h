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
 *  AtomicCounter: Thread-safe integer variable
 *
 *  Written by:
 *   Author: Steve Parker
 *   Department of Computer Science
 *   University of Utah
 *   Date: June 1997
 *
 *  Copyright (C) 1997 SCI Group
 */

#ifndef Core_Thread_AtomicCounter_h
#define Core_Thread_AtomicCounter_h

#include <MachineParameters.h>

#if defined(MANTA_X86) && !defined(_WIN32)
  #define USE_ATOMIC_COUNTER_X86
#endif

#ifdef USE_ATOMIC_COUNTER_X86
  #include <Core/Util/Align.h>
  #include <Parameters.h>
#endif

namespace Manta {

class AtomicCounter_private;

/**************************************

 CLASS
 AtomicCounter

 KEYWORDS
 Thread

 DESCRIPTION
 Provides a simple atomic counter.  This will work just like an
 integer, but guarantees atomicty of the ++ and -- operators.
 Despite their convenience, you do not want to make a large number
 of these objects.  See also WorkQueue.

 Not that this implementation does not offer an operator=, but
 instead uses a "set" method.  This is to avoid the inadvertant
 use of a statement like: x=x+2, which would NOT be thread safe.

****************************************/
#ifdef USE_ATOMIC_COUNTER_X86
class MANTA_ALIGN(MAXCACHELINESIZE) AtomicCounter {
#else
class AtomicCounter {
#endif
public:
  //////////
  // Create an atomic counter with an unspecified initial value.
  // <tt>name</tt> should be a static string which describes the
  // primitive for debugging purposes.
  AtomicCounter(const char* name);

  //////////
  // Create an atomic counter with an initial value.  name should
  // be a static string which describes the primitive for debugging
  // purposes.
  AtomicCounter(const char* name, int value);

  //////////
  // Destroy the atomic counter.
  ~AtomicCounter();

  //////////
  // Allows the atomic counter to be used in expressions like
  // a normal integer.  Note that multiple calls to this function
  // may return different values if other threads are manipulating
  // the counter.
  operator int() const;

  //////////
  // Increment the counter and return the new value.
  // This does not return AtomicCounter& like a normal ++
  // operator would, because it would destroy atomicity
  int operator++();

  //////////
  //    Increment the counter and return the old value
  int operator++(int);

  // Increment the counter by the given value and return the old
  // value. Like operator++ this doesn't return AtomicCounter& to
  // avoid atomicity issues.
  int operator+=(int);

  //////////
  // Decrement the counter and return the new value
  // This does not return AtomicCounter& like a normal --
  // operator would, because it would destroy atomicity
  int operator--();

  //////////
  // Decrement the counter and return the old value
  int operator--(int);

  //////////
  // Set the counter to a new value
  void set(int);

private:
  const char* name_;
#ifdef USE_ATOMIC_COUNTER_X86
  int value;
#else
  AtomicCounter_private* priv_;
#endif
  // Cannot copy them
  AtomicCounter(const AtomicCounter&);
  AtomicCounter& operator=(const AtomicCounter&);
};


#ifdef USE_ATOMIC_COUNTER_X86
/*
 *  AtomicCounter: Thread-safe integer variable written in x86 assembly
 *
 *  Written by:
 *   Author: Solomon Boulos
 *   Department of Computer Science
 *   University of Utah
 *   Date: 10-Sep-2007
 *
 */
inline AtomicCounter::operator int() const {
  return value;
}

inline
int
AtomicCounter::operator++() {
  __volatile__ register int return_val = 1;

  __asm__ __volatile__(
      "lock;\n"
      "xaddl %1, %0;\n" :
      "+m" (value), "+r"(return_val) :
      "m" (value) , "r" (return_val)
      /* no unknown clobbers */
    );
  return return_val + 1;
}

inline
int
AtomicCounter::operator++(int) {
  __volatile__ register int return_val = 1;

  __asm__ __volatile__(
      "lock;\n"
      "xaddl %1, %0;\n" :
      "+m" (value), "+r"(return_val) :
      "m" (value) , "r" (return_val)
      /* no unknown clobbers */
    );
  return return_val;
}

inline
int
AtomicCounter::operator+=(int val) {
  __volatile__ register int return_val = val;

  __asm__ __volatile__(
      "lock;\n"
      "xaddl %1, %0;\n" :
      "+m" (value), "+r"(return_val) :
      "m" (value) , "r" (return_val), "r" (val)
      /* no unknown clobbers */
    );
  return return_val;
}

inline
int
AtomicCounter::operator--() {
  __volatile__ register int return_val = -1;
  __asm__ __volatile__(
      "lock;\n"
      "xaddl %1, %0;\n" :
      "+m" (value), "+r"(return_val) :
      "m" (value) , "r" (return_val)
      /* no unknown clobbers */
    );
  return return_val - 1;
}

inline
int
AtomicCounter::operator--(int) {
  __volatile__ register int return_val = -1;
  __asm__ __volatile__(
      "lock;\n"
      "xaddl %1, %0;\n" :
      "+m" (value), "+r"(return_val) :
      "m" (value) , "r" (return_val)
      /* no unknown clobbers */
    );
  // The exchange returns the old value
  return return_val;
}

inline
void
AtomicCounter::set(int v) {
  __volatile__ register int copy_val = v;
  __asm__ __volatile__(
    "lock;\n"
    "xchgl %1, %0\n" :
    "+m" (value), "+r" (copy_val) :
    "m" (value), "r" (copy_val), "r" (v)
    /* no unknown clobbers */
    );
}
#endif // USE_ATOMIC_COUNTER_X86

}

#endif

