
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

%module manta
// Supress stupid warning about wrapping the non const versions of functions.
#pragma SWIG nowarn=512

%include "std_string.i"
%include "std_vector.i"
%include "exception.i"

///////////////////////////////////////////////////////
// Import wrappers from mantainterface and import all objects in python.
%import mantainterface.i

%pythoncode 
%{
  from mantainterface import *
%}

///////////////////////////////////////////////////////
// Exceptions

// Wrappers for these are generated in manta.i, only
// the headers are needed here.
%{
#include <Core/Exceptions/Exception.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/BadPrimitive.h>
#include <Core/Exceptions/InvalidState.h>
#include <Core/Exceptions/UnknownColor.h>
#include <Core/Exceptions/InputError.h>
%}

///////////////////////////////////////////////////////
// ImageDisplay

%{
#include <Engine/Display/SyncDisplay.h>
//#if MANTA_ENABLE_X11
#  include <Engine/Display/OpenGLDisplay.h>
//#endif
#include <Engine/Display/PureOpenGLDisplay.h>
#include <Engine/Display/NullDisplay.h>
#include <Engine/Display/FileDisplay.h>
#include <Engine/Display/MultiDisplay.h>

#include <Engine/ImageTraversers/NullImageTraverser.h>
#include <Engine/ImageTraversers/DeadlineImageTraverser.h>
#include <Engine/ImageTraversers/TiledImageTraverser.h>
#include <Engine/ImageTraversers/DissolveImageTraverser.h>
#include <Engine/ImageTraversers/DissolveTiledImageTraverser.h>
#include <Engine/ImageTraversers/DissolveImageTraverser.h>
#include <Engine/ImageTraversers/DissolveTiledImageTraverser.h>
#include <Engine/ImageTraversers/FilteredImageTraverser.h>
#include <Engine/ImageTraversers/DeadlineImageTraverser.h>

#include <Engine/Shadows/HardShadows.h>
#include <Engine/Shadows/NoShadows.h>
%}

%manta_Release_PythonGIL(Manta::SyncDisplay::waitOnFrameReady);
%manta_Manta_Exception(Manta::PureOpenGLDisplay::PureOpenGLDisplay);
%manta_Manta_Exception(Manta::PureOpenGLDisplay::setMode);

%include <Engine/Display/SyncDisplay.h>
//#if MANTA_ENABLE_X11
%include <Engine/Display/OpenGLDisplay.h>
//#endif
%include <Engine/Display/PureOpenGLDisplay.h>
%include <Engine/Display/NullDisplay.h>
%include <Engine/Display/FileDisplay.h>
%include <Engine/Display/MultiDisplay.h>

%include <Engine/ImageTraversers/NullImageTraverser.h>
%include <Engine/ImageTraversers/DeadlineImageTraverser.h>
%include <Engine/ImageTraversers/TiledImageTraverser.h>
%include <Engine/ImageTraversers/DissolveImageTraverser.h>
%include <Engine/ImageTraversers/DissolveTiledImageTraverser.h>
%include <Engine/ImageTraversers/DissolveImageTraverser.h>
%include <Engine/ImageTraversers/DissolveTiledImageTraverser.h>
%include <Engine/ImageTraversers/FilteredImageTraverser.h>
%include <Engine/ImageTraversers/DeadlineImageTraverser.h>

%include <Engine/Shadows/HardShadows.h>
%include <Engine/Shadows/NoShadows.h>

///////////////////////////////////////////////////////
// UI
%{
#include <UserInterface/AutomatorUI.h>
#include <UserInterface/SyncFrameAutomator.h>
#include <UserInterface/CameraPathAutomator.h>
%}

%include <UserInterface/AutomatorUI.h>
%include <UserInterface/SyncFrameAutomator.h>
%include <UserInterface/CameraPathAutomator.h>

////////////////////////////////////////////////////////
// Lights and backgrounds
%{
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Lights/PointLight.h>
#include <Model/Lights/HeadLight.h>
%}

