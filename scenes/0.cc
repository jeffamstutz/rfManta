
#include <Core/Geometry/Vector.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/MantaInterface.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Textures/CheckerTexture.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Util/NotFinished.h>
#include <Engine/Factory/Factory.h>
#include <iostream>
#include <math.h>
#include <string.h>

using namespace Manta;
using namespace std;

static const double SCALE = 1./3.;
static const double BV_RADIUS = 1.0;

static void create_dirs(Vector* objset)
{
  double dist=1./sqrt(2.0);
  Vector dir[3];
  dir[0]=Vector(dist, dist, 0);
  dir[1]=Vector(dist, 0, -dist);
  dir[2]=Vector(0, dist, -dist);

  Vector axis(1, -1, 0);
  axis.normalize();

  double rot=asin(2.0/sqrt(6.0));
  AffineTransform t;
  t.initWithRotation(axis, rot);

  for(int n=0;n<3;n++){
    dir[n] = t.multiply_vector(dir[n]);
  }

  for(int ns=0;ns<3;ns++){
    AffineTransform t;
    t.initWithRotation(Vector(0,0,1), ns*2.*M_PI/3.);
    for(int nv=0;nv<3;nv++){
      objset[ns*3+nv] = t.multiply_vector(dir[nv]);
    }
  }
}

static void create_objs(Group* group, const Vector& center,
                        double radius, const Vector& dir, int depth,
                        Vector* objset, Material* matl)
{
  group->add(new Sphere(matl, center, radius));

  // Check if children should be generated
  if(depth > 0){
    depth--;

    // Rotation matrix to new axis from +Z axis
    AffineTransform mx;
    mx.initWithIdentity();
    mx.rotate(Vector(0,0,1), dir);

    double scale = radius * (1+SCALE);

    for(int n=0;n<9;n++){
      Vector child_vec(mx.multiply_vector(objset[n]));
      Vector child_pt(center+child_vec*scale);
      double child_rad=radius*SCALE; Vector child_dir = child_pt-center;
      child_dir *= 1./scale;
      create_objs(group, child_pt, child_rad, child_dir, depth, objset, matl);
    }
  }
}

static void make_box(Group* group, Material* matl,
                     const Vector& corner, const Vector& x, const Vector& y, const Vector& z)
{
  group->add(new Parallelogram(matl, corner, x*2, z*2));
  group->add(new Parallelogram(matl, corner+y*2, z*2, x*2));
  group->add(new Parallelogram(matl, corner, y*2, z*2));
  group->add(new Parallelogram(matl, corner+x*2, z*2, y*2));
  group->add(new Parallelogram(matl, corner+z*2, x*2, y*2));
}

static void make_obj(Group* world, Group* group, int size)
{
  Vector objset[9];
  create_dirs(objset);
  Material* matl0=new Lambertian(Color(RGBColor(.4,.4,.4)));
  create_objs(group, Vector(0,0,.5), BV_RADIUS/2.0, Vector(0,0,1),
                size, objset, matl0);

  Vector diag1(1,1,0);
  diag1.normalize();
  Vector diag2(-1,1,0);
  diag2.normalize();
  Material* matl1=new Lambertian(Color(RGBColor(.2,.4,.2)));
  diag1*=1.5;
  diag2*=1.5;
  Vector z(0,0,.4);
  Vector corner(-1.8,-.3,0);
  make_box(world, matl1, corner, diag1, diag2, z);

  Material* matl3=new MetalMaterial( Color(RGBColor(.7,.7,.7)));
  world->add(new Sphere(matl3, corner+diag1*1.25+diag2*.6+z*2+Vector(0,0,.6), .6));
  double planesize=15;
  double scale = 2*planesize;
  Material* matl2 = new Lambertian(new CheckerTexture<Color>(Color(RGBColor(.95,.95,.95)),
                                                             Color(RGBColor(.7,.3,.3)),
                                                             Vector(1,1.1,0)*scale,
                                                             Vector(-1.1,1,0)*scale));
  Vector edge1(planesize, planesize*1.1, 0);
  Vector edge2(-planesize*1.1, planesize, 0);
  Object* obj1=new Parallelogram(matl2, Vector(0,0,0)-edge1-edge2, edge1*2, edge2*2);
  world->add(obj1);
}

extern "C"
Scene* make_scene(const ReadContext& context, const vector<string>& args)
{
  int scenesize=2;
  double light_radius=0.8;
  Group* group = new Group;
  AccelerationStructure *as = NULL;

  Factory factory( context.manta_interface );

  for(size_t i=0;i<args.size();i++){
    string arg = args[i];
    if(arg == "-size"){
      if(!getIntArg(i, args, scenesize))
        throw IllegalArgument("scene 0 -size", i, args);
    } else if(arg == "-light"){
      if(!getDoubleArg(i, args, light_radius))
        throw IllegalArgument("scene 0 -light", i, args);
    } else if(arg == "-DynBVH"){
      as = new DynBVH;
    } else {
      cerr << "Valid options for scene 0:\n";
      cerr << " -size n     - Sets depth of sphereflake\n";
      cerr << " -light r    - Sets radius of light source for soft shadows\n";
      cerr << " -group spec - Sets the group or acceleration structure to use for the sphereflake\n";
      throw IllegalArgument("scene 0", i, args);
    }
  }

  Group* world = new Group();

  NOT_FINISHED("scene 0");
  make_obj(world, group, scenesize);

  if(as) {
    world->add(as);
    as->setGroup(group);
    as->rebuild();
  }
  else
    world->add(group);


#if 0
  double ambient_scale=1.0;
  Color bgcolor(RGB(0.1, 0.2, 0.45));
  Color cdown(RGB(0.82, 0.62, 0.62));
  Color cup(RGB(0.1, 0.3, 0.8));


  rtrt::Plane groundplane ( Vector(0, 0, 0), Vector(0, 0, 3) );
  Scene* scene=new Scene(obj, cam,
                         bgcolor, cdown, cup, groundplane,
                         ambient_scale, Arc_Ambient);

  scene->select_shadow_mode( Single_Soft_Shadow );
#endif
  Scene* scene = new Scene();
  scene->setBackground(new LinearBackground(Color(RGB(0.2, 0.4, 0.9)),
                                            Color::black(),
                                            Vector(0,0,1)));
  scene->setObject(world);
  scene->addBookmark("default view", Vector(1.8,-5.53,1.25), Vector(0.0,-.13,1.22),
                     Vector(0,0,1), 28.2, 28.2);
  scene->addBookmark("top view", Vector(-0.8, 0.1, 7), Vector(-1, 1, 1.5), Vector(-1, 1, 0), 45, 45);
  NOT_FINISHED("soft shadows/area lights for scene 0");
  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(5,-3,3), Color(RGB(1,1,.8))*2));
  Color cup(RGB(0.1, 0.3, 0.8));
  Color cdown(RGB(0.82, 0.62, 0.62));
  Vector up(0,0,1);
  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
  scene->setLights(lights);
  return scene;
}
