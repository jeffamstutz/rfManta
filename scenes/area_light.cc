// Default scene includes.
#include <Core/Util/Args.h>
#include <Core/Color/ColorDB.h>
#include <Interface/LightSet.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/EnvMapBackground.h>
#include <Model/Backgrounds/TextureBackground.h>
#include <Model/Lights/PointLight.h>
#include <Model/Lights/AreaLight.h>

#include <Model/Textures/Constant.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Textures/MarbleTexture.h>
#include <Model/Textures/ImageTexture.h>
// Manta Materials
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/EmitMaterial.h>
#include <Model/Materials/ThinDielectric.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/NullMaterial.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/Lambertian.h>

#include <Model/Groups/Group.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/Cube.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Core/Thread/Thread.h>
#include <Interface/Scene.h>

#include <Model/Lights/PointLight.h>

#include <iostream>

using namespace Manta;

extern "C"
Scene* make_scene(const ReadContext& context, const vector<string>& args)
{
  Scene* scene = new Scene();

  bool use_dielectric = false;
  bool use_point_light = false;

  for(unsigned int i = 0; i < args.size(); ++i) {
    string arg = args[i];

    if(arg == "-use_dielectric")
      use_dielectric = true;

    if(arg == "-use_point_light")
      use_point_light = true;

  }

  scene->setBackground(new ConstantBackground(Color::white()*.2));

  Group* group = new Group();

  scene->setObject(group);

  LightSet* lights = new LightSet();

  Color area_light_color = Color(RGB(.9,.85,.45))*6;
  Parallelogram* area_light_geometry = new Parallelogram(new EmitMaterial(area_light_color),
                                                         Vector(.25,1,.25), Vector(.5,0,0), Vector(0,0,.5));

  if(!use_point_light)
    group->add(area_light_geometry);

  group->add(new Parallelogram(new Lambertian(Color::white()*.8),
                               Vector(-10, 0, -10), Vector(20, 0, 0), Vector(0, 0, 20)));

  Material* matl;
  if(use_dielectric) {
    matl = new ThinDielectric(1.0, Color(RGB(.5,0,0)), .01);
  } else {
    matl = new Lambertian(Color(RGB(.5,0,0)));
  }
  group->add(new Parallelogram(matl,
                               Vector(.25, .5, .25), Vector(0, 0,.5), Vector(.5,0,0)));



  if(!use_point_light)
    lights->add(new AreaLight(area_light_geometry, area_light_color));
  else
    lights->add(new PointLight(Vector(.5, 1, .5), Color(RGB(.9, .85, .45))));

  lights->setAmbientLight(new ConstantAmbient(Color::black()));

  scene->setLights(lights);

  Vector eye = Vector(0, 1.25, 2);
  Vector lookat =  Vector(.6, .32, .25);

  scene->addBookmark("default view",
                     eye,
                     lookat,
                     Vector(0,1,0), 60, 45);

  scene->getRenderParameters().setMaxDepth(5);
  scene->getRenderParameters().setImportanceCutoff(0.01);

  return scene;

}
