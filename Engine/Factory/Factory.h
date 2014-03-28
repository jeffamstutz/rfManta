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

#ifndef Manta_Engine_Control_Factory__h
#define Manta_Engine_Control_Factory__h

#include <vector>
#include <string>
#include <map>

#include <Interface/MantaInterface.h>
#include <Engine/Factory/RegisterKnownComponents.h>

namespace Manta {

  using namespace std;
  
  class Camera;
  class CameraPath;
  class Group;
  class IdleMode;
  class ImageDisplay;
  class Image;
  class ImageTraverser;
  class LoadBalancer;
  class PixelSampler;
  class Renderer;
  class Scene;
  class SetupCallback;
  class ShadowAlgorithm;
  class UserInterface;
  class RayPacket;

  class Factory {
  public:

    /////////////////////////////////////////////////////////////////////////////
    // Constructor.
    Factory( MantaInterface *interface_, bool AutoRegisterKnownComponents = true ) :
      manta_interface( interface_ ) {      
      if (AutoRegisterKnownComponents) registerKnownComponents( this );
    }
    virtual ~Factory();

    typedef vector<string> listType;
    
    // ImageDisplay
    typedef ImageDisplay* (*ImageDisplayCreator)(const vector<string>& args);
    
    virtual ImageDisplay* createImageDisplay(const string& spec) const;
    virtual void registerComponent(const string& name, ImageDisplayCreator display);
    virtual listType listImageDisplays() const;

    // ImageTraverser
    typedef ImageTraverser* (*ImageTraverserCreator)(const vector<string>& args);
    virtual void registerComponent(const string& name, ImageTraverserCreator creator);
    virtual listType listImageTraversers() const;
    virtual bool selectImageTraverser(const string& spec) const;
  
    // Image Creator.
    typedef Image* (*ImageCreator)(const vector<string>& args, bool stereo, int xres, int yres);
    virtual bool selectImageType(const string& spec) const;
    virtual void registerComponent(const string& name, ImageCreator creator);
    virtual listType listImageTypes() const;  

    // Load Balancer.
    typedef LoadBalancer* (*LoadBalancerCreator)(const vector<string>& args);
    virtual bool selectLoadBalancer(const string& spec) const;
    virtual void registerComponent(const string& name, LoadBalancerCreator creator);
    virtual listType listLoadBalancers() const;

    // Pixel Sampler
    typedef PixelSampler* (*PixelSamplerCreator)(const vector<string>& args);
    virtual bool selectPixelSampler(const string& spec) const;
    virtual void registerComponent(const string& name, PixelSamplerCreator creator);
    virtual listType listPixelSamplers() const;

    // Renderer
    typedef Renderer* (*RendererCreator)(const vector<string>& args);
    virtual bool selectRenderer(const string& spec) const;
    virtual void registerComponent(const string& name, RendererCreator creator);
    virtual listType listRenderers() const;  

    // Shadow Algorithm
    typedef ShadowAlgorithm* (*ShadowAlgorithmCreator)(const vector<string>& args);
    virtual bool selectShadowAlgorithm(const string& spec) const;
    virtual void registerComponent(const string& name, ShadowAlgorithmCreator creator);
    virtual listType listShadowAlgorithms() const;

    // Idle Modes
    typedef IdleMode* (*IdleModeCreator)(const vector<string>& args);
    MantaInterface::IdleModeHandle addIdleMode(const string& spec) const;
    virtual void registerComponent(const string& name, IdleModeCreator creator);
    virtual listType listIdleModes() const;

    // Camera
    typedef Camera* (*CameraCreator)(const vector<string>& args);
    virtual Camera* createCamera(const string& spec) const;
    virtual void registerComponent(const string& name, CameraCreator creator);
    virtual listType listCameras() const;

    // Scene Modules.
    virtual Scene *readScene(const string& sceneSpec) const;
    virtual void readStack( const string &name, const vector<string> &args ) const;
    virtual void setScenePath(const string& path);    

    // Groups
    typedef Group* (*GroupCreator)(const vector<string>& args);
    virtual Group* makeGroup(const string& groupSpec) const;
    virtual void registerObject(const string& name, GroupCreator creator);
    virtual listType listGroups() const;

    // User Interfaces
    typedef UserInterface* (*UserInterfaceCreator)(const vector<string>& args, MantaInterface *rtrt_interface);
    virtual UserInterface* createUserInterface(const string& spec);
    virtual void registerComponent(const string& name, UserInterfaceCreator creator);
    virtual listType listUserInterfaces() const;

    // Member accessors.
    void setMantaInterface( MantaInterface *manta_interface_ ) { manta_interface = manta_interface_; }
    MantaInterface *getMantaInterface() { return manta_interface; }
    
  private:
    
    // CreateImageCallback
    static void createImageCallback( bool stereo, int xres, int yres, Image *& image,
                                     ImageCreator creator,
                                     vector<string> args );
    
    // Component databases and current instances
    typedef map<string, ImageDisplayCreator> ImageDisplayMapType;
    ImageDisplayMapType imageDisplays;
		
    typedef map<string, ImageCreator> ImageCreatorMapType;
    ImageCreatorMapType imageCreators;

    vector<string> currentImageCreatorArgs;

    typedef map<string, ImageTraverserCreator> ImageTraverserMapType;
    
		ImageTraverserMapType imageTraversers;

    
		typedef map<string, LoadBalancerCreator> LoadBalancerMapType;
    LoadBalancerMapType loadBalancers;

    
		typedef map<string, PixelSamplerCreator> PixelSamplerMapType;
    PixelSamplerMapType pixelSamplers;

    
		typedef map<string, RendererCreator> RendererMapType;
    RendererMapType renderers;

    
		typedef map<string, ShadowAlgorithmCreator> ShadowAlgorithmMapType;
    ShadowAlgorithmMapType shadowAlgorithms;

    
		typedef map<string, IdleModeCreator> IdleModeMapType;
    IdleModeMapType idleModes;

    
		typedef map<string, CameraCreator> CameraMapType;
    CameraMapType cameras;
    
		typedef map<string, UserInterfaceCreator> UserInterfaceMapType;
    UserInterfaceMapType userInterfaces;
    
		typedef map<string, GroupCreator> GroupMapType;
    GroupMapType groups;

    // Scene
  
    string scenePath;
    
    Scene* readArchiveScene(const string& name, const vector<string>& args) const;
    Scene* readMOScene(const string& name, const vector<string>& args, bool printErrors) const;
    void readMOStack(const string& name, const vector<string>& args, bool printErrors ) const;

  private:

    MantaInterface *manta_interface;
  };

};
  
#endif
