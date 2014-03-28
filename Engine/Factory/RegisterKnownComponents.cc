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


#include <Engine/Display/FileDisplay.h>
#include <Engine/Display/NullDisplay.h>
#if MANTA_ENABLE_X11
#  include <Engine/Display/OpenGLDisplay.h>
#endif
#include <Engine/Factory/Factory.h>
#include <Engine/IdleModes/ZoomIdleMode.h>
#include <Engine/ImageTraversers/DeadlineImageTraverser.h>
#include <Engine/ImageTraversers/DissolveImageTraverser.h>
#include <Engine/ImageTraversers/DissolveImageTraverser.h>
#include <Engine/ImageTraversers/DissolveTiledImageTraverser.h>
#include <Engine/ImageTraversers/DissolveTiledImageTraverser.h>
#include <Engine/ImageTraversers/FilteredImageTraverser.h>
#include <Engine/ImageTraversers/NullImageTraverser.h>
#include <Engine/ImageTraversers/TiledImageTraverser.h>
#include <Engine/LoadBalancers/CyclicLoadBalancer.h>
#include <Engine/LoadBalancers/SimpleLoadBalancer.h>
#include <Engine/LoadBalancers/WQLoadBalancer.h>
#include <Engine/PixelSamplers/ClusterSampler.h>
#include <Engine/PixelSamplers/FastSampler.h>
#include <Engine/PixelSamplers/JitterSampler.h>
#include <Engine/PixelSamplers/NullSampler.h>
#include <Engine/PixelSamplers/RegularSampler.h>
#include <Engine/PixelSamplers/SingleSampler.h>
#include <Engine/Renderers/KajiyaPathtracer.h>
#include <Engine/Renderers/Moire.h>
#include <Engine/Renderers/NPREdges.h>
#include <Engine/Renderers/Noise.h>
#include <Engine/Renderers/NullRenderer.h>
#include <Engine/Renderers/RayGen.h>
#include <Engine/Renderers/Raytracer.h>
#include <Engine/Shadows/HardShadows.h>
#include <Engine/Shadows/NoShadows.h>
#include <Image/NullImage.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>
#include <Model/Cameras/EnvironmentCamera.h>
#include <Model/Cameras/FisheyeCamera.h>
#include <Model/Cameras/SphereCamera.h>
#include <Model/Cameras/OrthogonalCamera.h>
#include <Model/Cameras/PinholeCamera.h>
#include <Model/Cameras/ThinLensCamera.h>
#include <Model/Groups/Group.h>

#include <UserInterface/CameraPathAutomator.h>
#include <UserInterface/NullUI.h>
// #include <UserInterface/PromptUI.h>
#if MANTA_ENABLE_X11
#  include <UserInterface/XWindowUI.h>
#endif


// If you add a component that conditionally builds based on how the
// build is configured, please add it to
// RegisterConfigurableComponents.h.CMakeTemplate.
#include <RegisterConfigurableComponents.h>

namespace Manta {
  extern "C" void registerKnownComponents(Factory* engine)
  {
    // Register display components
    engine->registerComponent("null", &NullDisplay::create);
#if MANTA_ENABLE_X11
    engine->registerComponent("opengl", &OpenGLDisplay::create);
#endif
    engine->registerComponent("file", &FileDisplay::create);

    // Register image traversers
    engine->registerComponent("null", &NullImageTraverser::create);
    engine->registerComponent("tiled", &TiledImageTraverser::create);
    engine->registerComponent("dissolve", &DissolveImageTraverser::create);
    engine->registerComponent("dissolvetiled", &DissolveTiledImageTraverser::create);
    engine->registerComponent("filtered",&FilteredImageTraverser::create);
    engine->registerComponent("deadline", &DeadlineImageTraverser::create);

    // Register image types
    engine->registerComponent("null", &NullImage::create);
    engine->registerComponent("rgba8", &SimpleImage<RGBA8Pixel>::create);
    engine->registerComponent("rgb8", &SimpleImage<RGB8Pixel>::create);
    engine->registerComponent("abgr8", &SimpleImage<ABGR8Pixel>::create);
    engine->registerComponent("argb8", &SimpleImage<ARGB8Pixel>::create);
    engine->registerComponent("bgra8", &SimpleImage<BGRA8Pixel>::create);
    engine->registerComponent("rgbafloat", &SimpleImage<RGBAfloatPixel>::create);
    engine->registerComponent("rgbfloat", &SimpleImage<RGBfloatPixel>::create);
    engine->registerComponent("rgbzfloat", &SimpleImage<RGBZfloatPixel>::create);
    engine->registerComponent("rgba8zfloat", &SimpleImage<RGBA8ZfloatPixel>::create);

    // Register load balancers
    engine->registerComponent("cyclic", &CyclicLoadBalancer::create);
    engine->registerComponent("simple", &SimpleLoadBalancer::create);
    engine->registerComponent("workqueue", &WQLoadBalancer::create);

    // Register pixel samplers
    engine->registerComponent("fast", &FastSampler::create);
    engine->registerComponent("null", &NullSampler::create);
    engine->registerComponent("singlesample", &SingleSampler::create);
    engine->registerComponent("jittersample", &JitterSampler::create);
    engine->registerComponent("regularsample", &RegularSampler::create);
    engine->registerComponent("cluster", &ClusterSampler::create);

    // Register renderers
    engine->registerComponent("pathtracer", &KajiyaPathtracer::create);
    engine->registerComponent("null", &NullRenderer::create);
    engine->registerComponent("raygen", &RayGen::create);
    engine->registerComponent("moire", &Moire::create);
    engine->registerComponent("noise", &Noise::create);
    engine->registerComponent("raytracer", &Raytracer::create);
    engine->registerComponent("edges", &NPREdges::create);

    // Register cameras
    engine->registerComponent("environment", &EnvironmentCamera::create);
    engine->registerComponent("sphere", &SphereCamera::create);
    engine->registerComponent("pinhole", &PinholeCamera::create);
    engine->registerComponent("orthogonal", &OrthogonalCamera::create);
    engine->registerComponent("thinlens", &ThinLensCamera::create);
    engine->registerComponent("fisheye", &FisheyeCamera::create);

    // Register shadow algorithms
    engine->registerComponent("noshadows", &NoShadows::create);
    engine->registerComponent("hard", &HardShadows::create);

    // Idle modes
    engine->registerComponent("zoom", &ZoomIdleMode::create);

    // Register user interfaces
    engine->registerComponent("null", &NullUI::create);
    //     engine->registerComponent("prompt", &PromptUI::create);
#if MANTA_ENABLE_X11
    engine->registerComponent("X", &XWindowUI::create);
#endif
    engine->registerComponent("camerapath", &CameraPathAutomator::create);

    // Register groups
    engine->registerObject("group", &Group::create);

    // Register the configurable components
    RegisterConfigurableComponents(engine);
  }
}
