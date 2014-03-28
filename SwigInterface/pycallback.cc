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

#include <SwigInterface/pycallback.h>

#include <iostream>

using namespace Manta;
using namespace std;


// #define INCREMENT_PYTHON_REFERENCE_COUNTER 1
// #define DECREMENT_PYTHON_REFERENCE_COUNTER 1

PyObjectContainer::PyObjectContainer(PyObject* object_in)
  : object(object_in)
{
#ifdef INCREMENT_PYTHON_REFERENCE_COUNTER
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  Py_XINCREF(object);
  PyEval_ReleaseLock();
#endif
}

// Copy constructor
PyObjectContainer::PyObjectContainer(const PyObjectContainer& copy)
  : object(copy.object)
{
#ifdef INCREMENT_PYTHON_REFERENCE_COUNTER
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  Py_XINCREF(object);
  PyEval_ReleaseLock();
#endif
}

PyObjectContainer::~PyObjectContainer()
{
#ifdef DECREMENT_PYTHON_REFERENCE_COUNTER
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  Py_XDECREF(object);
  PyEval_ReleaseLock();
#endif
}

void
PyObjectContainer::set(PyObject* new_object)
{
#if defined(INCREMENT_PYTHON_REFERENCE_COUNTER) || defined(DECREMENT_PYTHON_REFERENCE_COUNTER)
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
#endif
#ifdef INCREMENT_PYTHON_REFERENCE_COUNTER
  Py_XINCREF(new_object);
#endif
#ifdef DECREMENT_PYTHON_REFERENCE_COUNTER
  Py_XDECREF(object);
#endif
#if defined(INCREMENT_PYTHON_REFERENCE_COUNTER) || defined(DECREMENT_PYTHON_REFERENCE_COUNTER)
  PyEval_ReleaseLock();
#endif
  object = new_object;
}

// This function assumes that someone has locked the GIL
static void
makePyCall(PyObject* function, PyObject* args)
{
  // Call the python function
  PyObject* result = PyEval_CallObject(function, args);
  
  // Check the result
  if (result == NULL) {
    // Something went wrong
    PyObject* error = PyErr_Occurred();
    if (error) {
      PyErr_Print();
    }
  } else {
    // We don't care what the result is
    Py_DECREF(result);
  }
}

namespace Manta {
  
void
callFromC(PyObjectContainer function, PyObjectContainer args)
{
  // Get ready to call python code
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  makePyCall(function(), args());
  
  // Done with python code
  PyGILState_Release(gstate);
}

CallbackBase_0Data*
createMantaTransaction(PyObject* function, PyObject* args)
{
#ifndef INCREMENT_PYTHON_REFERENCE_COUNTER
  Py_XINCREF(function);
  Py_XINCREF(args);
#endif
  return Callback::create(&callFromC,
                          PyObjectContainer(function),
                          PyObjectContainer(args));
}

void
callFromC_OneShot(int a1, int a2,
                  PyObjectContainer function, PyObjectContainer args)
{
  // Get ready to call python code
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  // Create a new Touple to accomodate the new args
  int num_args = PyTuple_Size(args());
  PyObject* new_args = PyTuple_New(num_args + 2);
  // Pack the new args to the arguments
  PyTuple_SET_ITEM(new_args, 0, PyInt_FromLong(a1));
  PyTuple_SET_ITEM(new_args, 1, PyInt_FromLong(a2));
  for(int i = 0; i < num_args; ++i) {
    PyTuple_SET_ITEM(new_args, i+2, PyTuple_GET_ITEM(args(), i));
  }
  
  makePyCall(function(), new_args);

#ifdef DECREMENT_PYTHON_REFERENCE_COUNTER
  // You can't decrement this here, because it could lead to deleted
  // SWIG proxy classes.
  Py_DECREF(new_args);
#endif

  // Done with python code
  PyGILState_Release(gstate);
}

void
callFromC_Animation(int a1, int a2, bool& b1,
                    PyObjectContainer function, PyObjectContainer args)
{
  // Get ready to call python code
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  // Create a new Touple to accomodate the new args
  int num_args = PyTuple_Size(args());
  PyObject* new_args = PyTuple_New(num_args + 3);
  // Pack the new args to the arguments
  PyTuple_SET_ITEM(new_args, 0, PyInt_FromLong(a1));
  PyTuple_SET_ITEM(new_args, 1, PyInt_FromLong(a2));
  // Create a dictionary to pass in/out return parameters
  PyObject* return_parameters = PyDict_New();
  if (PyDict_SetItemString(return_parameters,
                           "changed",
                           b1 ? Py_True : Py_False)) {
    // There was some kind of a problem
    cerr << "Can't add item to dictionary.  Aborting callback.\n";
    // Something went wrong
    PyObject* error = PyErr_Occurred();
    if (error) {
      PyErr_Print();
    }
    Py_DECREF(return_parameters);
    // goto cleanup; // G++ Doesn't seem to like this.
#ifdef DECREMENT_PYTHON_REFERENCE_COUNTER
    // You can't decrement this here, because it could lead to deleted
    // SWIG proxy classes.
    Py_DECREF(new_args);
#endif

    // Done with python code
    PyGILState_Release(gstate);
    return;
  }
  PyTuple_SET_ITEM(new_args, 2, return_parameters);
  for(int i = 0; i < num_args; ++i) {
    PyTuple_SET_ITEM(new_args, i+2, PyTuple_GET_ITEM(args(), i));
  }
  
  makePyCall(function(), new_args);

  // Now pull out the value
  PyObject* changed = PyDict_GetItemString(return_parameters,
                                           "changed");
  if (changed != NULL) {
    // Check to see if this is a bool object
    bool new_b1;
    if (PyBool_Check(changed)) {
      // Get the value
      if (changed == Py_True) new_b1 = true;
      else if (changed == Py_False) new_b1 = false;
      else {
        // Shouldn't really need to check this, but I'm pedantic.
        cerr << "changed is a Bool, but not Py_True or Py_False\n";
        goto cleanup;
      }
    } else if (PyInt_Check(changed)) {
      // Some goofy person could have specified an int.
      
      // Get the value.
      long val = PyInt_AsLong(changed);
      new_b1 = val != 0;
    } else {
      cerr << "changed value isn't a bool or int\n";
      goto cleanup;
    }
    // cerr << "b1 = "<<b1<<", new_b1 = "<<new_b1<<"\n";
    if (new_b1 != b1) b1 = new_b1;
  }
 cleanup:
#ifdef DECREMENT_PYTHON_REFERENCE_COUNTER
  // You can't decrement this here, because it could lead to deleted
  // SWIG proxy classes.
  Py_DECREF(new_args);
#endif

  // Done with python code
  PyGILState_Release(gstate);
}

CallbackBase_2Data<int, int>*
createMantaOneShotCallback(PyObject* function, PyObject* args)
{
#ifndef INCREMENT_PYTHON_REFERENCE_COUNTER
  Py_XINCREF(function);
  Py_XINCREF(args);
#endif
  return Callback::create(&callFromC_OneShot,
                          PyObjectContainer(function),
                          PyObjectContainer(args));
}

CallbackBase_3Data<int, int, bool&>*
createMantaAnimationCallback(PyObject* function, PyObject* args)
{
#ifndef INCREMENT_PYTHON_REFERENCE_COUNTER
  Py_XINCREF(function);
  Py_XINCREF(args);
#endif
  return Callback::create(&callFromC_Animation,
                          PyObjectContainer(function),
                          PyObjectContainer(args));
}

} // end namespace Manta
