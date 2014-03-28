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


#ifndef Manta_SwigInterface_pycallback_h
#define Manta_SwigInterface_pycallback_h

#include <Python.h>

#include <Core/Util/Callback.h>

#include <stdio.h>

namespace Manta {
  
  /* This is the thing that is supposed to keep reference counts on
     our objects.  Unfortunately, when a pointer to a swig proxy class
     is passed in it causes a seg fault when deleted.  Since we have
     no idea what is a swig proxy class and what isn't we will simply
     have to error on the side of caution and not delete anything.
     Currently the reference counters are incremented, but not
     decremented.
   */
  class PyObjectContainer {
  public:
    PyObjectContainer(PyObject* object_in);
    // Copy constructor
    PyObjectContainer(const PyObjectContainer& copy);
    ~PyObjectContainer();

    PyObject* operator()() { return object; }
    void set(PyObject* new_object);
  private:
    // Don't make "object" public.  We need to make sure that things
    // get reference counted properly.
    PyObject* object;
  };


  // All the type checking for the "function", "args", and "new_args"
  // parameters is done for us in SWIG, so you don't need to do it in
  // the class itself.  BTW, don't change the names of these
  // parameters without making updates to pycallback.i.
#ifndef SWIG
  // You should not call this from swigged code as it will lock the
  // interpreter.
  void callFromC(PyObjectContainer function, PyObjectContainer args);
#endif

  Manta::CallbackBase_0Data*
  createMantaTransaction(PyObject* function, PyObject* args);

#ifndef SWIG
  void callFromC_OneShot(int a1, int a2,
                         PyObjectContainer function, PyObjectContainer args);
  void callFromC_Animation(int a1, int a2, bool&,
                           PyObjectContainer function, PyObjectContainer args);
#endif
  Manta::CallbackBase_2Data<int, int>*
  createMantaOneShotCallback(PyObject* function, PyObject* args);

  Manta::CallbackBase_3Data<int, int, bool&>*
  createMantaAnimationCallback(PyObject* function, PyObject* args);

} // end namespace Manta

#endif // #ifndef Manta_SwigInterface_pycallback_h
