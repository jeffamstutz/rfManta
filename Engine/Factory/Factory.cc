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

#include <Engine/Factory/Factory.h>

#include <Core/Containers/StringUtil.h>
#include <Core/Exceptions/UnknownComponent.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Persistent/Archive.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Util/Args.h>
#include <Core/Util/Plugin.h>
#include <Engine/Factory/RegisterKnownComponents.h>
#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/Scene.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>



using namespace Manta;

Factory::~Factory()
{
}

ImageDisplay *Factory::createImageDisplay(const string& spec ) const
{
  string name;
  vector<string> args;

  // Parse the arg string.
  parseSpec(spec, name, args);

  // Search for an image display creator with the name.
  ImageDisplayMapType::const_iterator iter = imageDisplays.find(name);
  if(iter == imageDisplays.end())
    throw UnknownComponent( "Image display not found", spec );

  return (*iter->second)(args);
}

void Factory::registerComponent(const string& name, ImageDisplayCreator fun)
{
  ImageDisplayMapType::iterator iter = imageDisplays.find(name);
  if(iter != imageDisplays.end())
    throw IllegalValue<string>("Duplicate ImageDisplay component", name);
  imageDisplays[name] = fun;
}

Factory::listType Factory::listImageDisplays() const
{
  Factory::listType list;
  for(ImageDisplayMapType::const_iterator iter = imageDisplays.begin();
      iter != imageDisplays.end(); iter++)
    list.push_back(iter->first);
  return list;
}


bool Factory::selectImageType(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);

  ImageCreatorMapType::const_iterator iter = imageCreators.find(name);
  if(iter == imageCreators.end())
    return 0;

  ImageCreator creator = iter->second;

  // Create a static method callback.
  manta_interface->setCreateImageCallback( Callback::create( &Factory::createImageCallback,
                                                             creator,
                                                             args ) );

  return true;
}

void Factory::createImageCallback( bool stereo, int xres, int yres, Image *& image, ImageCreator creator, vector<string> args ) {

  // Invoke the legacy image creator.
  image = (creator)(args, stereo, xres, yres);
}

void Factory::registerComponent(const string& name, ImageCreator fun)
{
  ImageCreatorMapType::iterator iter = imageCreators.find(name);
  if(iter != imageCreators.end())
    throw IllegalValue<string>("Duplicate ImageCreator component", name);
  imageCreators[name] = fun;
}

Factory::listType Factory::listImageTypes() const
{
  Factory::listType list;
  for(ImageCreatorMapType::const_iterator iter = imageCreators.begin();
      iter != imageCreators.end(); iter++)
    list.push_back(iter->first);
  return list;
}

bool Factory::selectImageTraverser(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  ImageTraverserMapType::const_iterator iter = imageTraversers.find(name);
  if(iter == imageTraversers.end())
    return false;

  manta_interface->setImageTraverser( (*iter->second)(args) );

  return true;
}

void Factory::registerComponent(const string& name, ImageTraverserCreator fun)
{
  ImageTraverserMapType::iterator iter = imageTraversers.find(name);
  if(iter != imageTraversers.end())
    throw IllegalValue<string>("Duplicate ImageTraverser component", name);
  imageTraversers[name] = fun;
}

Factory::listType Factory::listImageTraversers() const
{
  Factory::listType list;
  for(ImageTraverserMapType::const_iterator iter = imageTraversers.begin();
      iter != imageTraversers.end(); iter++)
    list.push_back(iter->first);
  return list;
}

bool Factory::selectLoadBalancer(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  LoadBalancerMapType::const_iterator iter = loadBalancers.find(name);
  if(iter == loadBalancers.end())
    return false;

  manta_interface->setLoadBalancer((*iter->second)(args));

  return true;
}

void Factory::registerComponent(const string& name, LoadBalancerCreator fun)
{
  LoadBalancerMapType::iterator iter = loadBalancers.find(name);
  if(iter != loadBalancers.end())
    throw IllegalValue<string>("Duplicate LoadBalancer component", name);
  loadBalancers[name] = fun;
}

Factory::listType Factory::listLoadBalancers() const
{
  Factory::listType list;
  for(LoadBalancerMapType::const_iterator iter = loadBalancers.begin();
      iter != loadBalancers.end(); iter++)
    list.push_back(iter->first);
  return list;
}



bool Factory::selectPixelSampler(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  PixelSamplerMapType::const_iterator iter = pixelSamplers.find(name);
  if(iter == pixelSamplers.end())
    return false;

  manta_interface->setPixelSampler( (*iter->second)(args) );

  return true;
}



void Factory::registerComponent(const string& name, PixelSamplerCreator fun)
{
  PixelSamplerMapType::iterator iter = pixelSamplers.find(name);
  if(iter != pixelSamplers.end())
    throw IllegalValue<string>("Duplicate PixelSampler component", name);
  pixelSamplers[name] = fun;
}

