#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/EnvMapBackground.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Instances/Instance.h>
#include <Model/Instances/InstanceRST.h>
#include <Model/Instances/InstanceRT.h>
#include <Model/Instances/InstanceST.h>
#include <Model/Instances/InstanceT.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Checker.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/Phong.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/ImageTexture.h>
#include <Model/Textures/MarbleTexture.h>
#include <Model/Textures/TileTexture.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/TexCoordMappers/LinearMapper.h>

#include <iostream>

using namespace Manta;

void addSphere(Group* group)
{
  Material* metal = new MetalMaterial( Color( RGB(0.3, 0.3, 0.3) ), 10 );

  Object* sphere = new Sphere( metal, Vector(0,0,-1.5), 1.0f );

  group->add(sphere);
}

void addDielectricSphere(Group* group)
{
  Material* glass1 = new Dielectric( 1.4, 1.0, Color(RGB(.7,.2,.2)) );
  Object* sphere1 = new Sphere( glass1, Vector(1.5, -.5, 2), .5 );

  Material* glass2 = new Dielectric( 1.1, 1.0, Color(RGB(.2,.7,.2)) );
  Object* sphere2 = new Sphere( glass2, Vector(-.5, -.3, 1.3), .7 );

  Material* glass3 = new Dielectric( 1.8, 1.0, Color(RGB(.2,.2,.7)) );
  Object* sphere3 = new Sphere( glass3, Vector(1.15, -.6, .4), .4 );

  Material* glass4 = new Dielectric( 1.2, 1.0, Color::white()*.9 );
  Object* sphere4 = new Sphere( glass4, Vector(-1.4, -.2, -.2), .8 );
  
  group->add(sphere1);
  group->add(sphere2);
  group->add(sphere3);
  group->add(sphere4);
}

void addFloor(Group* group)
{
  Texture<Color>* tile_tex = new CheckerTexture<Color>(Color::white()*.8, Color::black(), 
                                                       Vector(4,0,0), Vector(0,4,0));

  Material* tile = new Phong( tile_tex, new Constant<Color>(Color::white()*.8), 32, new Constant<ColorComponent>(.2) );

  Primitive* floor = new Parallelogram(tile, Vector(-2.5, -1.05, -2.5), Vector(5, 0, 0), Vector(0, 0, 5));

  floor->setTexCoordMapper( new LinearMapper( Vector(-2.5,-1.05,-2.5), Vector(1,0,0), Vector(0, 0, 1), Vector(0,1,0)) );
                            
  group->add(floor);
}

void addLights( LightSet* lights)
{
  lights->add( new PointLight( Vector(0, 30, 0) , Color( RGB(0.8,0.8,0.8) ) ) );
}


MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{

  std::string env_filename = "";
  EnvMapBackground::MappingType mapping_type = EnvMapBackground::CylindricalEqualArea;
  bool create_floor = false;
  bool create_glass = false;
  bool filter = false;

  for(size_t i = 0; i < args.size(); ++i) {
    std::string arg = args[i];
    if(arg == "-file") {
      if(!getStringArg(i, args, env_filename))
        throw IllegalArgument("scene hdritest -file", i , args);
    } else if (arg == "-cylindrical") {
      mapping_type = EnvMapBackground::CylindricalEqualArea;
    } else if (arg == "-latlon") {
      mapping_type = EnvMapBackground::LatLon;
    } else if (arg == "-sphere") {
      mapping_type = EnvMapBackground::DebevecSphere;
    } else if (arg == "-old") {
      mapping_type = EnvMapBackground::OldBehavior;
    } else if (arg == "-floor") {
      create_floor = true;
    } else if (arg == "-glass_sphere") {
      create_glass = true;
    } else if (arg == "-filter") {
      filter = true;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
    }
  }

  Vector up( 0.0f, 1.0f, 0.0f );
  Vector right( 1.0f, 0.0f, 0.0f );

  // Start adding geometry
  Group* all   = new Group();
  Scene* scene = new Scene();


  if ( !env_filename.empty() ) {
    ImageTexture<Color>* t = LoadColorImageTexture( env_filename, &std::cerr );
    if(filter)
      t->setInterpolationMethod(ImageTexture<Color>::Bilinear);

    scene->setBackground( new EnvMapBackground( t,
          mapping_type, right, up ) );
  } else {
    scene->setBackground( new ConstantBackground( Color(RGB(.3, .3, .9) ) ) );
  }


  addSphere(all);

  if(create_floor)
    addFloor(all);

  if(create_glass)
    addDielectricSphere(all);

  scene->setObject(all);

  LightSet* lights = new LightSet();
  addLights( lights );

  lights->setAmbientLight(new ConstantAmbient( Color( RGB(0.2, 0.2, 0.2) ) ) );

  scene->setLights(lights);

  // Add a default camera
  Vector eye(0,0,10);
  Vector lookat(0,0,0);
  Real   fov=45;
  scene->addBookmark("default view", eye, lookat, up, fov, fov);

  return scene;
}

