
/*
  Adding a new scene designed to stress acceleration structures.

  The scene creates a number of instanced objects randomly distributed throughout a cube.
  It takes the following options:

  -num - number of objects to render
  -type - type of objects to render [sphere, cone, disk, box]
  -model - load this mesh instead of using a builtin primitive [OBJ or PLY]
  -object_scale - scale the rendered objects by this amount
  -cube_scale - scale the bounding cube the primitives are rendered into
  -material - use a named shader instead of a constant color [lambertian, constant, null]
  -DynBVH - use DynBVH as the acceleration structure [default]
  -CGT - use CGT as the acceleration structure

  Here are some example command lines:

  -Visualize the BVH
  bin/manta -scene "lib/libscene_complexitytest.dylib(-num 500 -material null)" -imagetraverser "tiled(-tilesize 1x1)" -t

  -Load a bunch of bunnies
  bin/manta -scene "lib/libscene_complexitytest.dylib(-num 20 -object_scale 2 -material lambertian -model bun_zipper.ply)"

  -Big cubes
  bin/manta -scene "lib/libscene_complexitytest.dylib (-type box -object_scale 10 -cube_scale 3)"
*/



#include <Core/Color/ColorDB.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/UnknownColor.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/MT_RNG.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Image/TGAFile.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/MiscObjects/Difference.h>
#include <Model/MiscObjects/Intersection.h>
#include <Model/MiscObjects/KeyFrameAnimation.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/KDTree.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <Model/Groups/ObjGroup.h>
#include <Model/Instances/Instance.h>
#include <Model/Instances/InstanceRT.h>
#include <Model/Instances/InstanceRST.h>
#include <Model/Instances/InstanceST.h>
#include <Model/Instances/InstanceT.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Checker.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/NullMaterial.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <Model/Primitives/Cone.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/Disk.h>
#include <Model/Primitives/Hemisphere.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/SuperEllipsoid.h>
#include <Model/Primitives/Torus.h>
#include <Model/Primitives/Heightfield.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/TexCoordMappers/LinearMapper.h>
#include <Model/TexCoordMappers/SphericalMapper.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Textures/ImageTexture.h>
#include <Model/Textures/MarbleTexture.h>
#include <Model/Textures/NormalTexture.h>
#include <Model/Readers/PlyReader.h>
#include <Model/Textures/WoodTexture.h>
#include <Model/Textures/OakTexture.h>

#include <string>
#include <vector>
#include <iostream>

#include "UsePrivateCode.h"
#ifdef USE_PRIVATE_CODE
#include <Model/Groups/private/CGT.h>
#endif

using namespace Manta;
using namespace std;

AccelerationStructure* getAS(string type)
{
  if(type == "DynBVH")
    return new DynBVH();

  if(type == "KDTree")
    return new KDTree();

#ifdef USE_PRIVATE_CODE
  if(type == "CGT")
    return new Grid();
#endif

  static bool already_warned = false;
  if(!already_warned) {
    cerr << "Warning: unknown acceleration structure type, defaulting to DynBVH" << endl;
    already_warned = true;
  }

  return new DynBVH();
}

