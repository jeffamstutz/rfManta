
//
// Phong microbenchmark
// This program tests the performance of Phong::shade by constructing the
// necessary data structures and calling Phong:sshade in a tight loop
//

#include <Core/Color/Color.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/RayPacket.h>
#include <Interface/Scene.h>
#include <Engine/Shadows/HardShadows.h>
#include <Engine/Shadows/NoShadows.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Phong.h>
#include <Model/Primitives/Sphere.h>
#include <string>
using namespace Manta;
using namespace std;

int
main()
{
  LightSet lights;
  lights.add(new PointLight(Vector(0,5,8), Color(RGBColor(.6,.1,.1))));
  lights.setAmbientLight(new ConstantAmbient(Color::black()));

  Color x = Color(RGBColor(1,1,1));
  Phong p(x, Color(RGBColor(1,1,1)), 100, 0);
  PreprocessContext ppc(0, &lights);
  p.preprocess(ppc);

  vector<string> noargs;
  // Variant 1 - use one of the next two lines
  NoShadows shadows(noargs);
  //HardShadows shadows(noargs);

  Sphere s(&p, Vector(0, 0, 0), 1);
  Scene scene;
  scene.setObject(&s);

  RenderContext c(0, 0, 0, 1, 0, 0, 0, 0, &shadows, 0, &scene, 0);
  RayPacketData data;
  // Variant 2 - use one of the next two lines
  RayPacket rays(data, RayPacket::MaxSize, 0, RayPacket::NormalizedDirections);
  //RayPacket rays(data, RayPacket::MaxSize, 0, 0);
  rays.resetHit();
  for(int i=0;i<rays.getSize();i++){
    RayPacket::Element& e = rays.get(i);
    e.ray = Ray(Vector(0, 0, 10), Vector(0, 0, 1));
    e.hitInfo.hit(9, &p, &s, 0);
  }

   for (int i=0;i<1000000;++i)
     p.shade(c, rays);

   return 0;
}
