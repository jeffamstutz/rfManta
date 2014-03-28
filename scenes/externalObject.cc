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
#include <Model/Materials/Lambertian.h>
#include <Model/Primitives/Sphere.h>
#include <Model/TexCoordMappers/LinearMapper.h>
#include <Model/Textures/ImageTexture.h>

#include <iostream>

using namespace Manta;

class SphereTwoX : public Sphere {
public:
  SphereTwoX(Material* material, const Vector& center, Real radius)
    : Sphere(material, center, radius * 2) {
  }
};

void addLights( LightSet* lights)
{
  lights->add(new DirectionalLight(Vector(0, 0, 1), Color::white()));
}

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  Vector up( 0.0f, 1.0f, 0.0f );
  Vector right( 1.0f, 0.0f, 0.0f );

  // Start adding geometry
  Group* group = new Group();
  Scene* scene = new Scene();

  scene->setBackground( new ConstantBackground( Color(RGB(.5, .5, .5) ) ) );

  Material* red = new Lambertian(Color(RGBColor(.78, .1, .1)));
  group->add(new SphereTwoX(red, Vector(0,0,1.2), 1.0));
  scene->setObject(group);

  LightSet* lights = new LightSet();
  addLights( lights );

  lights->setAmbientLight(new ConstantAmbient( Color::black()));
  scene->setLights(lights);

  // Add a default camera
  scene->addBookmark("default view", Vector(3, 3, 2), Vector(0, 0, 0.3), Vector(0, 0, 1), 60, 60);

  return scene;
}

