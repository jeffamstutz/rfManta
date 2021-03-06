#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/ObjGroup.h>
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
#include <Model/Textures/MarbleTexture.h>

#include <iostream>

using namespace Manta;
using namespace std;

#define Rect(material, center, dir1, dir2) \
  Parallelogram(material, (center) - (dir1) - (dir2), (dir1)*2, (dir2)*2)

class TeapotObjGroup: public ObjGroup {
public:
  TeapotObjGroup( const char* filename ):
    ObjGroup(filename)
  {
  }

protected:
  virtual void create_materials( Glm::GLMmodel *model )
  {
    // Here we are going to override the materials with our stuff

    //////////////////////////////////////////////////////////////////////////
    // Path to the model for loading textures.
    string model_path = model->pathname;

    size_t pos = model_path.rfind( '/' );
    if (pos != string::npos) {
        model_path = model_path.substr( 0, pos+1 );
    }

    material_array = new Material *[ model->nummaterials ];
    material_array_size = model->nummaterials;

    // Just override the materials
    for (unsigned int i=0;i<model->nummaterials;++i) {
      material_array[i] = new Lambertian(new Constant<Color>(Color(RGB(1,0,1))));
    }
  }
};


void addFence(Group* group)
{
  Material* white = new Lambertian(Color::white() * 0.8);

  //vertica posts
  group->add( new Cube(white, Vector(91,-8,-20), Vector(99,0,40)) );
  group->add( new Cube(white, Vector(-9,-8,-20), Vector(-1,0,40)) );
  group->add( new Cube(white, Vector(-109,-8,-20), Vector(-101,0,40)) );

  //cross bars
  group->add( new Cube(white, Vector(-109,-8,40), Vector(99,0,44)) );
  group->add( new Cube(white, Vector(-109,-8,0), Vector(99,0,4)) );

  //slats
  group->add( new Cube(white, Vector(90,-10,-15), Vector(100,-8,60)) );
  group->add( new Cube(white, Vector(70,-10,-15), Vector(80,-8,60)) );
  group->add( new Cube(white, Vector(50,-10,-15), Vector(60,-8,60)) );
  group->add( new Cube(white, Vector(30,-10,-15), Vector(40,-8,60)) );
  group->add( new Cube(white, Vector(10,-10,-15), Vector(20,-8,60)) );
  group->add( new Cube(white, Vector(-10,-10,-15), Vector(0,-8,60)) );
  group->add( new Cube(white, Vector(-30,-10,-15), Vector(-20,-8,60)) );
  group->add( new Cube(white, Vector(-50,-10,-15), Vector(-40,-8,60)) );
  group->add( new Cube(white, Vector(-70,-10,-15), Vector(-60,-8,60)) );
  group->add( new Cube(white, Vector(-90,-10,-15), Vector(-80,-8,60)) );
  group->add( new Cube(white, Vector(-110,-10,-15), Vector(-100,-8,60)) );
}


void addFloor(Group* group, bool use_tex)
{
  Material* floor_matl = NULL;
  if(use_tex) {
    Texture<Color>* floor_tex = new MarbleTexture<Color>(Color(RGB(.2,.8,.1)),
                                                         Color(RGB(.5,.1,0)),
                                                         30,
                                                         2.4,
                                                         3.1,
                                                         5,
                                                         3.2,
                                                         .7);
    floor_matl = new Lambertian(floor_tex);
  } else {
    floor_matl = new Lambertian(Color::white() *0.8);
  }

  Object* floor=new Rect(floor_matl,
                         Vector(-100,-100,-20),
                         Vector(320,0,0),
                         Vector(0,320,0));
  group->add(floor);
}

// generate N points on [0,1]^2
void getLightSamples( float *u_, float *v_, int n)
{
  //for (int i = 0; i < n; i++) {
  // u[i] = drand48();
  //   v[i] = drand48();
  //}
  // hammersley from 97 jgt article
  float p, u, v;
  int k, kk, pos;

  for (k=0, pos=0 ; k<n ; k++)
  {
    u = 0;
    for (p=0.5, kk=k ; kk ; p*=0.5, kk>>=1)
      if (kk & 1)                           // kk mod 2 == 1
        u += p;

    v = (k + 0.5) / n;

    u_[k] = u;
    v_[k] = v;
  }
}

void addLights( LightSet* lights, int num_lights, float width_scale)
{
  float minx = -200; float scalex = 30*width_scale;
  float minz = 400; float scalez = 30*width_scale;
  float y = 400;
  Color col = Color(RGB(0.8,0.8,0.8)) * (1.0/num_lights);
  float *u = new float[num_lights];
  float *v = new float[num_lights];
  getLightSamples( u, v, num_lights );
  for (int i = 0; i < num_lights; i++) {
     lights->add(new PointLight(Vector(minx + u[i] * scalex, y, minz + v[i]*scalez), col));
  }
  delete [] u;
  delete [] v;
}


MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  int num_lights = 16;
  double width_scale= 1.0;
  bool use_tex = false;

  for(size_t i = 0; i < args.size(); ++i) {
    string arg = args[i];

    if(arg == "-num") {
      if(!getIntArg(i, args, num_lights))
        throw IllegalArgument("scene softshadow -num", i, args);
    } else if (arg == "-width_scale") {
      if(!getDoubleArg(i, args, width_scale))
        throw IllegalArgument("scene softshadow -width_scale", i , args);
    } else if (arg == "-use_tex") {
      use_tex = true;
    } else {
      cerr << "Valid options for softshadow:\n"
           << "  -use_tex - add a masking texture to the floor\n"
           << "  -num - number of point lights\n"
           << "  -width_scale - multiplier for light width\n" ;
      throw IllegalArgument("scene complexitytest", i, args);
    }
  }

  // Start adding geometry
  Group* group = new Group();

  addFence(group);
  addFloor(group, use_tex);

  Scene* scene = new Scene();
  scene->setBackground(new ConstantBackground(Color(RGB(0.1, 0.1, 0.1))));

  DynBVH* bvh = new DynBVH();
  bvh->setGroup(group);
  scene->setObject(bvh);

  LightSet* lights = new LightSet();
  addLights(lights, num_lights, width_scale);
  Color cup(RGB(0.2, 0.2, .2));
  Color cdown(RGB(0.2, 0.2, 0.2));
  Vector up(0,0,1);
  //  Vector up(0,1,0);
  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
  scene->setLights(lights);

  // Add a default camera
  Vector eye(-200,-200,80);
  Vector lookat(-30,-30,0);
  Real fov=45;
  scene->addBookmark("default view", eye, lookat, up, fov, fov);

  return scene;
}