Factory::listType Factory::listPixelSamplers() const
{
  Factory::listType list;
  for(PixelSamplerMapType::const_iterator iter = pixelSamplers.begin();
      iter != pixelSamplers.end(); iter++)
    list.push_back(iter->first);
  return list;
}


bool Factory::selectRenderer(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  RendererMapType::const_iterator iter = renderers.find(name);
  if(iter == renderers.end())
    return false;

  manta_interface->setRenderer( (*iter->second)(args) );

  return true;
}

void Factory::registerComponent(const string& name, RendererCreator fun)
{
  RendererMapType::iterator iter = renderers.find(name);
  if(iter != renderers.end())
    throw IllegalValue<string>("Duplicate Renderer component", name);
  renderers[name] = fun;
}

Factory::listType Factory::listRenderers() const
{
  Factory::listType list;
  for(RendererMapType::const_iterator iter = renderers.begin();
      iter != renderers.end(); iter++)
    list.push_back(iter->first);
  return list;
}

MantaInterface::IdleModeHandle Factory::addIdleMode(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  IdleModeMapType::const_iterator iter = idleModes.find(name);
  if(iter == idleModes.end())
    return (MantaInterface::IdleModeHandle)-1;

  return manta_interface->addIdleMode( (*iter->second)(args) );
}

void Factory::registerComponent(const string& name, IdleModeCreator fun)
{
  IdleModeMapType::iterator iter = idleModes.find(name);
  if(iter != idleModes.end())
    throw IllegalValue<string>("Duplicate IdleMode component", name);
  idleModes[name] = fun;
}

Factory::listType Factory::listIdleModes() const
{
  Factory::listType list;
  for(IdleModeMapType::const_iterator iter = idleModes.begin();
      iter != idleModes.end(); iter++)
    list.push_back(iter->first);
  return list;
}


bool Factory::selectShadowAlgorithm(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  ShadowAlgorithmMapType::const_iterator iter = shadowAlgorithms.find(name);
  if(iter == shadowAlgorithms.end())
    return false;
  manta_interface->setShadowAlgorithm(((*iter->second)(args)));
  return true;
}

void Factory::registerComponent(const string& name, ShadowAlgorithmCreator fun)
{
  ShadowAlgorithmMapType::iterator iter = shadowAlgorithms.find(name);
  if(iter != shadowAlgorithms.end())
    throw IllegalValue<string>("Duplicate ShadowAlgorithm component", name);
  shadowAlgorithms[name] = fun;
}

Factory::listType Factory::listShadowAlgorithms() const
{
  Factory::listType list;
  for(ShadowAlgorithmMapType::const_iterator iter = shadowAlgorithms.begin();
      iter != shadowAlgorithms.end(); iter++)
    list.push_back(iter->first);
  return list;
}

Camera* Factory::createCamera(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  CameraMapType::const_iterator iter = cameras.find(name);
  if(iter == cameras.end()){
    return 0;
  }
  return (*iter->second)(args);
}

void Factory::registerComponent(const string& name, CameraCreator fun)
{
  CameraMapType::iterator iter = cameras.find(name);
  if(iter != cameras.end())
    throw IllegalValue<string>("Duplicate Camera component", name);
  cameras[name] = fun;
}

Factory::listType Factory::listCameras() const
{
  Factory::listType list;
  for(CameraMapType::const_iterator iter = cameras.begin();
      iter != cameras.end(); iter++)
    list.push_back(iter->first);
  return list;
}


Scene *Factory::readScene(const string& spec) const
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);

  // Pull off the suffix...
  string::size_type dot = name.rfind('.');
  string suffix;
  if(dot == string::npos){
    suffix="";
  } else {
    suffix = name.substr(dot+1);
  }
  Scene* newScene = 0;

  // These are to try and guess what the shared library extension is
  // on various systems.
  string system_suffix;
#if defined (__APPLE__)
  system_suffix = "dylib";
#elif defined (_WIN32) || defined (__CYGWIN__)
  system_suffix = "dll";
#else
  system_suffix = "so";
#endif

  if(suffix == "rtml" || suffix == "xml"){
    newScene = readArchiveScene(name, args);
  } else if((suffix == "mo") || (suffix == "so") || (suffix == "dylib") ||
     (suffix == "dll")) {
    // Do this twice - once silently and once printing errors
    newScene = readMOScene(name, args, true);
    // if(!newScene)
    //   readMOScene(name, args, true);
  } else {
    // Try reading it as an MO
    newScene = readMOScene(name+"."+system_suffix, args, false);
  }
  if(!newScene){
    return 0;
  } else {
    manta_interface->setScene( newScene );
    return newScene;
  }
}

void Factory::readStack(const string &name, const vector<string> &args ) const
{
  readMOStack(name, args, true);
}

void Factory::setScenePath(const string& path)
{
  scenePath = path;
}

