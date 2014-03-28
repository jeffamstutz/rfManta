/* manta.i */
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

%module mantainterface
// Supress stupid warning about wrapping the non const versions of functions.
#pragma SWIG nowarn=512

%include "std_string.i"
%ignore std::vector::begin() const;
%ignore std::vector::end() const;
%ignore std::vector::rbegin() const;
%ignore std::vector::rend() const;
%include "std_vector.i"
%include "exception.i"

 // Ignore readwrite functions for now
%ignore *::readwrite;

// Some classes complain about not knowing about MantaRTTI.  I don't
// want to wrap the whole file, because I'm frightened by dealing with
// it.  For now, I'll create a dummy class that has no methods or data.
namespace Manta {
  template<class T>
  class MantaRTTI {};
}

%{
#include <MantaTypes.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/VectorT.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Geometry/BBox.h>
%}

%include <MantaTypes.h>
%include <Core/Geometry/Vector.h>
%include <Core/Geometry/VectorT.h>
%include <Core/Geometry/AffineTransform.h>
%include <Core/Geometry/BBox.h>

namespace Manta {
  %template(VectorT_Real3) VectorT<Manta::Real, 3>;
}

///////////////////////////////////////////////////////
// Exceptions

%{
#include <Core/Exceptions/Exception.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/BadPrimitive.h>
#include <Core/Exceptions/InvalidState.h>
#undef InvalidState
#include <Core/Exceptions/InputError.h>
%}

%include <Core/Exceptions/Exception.h>
%include <Core/Exceptions/IllegalArgument.h>
%include <Core/Exceptions/BadPrimitive.h>
%include <Core/Exceptions/InvalidState.h>
%include <Core/Exceptions/InputError.h>

