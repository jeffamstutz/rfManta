#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/DirectionalLight.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/TexCoordMappers/LinearMapper.h>
#include <Model/Textures/ImageTexture.h>

#include <iostream>

using namespace Manta;

void addLights( LightSet* lights)
{
  lights->add(new DirectionalLight(Vector(0, 0, 1), Color::white()));
}

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  std::string filename;
  for(size_t i = 0; i < args.size(); ++i) {
    std::string arg = args[i];
    if(arg == "-file") {
      if(!getStringArg(i, args, filename))
        throw IllegalArgument("scene macbeth -file", i , args);
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
    }
  }

  if (filename == "") {
    throw InputError("Filename required for macbeth scene");
  }

  Vector up( 0.0f, 1.0f, 0.0f );
  Vector right( 1.0f, 0.0f, 0.0f );

  // Start adding geometry
  Group* group = new Group();
  Scene* scene = new Scene();

  scene->setBackground( new ConstantBackground( Color(RGB(.5, .5, .5) ) ) );

  ImageTexture<Color>* texture = LoadColorImageTexture(filename, &std::cerr, true);
  Material* image = new CopyTextureMaterial(texture);
  Primitive* board = new Parallelogram(image, Vector(0, 0, 0), Vector(6, 0, 0), Vector(0, 4, 0));
  board->setTexCoordMapper( new LinearMapper( Vector(0, 0, 0), Vector(6,0,0), Vector(0, 4, 0), Vector(0,0,1)) );

  group->add(board);
  scene->setObject(group);

  LightSet* lights = new LightSet();
  addLights( lights );

  lights->setAmbientLight(new ConstantAmbient( Color::black()));
  scene->setLights(lights);

  // Add a default camera
  Vector eye(3,2,-10);
  Vector lookat(3,2,0);
  Real   fov=45;
  scene->addBookmark("default view", eye, lookat, up, fov, fov);

  return scene;
}

