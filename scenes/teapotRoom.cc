#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/ObjGroup.h>
#include <Model/Instances/Instance.h>
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
#include <Model/Readers/PlyReader.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/MarbleTexture.h>

#include <iostream>

using namespace Manta;
using namespace std;

#define Rect(material, center, dir1, dir2) \
  Parallelogram(material, (center) - (dir1) - (dir2), (dir1)*2, (dir2)*2)

static string bunny_file("/Users/bigler/manta/data/ply/bun_zipper_res4.ply");
static string teapot_file("/Users/bigler/manta/data/obj/goodteapot.obj");

void usage()
{
  cerr << "Valid options for scene:\n";
  cerr << " -teapot   - Path to teapot obj file.  Defaults to: "<<teapot_file<<"\n";
  cerr << " -bunny    - Path to bunny ply file.   Defaults to: "<<bunny_file<<"\n";
}

void argumentError(int i, const vector<string>& args)
{
  usage();
  throw IllegalArgument("scene teapotRoom", i, args);
}

static Material* glass= new Dielectric(1.5, 1.0, Color(RGB(.80, .93 , .87)).Pow(0.2));//, 0.001);
void addTable(Group* group)
{
  // Top
  Object* box=new Cube(glass, Vector(-50.0,-10.0,-1.0), Vector(30.0,30.0,0) );
  group->add(box);

  // Legs
  Material* legs = new MetalMaterial( Color::white() * 0.01 );
  group->add(new Cube(legs, Vector(-48.0,-8.0,-20.0), Vector(-45.0,-5.0,-1.0) ) );
  group->add(new Cube(legs, Vector(-48.0,25.0,-20.0), Vector(-45.0,28.0,-1.0) ) );
  group->add(new Cube(legs, Vector(25.0,-8.0,-20.0), Vector(28.0,-5.0,-1.0) ) );
  group->add(new Cube(legs, Vector(25.0,25.0,-20.0), Vector(28.0,28.0,-1.0) ) );

  // Crossbars
  group->add(new Cube(legs, Vector(-45.0,-7.0,-4.0), Vector(25.0,-6.0,-1.0) ) );
  group->add(new Cube(legs, Vector(-45.0,26.0,-4.0), Vector(25.0,28.0,-1.0) ) );
  group->add(new Cube(legs, Vector(-47.0,-7.0,-4.0), Vector(-46.0,25.0,-1.0) ) );
  group->add(new Cube(legs, Vector(26.0,-7.0,-4.0), Vector(27.0,25.0,-1.0) ) );
}

Object* packMesh(const string& mesh_name, const AffineTransform& transform,
                 Material* material)
{
  Mesh* mesh = 0;
  if(mesh_name != "") {
    // load mesh
    if(!strncmp(mesh_name.c_str()+mesh_name.length()-4, ".ply", 4)) {
      mesh = new Mesh();
      if (readPlyFile(mesh_name, transform, mesh, material)) {
        DynBVH* mesh_bvh = new DynBVH();
        mesh_bvh->setGroup(mesh);
        return mesh_bvh;
      } else {
        cerr << "Warning: could not load mesh: " << mesh_name << endl;;
        return 0;
      }
    } else if(!strncmp(mesh_name.c_str()+mesh_name.length()-4, ".obj", 4)) {
      mesh = new ObjGroup(mesh_name.c_str(), material);
      DynBVH* mesh_bvh = new DynBVH();
      mesh_bvh->setGroup(mesh);
      Instance* instance = new Instance(mesh_bvh, transform);
      return instance;
    } else {
      cerr << "Warning: could not load mesh: " << mesh_name << endl;
      return 0;
    }
  }
  return 0;
}

