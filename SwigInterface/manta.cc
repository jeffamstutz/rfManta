//#include <Interface/RTRTInterface.h>
#include <Core/Color/ColorDB.h>
//#include <Core/Util/Args.h>
#include <Interface/Scene.h>
//#include <Interface/UserInterface.h>

#include <Interface/LightSet.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Lights/PointLight.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Materials/Phong.h>
#include <Model/Groups/Group.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/TexCoordMappers/UniformMapper.h>

using namespace Manta;

Manta::Scene* createDefaultScene()
{
  // Create a default scene.  This scene is used for benchmarks, so
  // please do not change it.  Please create a new scene instead
  Scene* scene = new Scene();
  scene->setBackground(new ConstantBackground(ColorDB::getNamedColor("SkyBlue3")*0.5));
  Material* red=new Phong(Color(RGBColor(.6,0,0)),
                          Color(RGBColor(.6,.6,.6)), 32, 0.4);

  Material* plane_matl = new Phong(new CheckerTexture<Color>(Color(RGBColor(.6,.6,.6)),
																														 Color(RGBColor(0,0,0)),
																														 Vector(1,0,0),
																														 Vector(0,1,0)),
																	 new Constant<Color>(Color(RGBColor(.6,.6,.6))),
																	 32,
																	 new CheckerTexture<ColorComponent>
                                   ((ColorComponent)0.2,
                                    (ColorComponent)0.5,
                                    Vector(1,0,0),
                                    Vector(0,1,0)));

  Group* world = new Group();
  Primitive* floor = new Parallelogram(plane_matl, Vector(-20,-20,0),
                                       Vector(40,0,0), Vector(0,40,0));
  // Setup world-space texture coordinates for the checkerboard floor
  UniformMapper* uniformmap = new UniformMapper();
  floor->setTexCoordMapper(uniformmap);
  world->add(floor);
  world->add(new Sphere(red, Vector(0,0,1.2), 1.0));
  scene->setObject(world);

  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(0,5,8), Color(RGBColor(.6,.1,.1))));
  lights->add(new PointLight(Vector(5,0,8), Color(RGBColor(.1,.6,.1))));
  lights->add(new PointLight(Vector(5,5,2), Color(RGBColor(.2,.2,.2))));
  lights->setAmbientLight(new ConstantAmbient(Color::black()));
  scene->setLights(lights);
  scene->getRenderParameters().maxDepth = 5;
  scene->addBookmark("default view", Vector(3, 3, 2), Vector(0, 0, 0.3), Vector(0, 0, 1), 60, 60);
  return scene;
}
