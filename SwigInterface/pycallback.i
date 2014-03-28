%module pycallback

%include "exception.i"

// This will allow passing of PyObject*'s back and forth between
// python and C, through swig.
%typemap(in) PyObject* {
  $1 = $input;
}

%typemap(check) PyObject* function {
  if ($1 != 0 && !PyCallable_Check($1)) {
    SWIG_exception(SWIG_ValueError,"Function argument isn't callable.");
  }
}

%typemap(check) PyObject* args, PyObject* new_args {
  if ($1 != 0 && !PyTuple_Check($1)) {
    SWIG_exception(SWIG_ValueError,"Argument list isn't a tuple.");
  }
}

%typemap(out) PyObject* {
  $result = $1;
}

%{
#include <SwigInterface/pycallback.h>
#include <Core/Util/CallbackHandle.h>
#include <Core/Util/Callback.h>
#include <Core/Util/CallbackHelpers.h>
%}

%include <SwigInterface/pycallback.h>
%include <Core/Util/CallbackHandle.h>
%include <Core/Util/CallbackHelpers.h>

namespace Manta {
  %template(CallbackBase_2Data_Int_Int) CallbackBase_2Data<int, int>;
  // For animation callbacks
  %template(CallbackBase_3Data_Int_Int_BoolP) CallbackBase_3Data<int, int, bool&>;
}