Material* getMaterial(string type, Color c)
{
  if(type == "lambertian")
    return new Lambertian(c);

  if(type == "constant")
    return new CopyTextureMaterial(c);

  if(type == "null")
    return new NullMaterial();

  static bool already_warned = false;
  if(!already_warned) {
    cerr << "Warning: unknown material type, defaulting to constant color" << endl;
    already_warned = true;
  }

  return new CopyTextureMaterial(c);
}

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  Real object_scale = 1.0;
  Real cube_scale = 100.0;
  unsigned int num_objects = 1000;
  string mesh_name = "";
  string object_type = "sphere";
  string object_material = "constant";
  string acc_struct = "DynBVH";
  bool no_instances = false;

  for(size_t i = 0; i < args.size(); ++i) {
    string arg = args[i];

    if(arg == "-num") {
      if(!getArg<unsigned int>(i, args, num_objects))
        throw IllegalArgument("scene complexitytest -num", i, args);
    } else if (arg == "-type") {
      if(!getStringArg(i, args, object_type))
        throw IllegalArgument("scene complexitytest -type", i, args);
    } else if (arg == "-object_scale") {
      if(!getArg<Real>(i, args, object_scale))
        throw IllegalArgument("scene complexitytest -object_scale", i , args);
    } else if (arg == "-cube_scale") {
      if(!getArg<Real>(i, args, cube_scale))
        throw IllegalArgument("scene complexitytest -cube_scale", i , args);
      cube_scale *= 100;
    } else if (arg == "-model") {
      if(!getStringArg(i, args, mesh_name))
        throw IllegalArgument("scene complexitytest -model", i, args);
    } else if (arg == "-material") {
      if(!getStringArg(i, args, object_material))
        throw IllegalArgument("scene complexitytest -material", i, args);
    } else if (arg == "-DynBVH") {
      acc_struct = "DynBVH";
    } else if (arg == "-CGT") {
      acc_struct = "CGT";
    } else if (arg == "-KDTree") {
      acc_struct = "KDTree";
    } else if (arg == "-no_instances") {
      no_instances = true;
    } else {
      cerr << "Valid options for complexitytest:\n"
           << "  -num - number of objects to render\n"
           << "  -type - type of objects to render [sphere, cone, disk, torus, box]\n"
           << "  -model - load this mesh instead of using a builtin primitive\n"
           << "  -object_scale - scale the rendered objects by this amount\n"
           << "  -cube_scale - scale the bounding cube the primitives are rendered into\n"
           << "  -material - use a named shader instead of a constant color [lambertian, constant, null]\n";
      throw IllegalArgument("scene complexitytest", i, args);
    }
  }

  Mesh* mesh = NULL;

  if(mesh_name != "") {
    // load mesh
    if(!strncmp(mesh_name.c_str()+mesh_name.length()-4, ".ply", 4)) {
      mesh = new Mesh();
      AffineTransform t;
      t.initWithIdentity();
      if (!readPlyFile(mesh_name, t, mesh, NULL)) {
        cerr << "Warning: could not load mesh: " << mesh_name << endl;;
        delete mesh;
        mesh = NULL;
      }
    } else if(!strncmp(mesh_name.c_str()+mesh_name.length()-4, ".obj", 4)) {
      mesh = new ObjGroup(mesh_name.c_str());
    } else {
      cerr << "Warning: could not load mesh: " << mesh_name << endl;
    }
  }


  Group* group = new Group();

  Object* object = NULL;

  Material* matl = new NullMaterial();

  if(mesh) {
    // build a bvh around the mesh
    DynBVH* mesh_bvh = new DynBVH();
    mesh_bvh->setGroup(mesh);
    object = mesh_bvh;
  } else {
    if(object_type == "sphere") {
      object = new Sphere(matl, Vector(0,0,0), 1.0);
    } else if(object_type == "disk") {
      object = new Disk(matl, Vector(0,0,0), Vector(0,0,1), 1.0, Vector(1,0,0));
    } else if(object_type == "cone") {
      object = new Cone(matl, 1.0, 2.0);
    } else if(object_type == "torus") {
      object = new Torus(matl, 1.0, 2.0);
    } else if(object_type == "box") {
      object = new Cube(matl, Vector(-.5, -.5, -.5), Vector(.5, .5, .5));
    } else {
      throw InternalError("unknown primitive type: " + object_type);
    }
  }

  cerr << "Generating " << num_objects << " instances...\n";
  Material* no_instance_matl = new NullMaterial();
  MT_RNG rng;
  for(unsigned int i = 0; i < num_objects; ++i) {
    if(no_instances) {
      Vector origin = Vector(rng.nextReal(), rng.nextReal(), rng.nextReal())*cube_scale;
      Color color = Color(RGB(rng.nextReal(), rng.nextReal(), rng.nextReal()));

      group->add(new Sphere(no_instance_matl, origin, object_scale));

    } else {
      Vector origin = Vector(rng.nextReal(), rng.nextReal(), rng.nextReal())*cube_scale;
      Vector rotation_axis = Vector(rng.nextReal(), rng.nextReal(), rng.nextReal()).normal();
      Real rotation_amount = rng.nextReal()*M_PI*2.0;

      Color color = Color(RGB(rng.nextReal(), rng.nextReal(), rng.nextReal()));

      AffineTransform t;
      t.initWithIdentity();
      t.scale(Vector(object_scale, object_scale, object_scale));
      t.rotate(rotation_axis, rotation_amount);
      t.translate(origin);

      Instance* instance = new Instance(object, t);
      instance->overrideMaterial( getMaterial(object_material, color) );

      group->add(instance);
    }
  }


  Scene* scene = new Scene();
  scene->setBackground(new LinearBackground(Color(RGB(0.2, 0.4, 0.9)),
                                            Color(RGB(0.0,0.0,0.0)),
                                            Vector(0,1,0)));

  AccelerationStructure* as = getAS(acc_struct);
  as->setGroup(group);
  as->rebuild();
  scene->setObject(as);

  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(-2,4,-8), Color(RGB(1,1,1))*1));
  Color cup(RGB(0.3, 0.3, 0.3));
  Color cdown(RGB(0.62, 0.62, 0.62));
  Vector up(0,1,0);
  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
  scene->setLights(lights);
  return scene;
}