%include <Model/AmbientLights/ConstantAmbient.h>
%include <Model/Backgrounds/ConstantBackground.h>
%include <Model/Lights/PointLight.h>
%include <Model/Lights/HeadLight.h>

namespace Manta {

  // We used to be able to do this when Light was in the same
  // interface as these.  Now we have to extend new lights.

//   %extend Light {
//     Manta::PointLight* asPointLight() {
//       return dynamic_cast<Manta::PointLight*>(self);
//     }
//   };

//   %extend Light {
//     Manta::HeadLight* asHeadLight() {
//       return dynamic_cast<Manta::HeadLight*>(self);
//     }
//   };

  %extend PointLight {
    static PointLight* fromLight(Light* parent) {
      return dynamic_cast<Manta::PointLight*>(parent);
    }
  };

  %extend HeadLight {
    static HeadLight* fromLight(Light* parent) {
      return dynamic_cast<Manta::HeadLight*>(parent);
    }
  };
}

/////////////////////////////////////////////////////
// Image stuff
%{
#include <Image/TGAFile.h>
%}

%include <Image/TGAFile.h>

/////////////////////////////////////////////////////
// Textures
%{
#include <Model/Textures/Constant.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Textures/TileTexture.h>
#include <Model/Textures/ImageTexture.h>
#include <Model/Textures/MarbleTexture.h>
#include <Model/Textures/CloudTexture.h>
%}

%include <Model/Textures/Constant.h>
%include <Model/Textures/CheckerTexture.h>
%include <Model/Textures/TileTexture.h>
%include <Model/Textures/ImageTexture.h>
%include <Model/Textures/MarbleTexture.h>
%include <Model/Textures/CloudTexture.h>

namespace Manta {
  // Textures.  If you add a new texture like FunkyTexture<MagicType>,
  // you also need to add a template for Texture<MagicType> to reduce
  // warnings.

  // Turn off warning on previosly wrapped templates.  This affects
  // things when you have Manta::Real == Manta::ColorComponent.
  %warnfilter(404) Texture<Manta::ColorComponent>;
  %warnfilter(404) CheckerTexture<Manta::ColorComponent>;
  %warnfilter(404) TileTexture<Manta::ColorComponent>;
  
  %template(Texture_Color) Texture<Color>;
  %template(Texture_Real) Texture<Manta::Real>;
  %template(Texture_ColorComponent) Texture<Manta::ColorComponent>; 

  %template(CheckerTexture_Color) CheckerTexture<Color>;
  %template(CheckerTexture_Real) CheckerTexture<Manta::Real>;
  %template(CheckerTexture_ColorComponent) CheckerTexture<Manta::ColorComponent>;
  %template(TileTexture_Color) TileTexture<Color>;
  %template(TileTexture_Real) TileTexture<Manta::Real>;
  %template(TileTexture_ColorComponent) TileTexture<Manta::ColorComponent>;
  %template(Constant_Color) Constant<Color>;
  %template(ImageTexture_Color) ImageTexture<Manta::Color>;
  %template(MarbleTexture_Color) MarbleTexture<Manta::Color>;
  %template(CloudTexture_Color) CloudTexture<Manta::Color>;
  
}

/////////////////////////////////////////////////////
// Materials and Primitivs
%{
// #include <Model/Materials/OpaqueShadower.h>
// #include <Model/Materials/LitMaterial.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/OrenNayar.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/ThinDielectric.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Transparent.h>
//#include <Model/Materials/AmbientOcclusion.h>
#include <Model/Primitives/PrimitiveCommon.h>
#include <Model/Primitives/Cylinder.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Plane.h>
#include <Model/Primitives/PrimaryRaysOnly.h>
#include <Model/Primitives/Ring.h>
#include <Model/Primitives/Sphere.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Model/Primitives/BumpPrimitive.h>
#include <Model/Primitives/MeshTriangle.h>
%}