%define %manta_Exception_exception(Function)
%exception Function {
  try {
    $action
  } catch(Manta::Exception& e) {
    SWIG_exception(SWIG_ValueError, e.message());
  } catch(...) { 
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}
%enddef


%define %manta_IllegalArgument_exception(Function)
%exception Function {
  try {
    $action
  } catch(Manta::IllegalArgument& e) {
    SWIG_exception(SWIG_ValueError, e.message());
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}
%enddef

%define %manta_Manta_Exception(Function)
%exception Function {
  try {
    $action
  } catch(Manta::Exception& e) {
    SWIG_exception(SWIG_ValueError, e.message());
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}
%enddef

%define %manta_IllegalArgument_BadPrimitive_exception(Function)
%exception Function {
  try {
    $action
  } catch(Manta::IllegalArgument& e) {
    SWIG_exception(SWIG_ValueError, e.message());
  } catch(Manta::BadPrimitive& e) {
    static char exception_message[1024];
    sprintf(exception_message, "BadPrimitive: \"%s\"", e.message());
    SWIG_exception(SWIG_ValueError, exception_message);
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}
%enddef

// This will unlock the Python Global Interpreter Lock (GIL) before
// calling a potentially blocking function, and then lock it before
// continuing.
%define %manta_Release_PythonGIL(Function)
%exception Function {
  PyThreadState *_save = PyEval_SaveThread();
  try {
    $action
    PyEval_RestoreThread(_save);
  } catch(...) {
    PyEval_RestoreThread(_save);
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}
%enddef

///////////////////////////////////////////////////////
//

%{
#include <Core/Geometry/Ray.h>
#include <Core/Util/CallbackHandle.h>
#include <Interface/Context.h>
#include <Interface/IdleMode.h>
#include <Interface/Clonable.h>
#include <Interface/Interpolable.h>
#include <Interface/Object.h>
#include <Interface/Packet.h>
#include <Interface/Primitive.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Util/Align.h>
#include <Core/Util/About.h>
#include <Interface/Image.h>
#include <Interface/RayPacket.h>
#include <Interface/AccelerationStructure.h>
#include <Interface/ShadowAlgorithm.h>
%}

%include <Core/Geometry/Ray.h>
%include <Core/Util/CallbackHandle.h>
%include <Interface/Context.h>
%include <Interface/IdleMode.h>
%include <Interface/Clonable.h>
%include <Interface/Interpolable.h>
%include <Interface/Object.h>
%include <Core/Util/Align.h>
%include <Core/Util/About.h>
%ignore Manta::Image::set;
%ignore Manta::Image::get;
%include <Interface/Image.h>
%include <Interface/Packet.h> // moved up here for Primitive::getRandomPoints
%include <Interface/Primitive.h>
%include <Interface/TexCoordMapper.h>
%ignore Manta::RayPacket::getOrigin(int, int) const;
%ignore Manta::RayPacket::getDirection(int, int) const;
%ignore Manta::RayPacket::getInverseDirection(int, int) const;
%include <Interface/RayPacket.h>
%include <Interface/AccelerationStructure.h>
%include <Interface/ShadowAlgorithm.h>


%manta_IllegalArgument_exception(create);

%{

#include <Interface/UserInterface.h>
#include <Interface/RenderParameters.h>
#include <Interface/Scene.h>
#include <Core/Util/CallbackHelpers.h>
#include <Core/Util/Callback.h>
#include <SwigInterface/manta.h>
#include <Interface/MantaInterface.h>
#include <Interface/TValue.h>
#include <Interface/FrameState.h>
%}

namespace std {
  %template(vectorStr) vector<string>;

  
};

// This will unlock the GIL for potentially locking functions
%manta_Release_PythonGIL(Manta::MantaInterface::addTransaction);
%manta_Release_PythonGIL(Manta::MantaInterface::blockUntilFinished);

%include <Interface/UserInterface.h>
 
%include <Interface/RenderParameters.h>

namespace Manta {
   class BasicCameraData;
}

%ignore Manta::Scene::getObject() const;
%ignore Manta::Scene::getBackground() const;
%ignore Manta::Scene::getLights() const;
%ignore Manta::Scene::getRenderParameters() const;
%include <Interface/Scene.h>

%include <Core/Util/CallbackHelpers.h>
%include <Core/Util/Callback.h>
%include <Interface/Transaction.h>

// Ignore some operators that swig doesn't like
%ignore Manta::TValue::operator=;
%ignore Manta::TValue::operator T; // This needs to be 'operator T', so it matches the function header.
%include <Interface/TValue.h>
%include <Interface/FrameState.h>
%include <SwigInterface/manta.h>
%include <Interface/MantaInterface.h>

namespace Manta {
  using namespace std;

  class Camera;

  %template(TValueInt) TValue<int>;
}

extern Manta::Scene* createDefaultScene();

%{
  extern Manta::Scene* createDefaultScene();
%}

%exception;

/////////////////////////////////////////////////
// Model stuff

%{
#include <Core/Exceptions/UnknownColor.h>
%}

%include <Core/Exceptions/UnknownColor.h>

%exception Manta::ColorDB::getNamedColor {
  try {
    $action
  } catch(Manta::UnknownColor& e) {
    static char exception_message[1024];
    sprintf(exception_message, "Unknown color: \"%s\"", e.message());
    SWIG_exception(SWIG_ValueError, exception_message);
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError, "Unknown exception");
  }
}


/////////////////////////////////////////////////
// The Array Containers
%{
#include <Core/Containers/Array1.h>
%}

// Wrap the class here to get a customized interface to the class.
namespace Manta {
  template<class T>
  class Array1 {
  public:
    Array1();
    void add(const T&);
    T* get_objs();
    int size();
    %extend {
      T __getitem__( int i ) { return self->operator[](i); }
      void __setitem__(int i,T val) { self->operator[](i) = val; }
      int __len__ () { return self->size(); }
    }
  };
}
%template(Array1_ObjectP) Manta::Array1<Manta::Object*>;

%{
#include <MantaTypes.h>
#include <Core/Color/RGBColor.h>
#include <Core/Color/Spectrum.h>
#include <Core/Color/RGBTraits.h>
#include <Core/Color/GrayColor.h>
#include <Core/Color/Spectrum.h>
#include <Core/Color/Conversion.h>
#include <Core/Color/ColorSpace.h>
#include <Core/Color/ColorSpace_fancy.h>
#include <Core/Color/ColorDB.h>
#include <Core/Color/RegularColorMap.h>
%}

%include <MantaTypes.h>
%include <Core/Color/RGBColor.h>
%include <Core/Color/Spectrum.h>
%include <Core/Color/RGBTraits.h>
%include <Core/Color/GrayColor.h>
%include <Core/Color/Spectrum.h>
%include <Core/Color/Conversion.h>
%include <Core/Color/ColorSpace.h>
%include <Core/Color/ColorSpace_fancy.h>
%include <Core/Color/ColorDB.h>

%template(Array1_Color) Manta::Array1<Manta::Color>;
%include <Core/Color/RegularColorMap.h>


namespace Manta {
  //  typedef ColorSpace<RGBTraits> Color;
  %template(Color) ColorSpace<RGBTraits>;

  %extend ColorSpace<RGBTraits> {
    %newobject __str__;
    char* __str__() {
      return strdup(self->toString().c_str());
    }
    ColorSpace<RGBTraits> scaled(double val) {
      return self->operator*(val);
    }
  };
  
  
  %extend RGBColor {
    %newobject __str__;
    char* __str__() {
      return strdup(self->toString().c_str());
    }
  };
  %extend GrayColor {
    %newobject __str__;
    char* __str__() {
      return strdup(self->toString().c_str());
    }

  };
  %extend Spectrum {
    %newobject __str__;
    char* __str__() {
      return strdup(self->toString().c_str());
    }
  };
  
}

///////////////////////////////////////////////////////
// Cameras
%{
#include <Interface/Camera.h>
%}

%include <Interface/Camera.h>

///////////////////////////////////////////////////////
// ImageDisplay
typedef int Window;

%{
#include <Interface/ImageDisplay.h>
%}

%include <Interface/ImageDisplay.h>

///////////////////////////////////////////////////////
// UI
%{
#include <Interface/UserInterface.h>
%}

%include <Interface/UserInterface.h>


////////////////////////////////////////////////////////
// Lights and backgrounds
%{
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/AmbientLight.h>
#include <Interface/Background.h>
%}

%include <Interface/Light.h>
%ignore Manta::LightSet::getLight(int) const;
%include <Interface/LightSet.h>
%include <Interface/AmbientLight.h>
%include <Interface/Background.h>

namespace Manta {
  // This tells SWIG to deallocate the memory from toString functions.
  //  %newobject *::toString();
  %extend LightSet {
    %newobject __str__;
    char* __str__() {
      return strdup(self->toString().c_str());
    }
  };
  
  %extend AmbientLight {
    %newobject __str__;
    char* __str__() {
      return strdup(self->toString().c_str());
    }
  };

}

/////////////////////////////////////////////////////
// Image stuff

/////////////////////////////////////////////////////
// Textures
%{
#include <Interface/Texture.h>
%}

%include <Interface/Texture.h>


/////////////////////////////////////////////////////
// Materials and Primitivs
%{
#include <Interface/Material.h>
#include <Interface/TexCoordMapper.h>
#include <Model/Materials/OpaqueShadower.h>
#include <Model/Materials/LitMaterial.h>
%}

%include <Interface/Material.h>
%include <Interface/TexCoordMapper.h>
%include <Model/Materials/OpaqueShadower.h>
%include <Model/Materials/LitMaterial.h>

///////////////////////////////////////////////////////////////////////////////
// Groups

%{
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
%}
%ignore Manta::Group::get(size_t) const;
%include <Model/Groups/Group.h>
%include <Model/Groups/Mesh.h>


/////////////////////////////////////////////////
// GLM.

/////////////////////////////////////////////////
// The Array Containers

///////////////////////////////////////////////////////
// Engine Components

%{
#include <Interface/PixelSampler.h>
#include <Interface/ImageTraverser.h>
%}

%include <Interface/PixelSampler.h>
%include <Interface/ImageTraverser.h>

////////////////////////////////////////////////
// Thread stuff
%{
#include <Core/Thread/Thread.h>
#include <Core/Thread/Runnable.h>
#include <Core/Thread/Semaphore.h>
%}

// yield is a "future" reserved word for some versions of python.
// This renames it to be something else.
%rename(yieldProcess) yield;

%include <Core/Thread/Thread.h>

%rename(yield) yield;

%manta_Release_PythonGIL(Manta::Semaphore::down);
%include <Core/Thread/Semaphore.h>

namespace Manta {
  // We are going to wrap these classes here to avoid having to wrap
  // anything more related to Runnable.
  class Runnable
  {
  protected:
    virtual void run()=0; // pure virtual means no constructor
    virtual ~Runnable();  // protected virtual destructor, means no calls to destroy it.
  };
} // end namespace Manta


////////////////////////////////////////////////
// Python specific code

%pythoncode %{

def manta_new(obj):
    obj.thisown = 0
    return obj

def manta_delete(obj):
    obj.thisown = 1
    return obj  
%}


