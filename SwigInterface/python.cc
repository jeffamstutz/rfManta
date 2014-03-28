
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


#include <Interface/Context.h>
#include <Core/Exceptions/InternalError.h>

#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>

#include <string>
#include <iostream>

using namespace Manta;

#include <Python.h>

/*

# Example python scene file.
from manta import *

def make_scene(prefix):
    print "In python function make_scene: " + prefix + "\n";

    # Create the scene.
    scene = manta_new( Scene() );
    scene.setBackground(manta_new(ConstantBackground(ColorDB.getNamedColor("SkyBlue3").scaled(0.5))))

    # Create a material.
    material = manta_new( Flat( manta_new( NormalTexture() ) ) );

    # Add the sphere.
    world = manta_new(Group())
    world.add(manta_new( Sphere( material,
                                        Vector( 0, 0, 0 ),
                                        1.0 ) ) )
    scene.setObject( world )

    # Add light set.
    lights = manta_new( LightSet() )
    lights.add(manta_new(PointLight(Vector(20, 30, 100), Color(RGBColor(.9,.9,.9)))))
    lights.setAmbientLight(manta_new(ConstantAmbient(Color.black())))
    scene.setLights( lights );

    scene.getRenderParameters().maxDepth = 5

    return scene;

 */


///////////////////////////////////////////////////////////////////////////////
// This function is called to invoke a python script. Manta Interface is
// passed to the script.
void runPythonScript( MantaInterface *manta_interface, const std::string &file_name );  

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MAKE SCENE  MAKE SCENE  MAKE SCENE  MAKE SCENE  MAKE SCENE  MAKE SCENE  MAKE
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" 
Scene* make_scene(const ReadContext& context, const vector<string>& args) {

  /////////////////////////////////////////////////////////////////////////////
  // Obtain the filename of the python application.
  string file_name;
  string function_name = "make_scene";

  int first_arg = -1;
  
  for (int i=0;i<args.size();++i) {
    if (args[i] == "-file") {
      if (!getStringArg( i, args, file_name )) {
        throw IllegalArgument("-file <filename>", i, args);
      }
    }
    else if (args[i] == "-function") {
      if (!getStringArg( i, args, function_name )) {
        throw IllegalArgument("-function <name>", i, args);
      }
    }
    else {
      // Beginning of function arguments
      first_arg = i;
      break;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // From: http://dinsdale.python.org/doc/2.2.3/ext/pure-embedding.html  
  PyObject *pName, *pModule, *pDict, *pFunc;
  PyObject *pArgs, *pValue;

  Py_Initialize();

  pName = PyString_FromString( file_name.c_str() );
  /* Error checking of pName left out */

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule != NULL) {
    pDict = PyModule_GetDict(pModule);
    /* pDict is a borrowed reference */

    pFunc = PyDict_GetItemString(pDict, function_name.c_str() );
    /* pFun: Borrowed reference */

    if (pFunc && PyCallable_Check(pFunc)) {
      int num_args = args.size()-first_arg;
      fprintf(stderr, "num args %d", num_args);
      pArgs = PyTuple_New( num_args );
      for (int argi = first_arg; argi < args.size(); ++argi) {
        pValue = PyString_FromString(args[argi].c_str());
        if (!pValue) {
          Py_DECREF(pArgs);
          Py_DECREF(pModule);
          fprintf(stderr, "Cannot convert argument\n");
          return 0;
        }
        /* pValue reference stolen here: */
        PyTuple_SetItem(pArgs, argi-first_arg, pValue);
      }

      // Invoke the function.
      pValue = PyObject_CallObject(pFunc, pArgs);
      Py_DECREF(pArgs);

      if (pValue != NULL) {
        std::cerr << "Function call successful" << std::endl;
        //        printf("Result of call: %s\n", PyString_AsString(pValue));
        Py_DECREF(pValue);
      }
      else {
        Py_DECREF(pModule);
        PyErr_Print();
        fprintf(stderr,"Call failed\n");
        return 0;
      }
      /* pDict and pFunc are borrowed and must not be Py_DECREF-ed */
    }
    else {
      if (PyErr_Occurred())
        PyErr_Print();
      fprintf(stderr, "Cannot find python function \"%s\"\n", function_name.c_str());
    }
    Py_DECREF(pModule);
  }
  else {
    PyErr_Print();
    fprintf(stderr, "Failed to load \"%s\"\n", file_name.c_str());
    return 0;
  }
  Py_Finalize();

  /////////////////////////////////////////////////////////////////////////////
  // Convert the returned value to a Scene pointer.

  return 0;
}

#if 0

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// RUN PYTHON SCRIPT  RUN PYTHON SCRIPT  RUN PYTHON SCRIPT  RUN PYTHON SCRIPT 
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void runPythonScript( MantaInterface *manta_interface,
                             const std::string &file_name ) {

  // Initialize Python.
  Py_Initialize();

  /////////////////////////////////////////////////////////////////////////////
  // Attempt to load the Manta module.
  PyObject *manta_module = PyImport_ImportModule( "manta.py" );
  if (manta_module == 0) {
    PyErr_Print();
    PyErr_Clear();
    goto python_finalize;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Create a python object for the Manta Interface.
  PyObject *python_manta_interface =
    SWIG_NewPointerObj((void*)manta_interface,
                       SWIGTYPE_p_Manta__MantaInterface, 0);
  
  /////////////////////////////////////////////////////////////////////////////
  // Create a local dictionary containing the manta interface.
  PyObject *manta_dict = PyDict_New();

  // Add the MantaInterface to the dictionary.
  PyDict_SetItemString( manta_dict, "manta_interface", python_manta_interface );
  
  /////////////////////////////////////////////////////////////////////////////
  // Run the script.

  // Open the file.
  FILE *file = fopen( file_name.c_str(), "rb" );
  if (file == 0) {
    perror( "Could not open python script." );
    goto python_finalize;
  }

  // Call python.
  PyRun_File( file, file_name.c_str(), 0, manta_dict, manta_dict );
  
 python_finalize:
  // De-initialize python.
  Py_Finalize();
}

#endif
