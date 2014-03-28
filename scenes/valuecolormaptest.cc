#include <Core/Color/ColorDB.h>
#include <Core/Color/Colormaps/LinearColormap.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/HeadLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/ValuePrimitive.h>
#include <Model/Textures/ValueColormap.h>

using namespace Manta;

MANTA_PLUGINEXPORT
Scene *make_scene(const ReadContext&, const vector<string>& args)
{
  ValueColormap<float> *cmap = new ValueColormap<float>(0.0, 5.0, Rainbow);
  Lambertian *matl = new Lambertian(cmap);

  Group *group = new Group;
  const int numSpheres = 20;
  const float inc = 5.0 / (numSpheres-1);
  for(int i=0; i<numSpheres; i++){
    Primitive *prim = new ValuePrimitive<float>(new Sphere(matl, Vector(2.0*i, 0.0, 0.0), 0.6),
                                                i*inc);
    group->add(prim);
  }

  Scene *scene = new Scene;
  scene->setBackground(new ConstantBackground(Color(RGBColor(0.6,0.6,0.6))));
  scene->setObject(group);

  LightSet *lights = new LightSet;
  lights->add(new HeadLight(3, Color(RGBColor(1,1,1))));
  lights->setAmbientLight(new ConstantAmbient(ColorDB::getNamedColor("black")));
  scene->setLights(lights);
  
  return scene;
}
