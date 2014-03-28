
// Manta scene for grid-based isosurface volume rendering
// Aaron Knoll
// knolla@sci.utah.edu
// May 2006


#include <Core/Containers/Array3.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>
#include <Core/Thread/Time.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/MantaInterface.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Cameras/PinholeCamera.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/HeadLight.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/Phong.h>
#include <Model/MiscObjects/CuttingPlane.h>
#include <Model/Primitives/IsosurfaceGridVolume.h>
#include <Model/Primitives/OctreeVolume.h>

using namespace Manta;

enum CuttingPlaneType { CUTTING_NONE, CUTTING_DEFAULT, CUTTING_SPECIFIED };

///////////////////////////////////////////////////////////////////////////
MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext& context, const vector<string>& args) {

  string filename = "";
  int macrocells = 3;
  double isovalue = 20;

  Vector plane_point;
  Vector plane_normal;

  // NOTE(boulos): As of 29-Jul-2007 this was unused.
  //CuttingPlaneType cutting_type = CUTTING_NONE;

  // Parse args.i
  for (size_t i=0;i<args.size();++i) {
    if (args[i] == "-file") {
      if (!getStringArg(i, args, filename))
        throw IllegalArgument("gridisovol -file <filename>", i, args);
    }
    else if (args[i] == "-macrocells") {
      if (!getIntArg(i, args, macrocells))
        throw IllegalArgument("octisovol -macrocells <#macrocells>", i, args);
    }
    else if (args[i] == "-isovalue") {
      if (!getDoubleArg(i, args, isovalue))
        throw IllegalArgument("octisovol -isovalue <isovalue>", i, args);
    }
    else {
      cerr << "Read built grid volume:" << endl;
      cerr << "-file <filename>"             << endl;
      cerr << "-macrocells <#macrocells>" << endl;
      cerr << "-isovalue <isovalue>" << endl;
      throw IllegalArgument( "gridisovol", i, args );
    }
  }

  char c_filename[2048];
  strcpy(c_filename, filename.c_str());

  // Create the scene.
  Scene *scene = new Scene();
  Group *group = new Group();

  Manta::BBox bounds;

  //Material* mat1 = new Phong(Color(RGBColor(0.05f, 0.3f, 0.6f)), Color(RGBColor(1.f, 1.f, 1.f)), 50);
  Material* mat1 = new Lambertian(Color(RGBColor(0.05f, 0.3f, 0.6f)));
  IsosurfaceGridVolume* igv = new IsosurfaceGridVolume(c_filename, macrocells, isovalue, mat1);
  bounds = igv->getBounds();
  group->add(igv);

  // Add the tree to the scene.
  scene->setObject( group );

  float fov = 45.f;
  float view_dist = 0.5f * bounds[1].data[0] * (1.0f + 1.0f / tanf(0.5f * fov));
  Vector lookat = (bounds[1] - bounds[0]) * 0.5f;
  Vector eye = lookat;
  Vector up = Vector(0,0,1);
  eye.data[0] -= view_dist;

  // Set other important scene properties.
  // NOTE(boulos): As of 29-Jul-2007 these were unused as well...

  //PinholeCamera *camera = new PinholeCamera(eye, lookat, up, fov, fov);

  //scene->setCamera(camera);

  //NOTE: this will create a light directly "above" the volume in the +up direction.
  //  Some volumes (e.g. heptane) will require us to change this to a -
  LightSet *lights = new LightSet();
  float ldist = 10.f * (lookat-eye).length();

  lights->add( new PointLight( Vector(lookat + ldist*up), Color(RGBColor(1,1,1)) ) );
  lights->setAmbientLight( new ConstantAmbient( Color(RGBColor(0.4,0.5,0.5) ) ));
  scene->setLights(lights);


  // Background.
  scene->setBackground( new ConstantBackground( Color(RGB(.96, 0.96, 0.97)) ) );

  return scene;
}

