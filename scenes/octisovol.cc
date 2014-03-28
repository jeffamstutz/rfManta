
// Manta scene for octree isosurface volume rendering
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
#include <Model/Primitives/IsosurfaceOctreeVolume.h>
#include <Model/Primitives/OctreeVolume.h>
using namespace Manta;

///////////////////////////////////////////////////////////////////////////
MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext& context, const vector<string>& args) {

  string filename = "";
  string buildfrom_filename = "";

  int tstart = 0;
  int tend = 0;
  double variance = 0.1;
  int kernel_width = 2;
  int mres_levels = 0;
  double isomin = 0;
  double isomax = 255;
  double isovalue = 100;

  // Parse args.i
  for (size_t i=0;i<args.size();++i) {
    if (args[i] == "-file") {
      if (!getStringArg(i, args, filename))
        throw IllegalArgument("octisovol -file <filename>", i, args);
    }
    else if (args[i] == "-buildfrom") {
      if (!getStringArg(i, args, buildfrom_filename))
        throw IllegalArgument("octisovol -buildfrom <filename>", i, args);
    }
    else if (args[i] == "-tstart") {
      if (!getIntArg(i, args, tstart))
        throw IllegalArgument("octisovol -tstart <tstart>", i, args);
    }
    else if (args[i] == "-tend") {
      if (!getIntArg(i, args, tend))
        throw IllegalArgument("octisovol -tend <tend>", i, args);
    }
    else if (args[i] == "-variance") {
      if (!getDoubleArg(i, args, variance))
        throw IllegalArgument("octisovol -variance <variance>", i, args);
    }
    else if (args[i] == "-kernel_width") {
      if (!getIntArg(i, args, kernel_width))
        throw IllegalArgument("octisovol -kernel_width <kernel_width>", i, args);
    }
    else if (args[i] == "-mres_levels") {
      if (!getIntArg(i, args, mres_levels))
        throw IllegalArgument("octisovol -mres_levels <mres_levels>", i, args);
    }
    else if (args[i] == "-isomin") {
      if (!getDoubleArg(i, args, isomin))
        throw IllegalArgument("octisovol -isomin <isomin>", i, args);
    }
    else if (args[i] == "-isomax") {
      if (!getDoubleArg(i, args, isomin))
        throw IllegalArgument("octisovol -isomax <isomax>", i, args);
    }
    else if (args[i] == "-isovalue") {
      if (!getDoubleArg(i, args, isovalue))
        throw IllegalArgument("octisovol -isovalue <isovalue>", i, args);
    }
    else {
      cerr << "Read built octree volume:" << endl;
      cerr << "-file <filename>"             << endl;
      cerr << "Octree build:" << endl;
      cerr << "  -buildfrom <filename> " << endl;
      cerr << "Octree build options:" << endl;
      cerr << "  -tstart <tstart>" << endl;
      cerr << "  -tend <tend>" << endl;
      cerr << "  -variance <variance>" << endl;
      cerr << "  -kernel_width <kernel_width>" << endl;
      cerr << "  -mres_levels <mres_levels>" << endl;
      cerr << "  -isomin <isomin>" << endl;
      cerr << "  -isomax <isomax>" << endl;
      cerr << "  -isovalue <isovalue>" << endl;
      throw IllegalArgument( "octisovol", i, args );
    }
  }

  char c_filename[2048];
  char c_buildfrom_filename[2048];
  strcpy(c_filename, filename.c_str());
  strcpy(c_buildfrom_filename, buildfrom_filename.c_str());

  OctreeVolume* ov;

  if (strcmp(c_filename, "")!=0)
  {
    cerr << "Creating octree volume from existing .otd data." << endl;
    ov = new OctreeVolume(c_filename);
  }
  else if (strcmp(c_buildfrom_filename,"")!=0)
  {
    cerr << "Creating octree volume from original rectilinear grid data." << endl;
    ov = new OctreeVolume(c_buildfrom_filename, tstart, tend, variance, kernel_width,
                          static_cast<Manta::ST>(isomin), static_cast<Manta::ST>(isomax));
  }
  else
  {
    throw InputError( "octisovol: No volume source specified. Use either -file or -buildfrom.");
  }

  ov->set_isovalue(isovalue);

  // Create the scene.
  Scene *scene = new Scene();
  Group *group = new Group();

  Manta::BBox bounds;

  //Material* mat1 = new Phong(Color(RGBColor(0.05f, 0.3f, 0.6f)), Color(RGBColor(1.f, 1.f, 1.f)), 50);
//#define H300
#ifdef H300
  Material* mat1 = new Lambertian(Color(RGBColor(0.8f, 0.4f, 1.f)));      //purple
#else
  Material* mat1 = new Lambertian(Color(RGBColor(0.05f, 0.3f, 0.6f)));      //blue
#endif
  IsosurfaceOctreeVolume* iov = new IsosurfaceOctreeVolume(ov, mat1);
  bounds = iov->getBounds();
  group->add(iov);

  // Add the tree to the scene.
  scene->setObject( group );

  float fov = 45.f;
  float view_dist = 0.5f * bounds[1].data[0] * (1.0f + 1.0f / tanf(0.5f * fov));
  Vector lookat = (bounds[1] - bounds[0]) * 0.5f;
  Vector eye = lookat;
  Vector up = Vector(0,0,1);
  eye.data[0] -= view_dist;

  // Set other important scene properties.
  // NOTE(boulos): These weren't being used as of 29-Jul-2007.

  //PinholeCamera *camera = new PinholeCamera(eye, lookat, up, fov, fov);
  //scene->setCamera(camera);

  //NOTE: this will create a light directly "above" the volume in the +up direction.
  //  Some volumes (e.g. heptane) will require us to change this to a -
  LightSet *lights = new LightSet();
  float ldist = 10.f * (lookat-eye).length();

#ifdef H300
  lights->add( new PointLight( Vector(lookat + ldist*(eye - 2*up)), Color(RGBColor(0.8,0.8,0.8)) ) );
#else
  lights->add( new PointLight( Vector(lookat + ldist*up), Color(RGBColor(1,1,1)) ) );
#endif
  lights->setAmbientLight( new ConstantAmbient( Color(RGBColor(0.4,0.5,0.5) ) ));
  scene->setLights(lights);


  // Background.
#ifdef H300
  scene->setBackground( new ConstantBackground( Color(RGB(0,0,0)) ) );
#else
  scene->setBackground( new ConstantBackground( Color(RGB(.96, 0.96, 0.97)) ) );
#endif

  return scene;
}