//%include <Model/Materials/OpaqueShadower.h>
// %include <Model/Materials/LitMaterial.h>
%include <Model/Materials/CopyTextureMaterial.h>
%include <Model/Materials/Phong.h>
%include <Model/Materials/Lambertian.h>
%include <Model/Materials/OrenNayar.h>
%include <Model/Materials/MetalMaterial.h>
%include <Model/Materials/Dielectric.h>
%include <Model/Materials/ThinDielectric.h>
%include <Model/Materials/Flat.h>
%include <Model/Materials/Transparent.h>
//%include <Model/Materials/AmbientOcclusion.h>
%include <Model/Primitives/PrimitiveCommon.h>
%include <Model/Primitives/Cylinder.h>
%include <Model/Primitives/Cube.h>
%include <Model/Primitives/Parallelogram.h>
%include <Model/Primitives/Plane.h>
%include <Model/Primitives/PrimaryRaysOnly.h>
%include <Model/Primitives/Ring.h>
%include <Model/Primitives/Sphere.h>
%include <Model/TexCoordMappers/UniformMapper.h>
%include <Model/Primitives/BumpPrimitive.h>
%include <Model/Primitives/MeshTriangle.h>

///////////////////////////////////////////////////////////////////////////////
// Groups

%{
#include <Model/Groups/Group.h>
#include <Model/Textures/NormalTexture.h>
%}

%include <Model/Groups/Group.h>
%include <Model/Textures/NormalTexture.h>

%{
#include <Model/Groups/KDTree.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Mesh.h>
#include <Model/Groups/ObjGroup.h>
%}

//TODO: these are quick fixes, should actually fix these functions with SWIG  -Carson
%ignore Manta::DynBVH::firstIntersects;
%ignore Manta::DynBVH::finishUpdate;
%ignore Manta::DynBVH::parallelTopDownUpdate;
%ignore Manta::DynBVH::beginParallelBuild;
%ignore Manta::DynBVH::beginParallelUpdate;
%ignore Manta::DynBVH::parallelTopDownBuild;
%ignore Manta::DynBVH::parallelApproximateSAH;
%ignore Manta::DynBVH::parallelComputeBounds;
%ignore Manta::DynBVH::parallelComputeBoundsReduction;
%ignore Manta::DynBVH::parallelComputeBins;
%ignore Manta::DynBVH::parallelComputeBinsReduction;
%ignore Manta::DynBVH::splitBuild;
%ignore Manta::DynBVH::beginParallelPreprocess;
%ignore Manta::DynBVH::parallelPreprocess;
%ignore Manta::DynBVH::finishParallelPreprocess;

%include <Model/Groups/KDTree.h>
%include <Model/Groups/DynBVH.h>
%include <Model/Groups/Mesh.h>
%include <Model/Groups/ObjGroup.h>

/////////////////////////////////////////////////
// GLM.
%{
#include <Model/Readers/glm/glm.h>
%}

%include <Model/Readers/glm/glm.h>

///////////////////////////////////////////////////////
// Engine Factory
%{
#include <Engine/Factory/Factory.h>
%}
%include <Engine/Factory/Factory.h>

///////////////////////////////////////////////////////
// Engine Control
%{
#include <Engine/Control/RTRT.h>
%}

%include <Engine/Control/RTRT.h>

////////////////////////////////////////////////
// Pixel Samplers
%{
#include <Engine/PixelSamplers/JitterSampler.h>
#include <Engine/PixelSamplers/SingleSampler.h>
#include <Engine/PixelSamplers/TimeViewSampler.h>
%}

%include <Engine/PixelSamplers/JitterSampler.h>
%include <Engine/PixelSamplers/SingleSampler.h>
%include <Engine/PixelSamplers/TimeViewSampler.h>

namespace Manta {
  %extend JitterSampler {
    static JitterSampler* fromPixelSampler(PixelSampler* parent) {
      return dynamic_cast<Manta::JitterSampler*>(parent);
    }
  };
  %extend SingleSampler {
    static SingleSampler* fromPixelSampler(PixelSampler* parent) {
      return dynamic_cast<Manta::SingleSampler*>(parent);
    }
  };
  %extend TimeViewSampler {
    static TimeViewSampler* fromPixelSampler(PixelSampler* parent) {
      return dynamic_cast<Manta::TimeViewSampler*>(parent);
    }
  };
} // end namespace Manta
