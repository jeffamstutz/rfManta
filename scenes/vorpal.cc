#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/MT_RNG.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/Phong.h>
#include <Model/MiscObjects/KeyFrameAnimation.h>
#include <Model/Primitives/Heightfield.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Readers/PlyReader.h>
#include <Model/Textures/HeightColorMap.h>

#include <string>
#include <iostream>
#include <fstream>

using namespace Manta;
using namespace std;


Group* readParticles(const string& particles_filename, Vector& maxBound,
                     HeightColorMap *heightMap)
{
  ifstream in(particles_filename.c_str());
  if(!in){
    cerr << "Error opening " << particles_filename << "\n";
    exit(1);
  }

  int nparticles;
  float min_x, min_y, max_x, max_y;
  in >> nparticles >> min_x >> min_y >> max_x >> max_y;
  in.get();
  float *data = new float[nparticles*3];
  in.read(reinterpret_cast<char*>(data), sizeof(float)*(nparticles*3));
  if(!in){
    cerr << "Error reading data from " << particles_filename << "\n";
    exit(1);
  }

  float max_dim = std::max(max_x-min_x, max_y-min_y)/8;
  maxBound = Vector( (max_x-min_x)/max_dim, (max_y-min_y)/max_dim, 1);

  Group *particles = new Group;
//    Material *particle_matl = new Lambertian(Color(RGB(.2,.6,.2)));

  Material *particle_matl =new Phong(heightMap// Color(RGB(0xCD/255.0, 0x7f/255.0,0x32/255.0))
                                     ,new Constant<Color>(Color(RGB(.7,.7,.7))),
                                     64, 0);
//   Material *particle_matl =new Phong(Color(RGB(0xCD/255.0, 0x7f/255.0,0x32/255.0)),
//                                      Color(RGB(.7,.7,.7)),
//                                      64,  (ColorComponent) 0.4);

  //Note, we add some randomness just to test the dynnamic scenes and interpolation...
  MT_RNG rng;
  for (int i = 0; i < nparticles; ++i) {
    Vector center( (data[i*3 + 0]-min_x)/max_dim*(rng.nextDouble()*.2+.9),
                   (data[i*3 + 1]-min_y)/max_dim*(rng.nextDouble()*.2+.9),
                   data[i*3 + 2]*2*(rng.nextDouble()*.2+.9));
    particles->add(new Sphere(particle_matl, center, .03));
  }

  return particles;
}

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext&, const vector<string>& args)
{
  std::cout << "Make_scene args: " << args.size() << std::endl;

  string heightfield_filename, particles_filename;
  for(size_t i=0;i<args.size();i++){
    string arg = args[i];
    if(arg == "-heightfield"){
      if(!getStringArg(i, args, heightfield_filename))
        throw IllegalArgument("scene vorpal -heightfield", i, args);
    }
    else if(arg == "-particles"){
      if(!getStringArg(i, args, particles_filename))
        throw IllegalArgument("scene vorpal -particles", i, args);
    } else {
      if(arg[0] == '-') {
        cerr << "Valid options for scene vorpal:\n";
        cerr << "-heightfield <filename>\n";
        cerr << "-particles <filename>\n";
        throw IllegalArgument("scene vorpal", i, args);
      }
    }
  }

  Group *group = new Group;

  Vector minBound(0, 0, 0), maxBound;
  HeightColorMap *heightMap = new HeightColorMap(0, 1);

  Group *particles1 = readParticles(particles_filename, maxBound, heightMap);

//   string particles2_filename = "/home/sci/thiago/work/Fusion/vorpal/code/cropped_particles_5e10_intel";
//   Group *particles2 = readParticles(particles_filename, maxBound, heightMap);

//   string particles3_filename = "/home/sci/thiago/work/Fusion/vorpal/code/cropped_particles_1e11_intel";
//   Group *particles3 = readParticles(particles_filename, maxBound, heightMap);

  Heightfield* heightfield = new Heightfield(NULL, heightfield_filename.c_str(), minBound, maxBound);
  heightfield->setMaterial(new Lambertian(heightMap));

//   string heightfield2_filename = "/home/sci/thiago/work/Fusion/vorpal/code/vorpal_10k_rand1.hf";
//   Heightfield* heightfield2 = new Heightfield(NULL, heightfield2_filename.c_str(), minBound, maxBound);
//   heightfield2->setMaterial(new Lambertian(heightMap));

//   string heightfield3_filename = "/home/sci/thiago/work/Fusion/vorpal/code/vorpal_10k_rand2.hf";
//   Heightfield* heightfield3 = new Heightfield(NULL, heightfield3_filename.c_str(), minBound, maxBound);
//   heightfield3->setMaterial(new Lambertian(heightMap));

  BBox bbox;
  PreprocessContext preprocessContext;
  heightfield->computeBounds(preprocessContext, bbox);
//   heightfield2->computeBounds(preprocessContext, bbox);
//   heightfield3->computeBounds(preprocessContext, bbox);

  heightMap->setRange(bbox.getMin().z(), bbox.getMax().z());


  KeyFrameAnimation *particle_animation = new KeyFrameAnimation(KeyFrameAnimation::truncate);

  particles1->add(heightfield);
//   particles2->add(heightfield2);
//   particles3->add(heightfield3);

  particle_animation->push_back(particles1);
//   particle_animation->push_back(particles2);
//   particle_animation->push_back(particles3);
  particle_animation->setDuration(10);
  particle_animation->useAccelerationStructure(new DynBVH());

//   KeyFrameAnimation *animation = new KeyFrameAnimation(KeyFrameAnimation::linear);
//   Group *frame1 = new Group();
//   Group *frame2 = new Group();
//   Group *frame3 = new Group();
//   frame1->add(heightfield);
//   frame2->add(heightfield2);
//   frame3->add(heightfield3);
//   animation->push_back(frame1);
//   animation->push_back(frame2);
//   animation->push_back(frame3);
//   animation->setDuration(10);
//   animation->startAnimation();
//   //animation->pauseAnimation();
//   animation->useAccelerationStructure(new DynBVH());
//   group->add(animation);

//   AccelerationStructure *as = new DynBVH;
//   as->rebuild(animation);
  group->add(particle_animation);


  AffineTransform modelTransform;
  modelTransform.initWithScale(Vector(10, 10, 10));
  modelTransform.rotate(Vector(1,0,0), 1.57);
  modelTransform.translate(Vector(4, 3.5, 0));


  // Material *model_matl =
  //new Phong(Color(RGB(.8, .4, .6)),
  //                              Color(RGB(.7,.7,.7)),
  //                              64, 0);
//     new Lambertian(Color(RGB(.2,.6,.2)));

//   Group *modelGroup = new Group;
//   string modelName = "/usr/sci/data/Geometry/Stanford_Sculptures/bun_zipper.ply";
//   if (!readPlyFile(modelName, modelTransform, modelGroup, 0, model_matl))
//        printf("error loading or reading ply file: %s\n", modelName.c_str()); //would be better to throw an error.


//   AccelerationStructure *staticScene = new DynBVH();
//   staticScene->rebuild(modelGroup);
//   group->add(staticScene);
  //  group->add(animation);

  particle_animation->startAnimation();
//   particle_animation->pauseAnimation();

  AccelerationStructure *fullScene = new DynBVH();
  fullScene->setGroup(group);
  fullScene->rebuild();
  Group *sceneGroup = new Group();
  sceneGroup->add(fullScene);


  Scene* scene = new Scene();
  scene->setBackground(new LinearBackground(Color(RGB(0.2, 0.4, 0.9)),
                                            Color(RGB(0.0,0.0,0.0)),
                                            Vector(0,1,0)));
  scene->setObject(sceneGroup);

  LightSet* lights = new LightSet();

  group->computeBounds(preprocessContext, bbox);
  Vector lightCenter =(maxBound + minBound)/2 + Vector(0,0,bbox.getMax().z()*4);

  const int NUM_LIGHT_SAMPLES_ROOT = 1;
  for (float i=0; i < NUM_LIGHT_SAMPLES_ROOT; ++i) {
    for (float j=0; j < NUM_LIGHT_SAMPLES_ROOT; ++j) {
      lights->add(new PointLight(lightCenter+1*Vector(i/NUM_LIGHT_SAMPLES_ROOT,
                                                      j/NUM_LIGHT_SAMPLES_ROOT,0),
                                 Color(RGB(1,1,1))*1/(NUM_LIGHT_SAMPLES_ROOT*NUM_LIGHT_SAMPLES_ROOT)));
    }
  }
  Color cup(RGB(0.3, 0.3, 0.3));
  Color cdown(RGB(0.1, 0.1, 0.1));
  Color grey(RGB(0.1, 0.1, 0.1));
  Vector up(0,1,0);
  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
//   lights->setAmbientLight(new ConstantAmbient(grey));
  scene->setLights(lights);
  return scene;
}