void addTableTopStuff(Group* group)
{
  // Glass sphere
  Object* sphere = new Sphere(glass, Vector(20.0, 20.0, 4.0), 4.0);
  group->add(sphere);

  // Bunny
  {
    Material* bunny_material = new Phong(Color(RGB(0.63, 0.51, 0.5)),
                                         Color::white()*0.3,
                                         400);
    AffineTransform bunnyT;
    bunnyT.initWithIdentity();
    bunnyT.translate(Vector(-.35, -.0335, .15));
    bunnyT.rotate(Vector(1,0,0), M_PI_2);
    bunnyT.scale(Vector(95.0, -95.0, 95.0));
    Object* bunny = packMesh(bunny_file, bunnyT, bunny_material);
    if (bunny)
      group->add(bunny);
  }

  // Teapot
  {
    AffineTransform teapotT;
    teapotT.initWithIdentity();
    teapotT.translate(Vector(2.0,1.575,0.0));
    teapotT.rotate(Vector(0,1,0), M_PI_4);
    teapotT.rotate(Vector(1,0,0), M_PI_2);
    teapotT.scale(Vector(1,1,1)*3);
    Material* teapot_material = new MetalMaterial(Color::white()*0.8);
    Object* teapot = packMesh(teapot_file, teapotT, teapot_material);
    if (teapot)
      group->add(teapot);
  }

  // SIGGRAPH proceedings
}
void addFloor(Group* group)
{
  /* RTRT   CrowMarble(double scale,
                       const Vector& direction,
                       const Color&  c1,
                       const Color&  c2,
                       const Color&  c3)
            FastTurbulence(int _noctaves = 6,
                           double _a = 0.5,
                           double _s = 2.0,
                           int seed = 0,
                           int tblsize = 4096)
  */

  /* Manta     MarbleTexture(
                             ValueType const &value1,
                             ValueType const &value2,
                             Real const scale,
                             Real const fscale,
                             Real const tscale,
                             int const octaves,
                             Real const lacunarity,
                             Real const gain );
  */

  MarbleTexture<Color>* mtex1=new MarbleTexture<Color>
    (
     Color(RGB(0.5,0.6,0.6)),    // value1
     //Color(RGB(0.4,0.55,0.52)),
     Color(RGB(0.35,0.45,0.42)), // value2
     10,                       // scale
     // Vector(2,1,0)            // RTRT direction
     2.0,                        // fscale (From RTRT dir?)
     15.0,                        // tscale
     6,                          // octaves
     2.0,                        // lacunarity (_s?)
     0.5                         // gain (_a?)
     );
  MarbleTexture<Color>* mtex2=new MarbleTexture<Color>
    (
     Color(RGB(0.4,0.3,0.2)),    // value1
     //Color(RGB(0.35,0.34,0.32)),
     Color(RGB(0.20,0.24,0.24)), // value2
     15,                      // scale
     // Vector(-1,3,0)           // RTRT direction
     1,                       // fscale (From RTRT dir?)
     10,                        // tscale
     6,                          // octaves
     2.0,                        // lacunarity (_s?)
     0.5                         // gain (_a?)
     );
//   MarbleTexture<Color>* mtex3=new MarbleTexture<Color>
//     (Color(RGB(0.1,0.2,0.5)), Color(RGB(0.7,0.8,1.0)),
//      10.0, 1.0, 15.0, 6, 2.0, 0.6 );

  Material* marble1=new Phong(
                              mtex1,
                              new Constant<Color>(Color::white()*0.6),
                              100,
                              new Constant<ColorComponent>(0)
                              );
  Material* marble2=new Phong(
                              mtex2,
                              new Constant<Color>(Color::white()*0.6),
                              100,
                              new Constant<ColorComponent>(0)
                              );

  Material* checkered_floor = new Checker(marble1,
                                          marble2,
                                          Vector(16,0,0), Vector(0,16,0));
  //Vector(0.005,0,0), Vector(0,0.0050,0));

  //  checkered_floor = new CopyTextureMaterial(new Constant<Color>(Color::white()));
  Object* floor=new Rect(checkered_floor,
                         Vector(0,0,-20.0),
                         Vector(160.0,0,0),
                         Vector(0,160.0,0));
  group->add(floor);
}

void addWalls(Group* group)
{
  Material* white = new Lambertian(Color::white() * 0.8);
  group->add( // room10
             new Rect(white, Vector(160.0,0,60.0), Vector(0,0,80.0), Vector(0,160.0,0))
              );

  group->add( // room00
             new Rect(white, Vector(-160.0,0,60.0), Vector(0,0,80.0), Vector(0,160.0,0))
              );

  group->add( // room11
             new Rect(white, Vector(0,160.0,60.0), Vector(0,0,80.0), Vector(160.0, 0,0))
              );

  group->add( // room01
             new Rect(white, Vector(0,-160.0,60.0), Vector(0,0,80.0), Vector(160.0, 0,0))
              );

//   group->add( // ceiling (roomtb)
//              new Rect(white, Vector(0,0,140.0), Vector(0,160.0,0), Vector(160.0, 0,0))
//               );

}

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  for(size_t i=0;i<args.size();i++){
    string arg = args[i];
    if (arg == "--help") {
      usage();
      return 0;
    } else if(arg == "-teapot") {
      if(!getStringArg(i, args, teapot_file))
        throw IllegalArgument("scene teapotRoom - teapot", i ,args);
    } else if(arg == "-bunny") {
      if(!getStringArg(i, args, bunny_file))
        throw IllegalArgument("scene teapotRoom - teapot", i ,args);
    }
  }

  // Start adding geometry
  Group* group = new Group();

  addTable(group);
  addTableTopStuff(group);
  addFloor(group);
  addWalls(group);

  Scene* scene = new Scene();
  scene->setBackground(new ConstantBackground(Color(RGB(0.1, 0.1, 0.1))));

  DynBVH* bvh = new DynBVH();
  bvh->setGroup(group);
  scene->setObject(bvh);

  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(200,400,1300), Color(RGB(1,1,1))*0.8));
  Color cup(RGB(0.5, 0.5, 0));
  Color cdown(RGB(0.1, 0.1, 0.7));
  Vector up(0,0,1);
  //  Vector up(0,1,0);
  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
  scene->setLights(lights);

  // Add a default camera
  Vector eye(70.0,139.0,30.0);
  Vector lookat(0,0,15.0);
  Real fov=45;
  scene->addBookmark("default view", eye, lookat, up, fov, fov);

  return scene;
}

