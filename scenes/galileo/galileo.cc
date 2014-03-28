#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Geometry/Vector.h>
#include <Core/Util/Args.h>
#include <Core/Util/NotFinished.h>
#include <Core/Util/Preprocessor.h>
#include <Engine/Factory/Factory.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/MantaInterface.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Textures/CheckerTexture.h>


#include <iostream>

#include <math.h>
#include <string.h>

using namespace std;
using namespace Manta;

extern FILE* yyin;
extern int yyparse(void);
extern Group* world;

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext& context, const vector<string>& args)
{
//   Group* world = new Group();

//   Material* matl0=new Lambertian(Color(RGBColor(.4,.4,.4)));
//   world->add(new Sphere(matl0, Vector(1, 1, 1), 1.0f));

  cerr << "args[0] = "<<args[0]<<"\n";

  yyin = fopen(args[0].c_str(), "r");
  if (!yyin) {
    perror("Can't open file");
    return NULL;
  }
  if (!yyparse()) {
    perror("Error parsing");
  }
  fclose(yyin);

  Scene* scene = new Scene();
  scene->setBackground(new LinearBackground(Color(RGB(0.2, 0.4, 0.9)),
                                            Color::black(),
                                            Vector(0,0,1)));
  scene->setObject(world);

  scene->addBookmark("default view",
                     /* eye */    Vector(10, 10, 10),
                     /* lookat */ Vector(1,1,1),
                     /* up     */ Vector(0,0,1),
                     /* hfov    */ 28.2,
                     /* vfov   */ 28.2);

  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(5,-3,3), Color(RGB(1,1,.8))*2));
  Color cup(RGB(0.1, 0.3, 0.8));
  Color cdown(RGB(0.82, 0.62, 0.62));
  Vector up(0,0,1);
  //  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
  lights->setAmbientLight(new ConstantAmbient(Color(RGBColor(0.3,0.3,0.3))));
  scene->setLights(lights);
  return scene;
}