Group* Factory::makeGroup(const string& groupSpec) const
{
  string name;
  vector<string> args;
  parseSpec(groupSpec, name, args);
  GroupMapType::const_iterator iter = groups.find(name);
  if(iter == groups.end())
    return 0;
  Group* group = (*iter->second)(args);
  return group;
}

void Factory::registerObject(const string& name, GroupCreator creator)
{
  GroupMapType::iterator iter = groups.find(name);
  if(iter != groups.end())
    throw IllegalValue<string>("Duplicate group", name);
  groups[name] = creator;
}

Factory::listType Factory::listGroups() const
{
  Factory::listType list;
  for(GroupMapType::const_iterator iter = groups.begin();
      iter != groups.end(); iter++)
    list.push_back(iter->first);
  return list;
}

Scene* Factory::readArchiveScene(const string& name, const vector<string>& args) const
{
  // Determine which directories to search for the scene library.
  string fullname = name;
  vector<string> dirs = split_string(scenePath, ':');
  for(vector<string>::iterator dir = dirs.begin(); dir != dirs.end(); dir++){

    fullname = *dir + "/" + name;

    // Check to see if the file exists.
    struct stat statbuf;
    if(stat(fullname.c_str(), &statbuf) == 0){
      break;
    }
  }

  Archive* archive = Archive::openForReading(fullname);
  if(!archive)
    throw InputError("Could not open Archive: " + fullname);

  ArchiveElement* root = archive->getRoot();
  if(!root)
    throw InputError("Cannot find scene in file: " + fullname);
  Scene* scene = 0;
  root->readwrite("scene", scene);
  if(!scene)
    throw InputError("Error reading file: " + fullname);
  return scene;
}

Scene* Factory::readMOScene(const string& name, const vector<string>& args,
                            bool printErrors) const
{
  // Determine which directories to search for the scene library.
  string fullname = name;
  vector<string> dirs = split_string(scenePath, ':');
  for(vector<string>::iterator dir = dirs.begin(); dir != dirs.end(); dir++){

    fullname = *dir + "/" + name;

    // Check to see if the file exists.
    struct stat statbuf;
    if(stat(fullname.c_str(), &statbuf) == 0){
      break;
    }
  }

  typedef Scene* (*MakerType)(const ReadContext&, const vector<string>&);
  Plugin<MakerType>* scene_file = new Plugin<MakerType>(fullname.c_str());

  if (!scene_file->loadSymbol("make_scene")) {
    return NULL;
  }

  MakerType make_scene = scene_file->function;
  // Call the make_scene function.
  ReadContext context(manta_interface);
  Scene* scene=(*make_scene)(context, args);
  if(!scene){
    delete scene_file;
    throw InputError( "make_scene returned null" );
  }
  // NOTE(boulos): Don't delete scene_file here because we need the
  // symbols to stay around if they're not linked into the binary
  // itself. (like a class defined in an external module)

  return scene;

}

///////////////////////////////////////////////////////////////////////////////
// This dynamically loads code to configure the manta rendering stack.
void Factory::readMOStack(const string& name, const vector<string>& args, bool printErrors ) const
{

  // Assume an absolute path by default
  string fullname = name;

  vector<string> dirs = split_string(scenePath, ':');
  for(vector<string>::iterator dir = dirs.begin(); dir != dirs.end(); dir++){

    fullname = *dir + "/"+name;

    // Check to see if the file exists in the directory.
    struct stat statbuf;
    if(stat(fullname.c_str(), &statbuf) != 0){
      break;
    }
  }

  typedef void (*MakerType)(ReadContext &, const vector<string>&);
  Plugin<MakerType>* make_stack = new Plugin<MakerType>(fullname.c_str());
  if (!make_stack->loadSymbol("make_stack")) {
    if(printErrors){
      cerr << "Stack configuration file found, but make_stack() function not found\n";
    }
    delete make_stack;
    return;
  }
  ReadContext context(manta_interface);
  MakerType make_stack_function = make_stack->function;
  // Call the function.
  (*make_stack_function)(context, args);
}

UserInterface* Factory::createUserInterface(const string& spec)
{
  string name;
  vector<string> args;
  parseSpec(spec, name, args);
  UserInterfaceMapType::iterator iter = userInterfaces.find(name);
  if(iter == userInterfaces.end())
    return 0;
  // This will create a new UserInterface
  UserInterface *ui = (*iter->second)(args, manta_interface);
  return ui;
}

void Factory::registerComponent(const string& name, UserInterfaceCreator creator)
{
  // Allow components to be re-registered.
  // UserInterfaceMapType::iterator iter = userInterfaces.find(name);
  // if(iter != userInterfaces.end())
  //   throw IllegalValue<string>("Duplicate User Interface component", name);
  userInterfaces[name] = creator;
}

Factory::listType Factory::listUserInterfaces() const
{
  Factory::listType list;
  for(UserInterfaceMapType::const_iterator iter = userInterfaces.begin();
      iter != userInterfaces.end(); iter++)
    list.push_back(iter->first);
  return list;
}
