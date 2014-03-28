/* File: volumeTest.cc
 * Author: Carson Brownlee (brownleeATcs.utah.edu)
 * Description: test for basic single ray volume renderer
 * example usage: bin/manta -scene "lib/libscene_volumeTest.so (-i /home/collab/brownlee/Data-Explosion/vol/temp_CC_M02_0496.nrrd -dataMinMax 300 2100)" -np 4

 *
*/
#include <Core/Color/Color.h>
#include <Core/Color/ColorDB.h>
#include <Core/Color/RegularColorMap.h>
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
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Cameras/PinholeCamera.h>
#include <Model/Groups/Group.h>
#include <Model/Instances/Instance.h>
#include <Model/Instances/InstanceRST.h>
#include <Model/Instances/InstanceRT.h>
#include <Model/Instances/InstanceST.h>
#include <Model/Instances/InstanceT.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Checker.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/Volume.h>
#include <Model/MiscObjects/CuttingPlane.h>
#include <Model/MiscObjects/Difference.h>
#include <Model/MiscObjects/Intersection.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/Disk.h>
#include <Model/Primitives/Heightfield.h>
#include <Model/Primitives/Hemisphere.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/SuperEllipsoid.h>
#include <Model/TexCoordMappers/LinearMapper.h>
#include <Model/TexCoordMappers/SphericalMapper.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Readers/VolumeNRRD.h>
//#include <Model/Textures/NormalTexture.h>

#include <iostream>

#include <math.h>
#include <string>
#include <vector>
#include <fstream>

using namespace Manta;
using namespace std;

MANTA_PLUGINEXPORT
Scene* make_scene(const ReadContext& context, const vector<string>& args)
{
  double t_inc = 0.00125;
  double x,y,z;
  double dataMin = -FLT_MAX;
  double dataMax = -FLT_MAX;
  Vector minBound(-0.001, -0.101, -0.001);
  Vector maxBound(0.101, 0.201, 0.101);
  string cMapType = "";
  string cMapFile = "";
  int depth = 2;
  string volFile = "";
  RGBAColorMap* cMap = NULL;
  size_t argc = args.size();
  vector<ColorSlice> slices;
  cout << "argc: " << argc << endl;
  for(size_t i = 0; i < argc; ++i) {
    string arg = args[i];
    if(arg == "-t")
    {
        if(!getDoubleArg(i, args, t_inc))
                throw IllegalArgument("scene volumeTest -t",i,args);
    }
    else if(arg == "-dataMinMax")
    {
        if(!(getDoubleArg(i, args, dataMin) && getDoubleArg(i, args, dataMax)))
                throw IllegalArgument("scene volumeTest -dataMinMax",i,args);
        cout << "datamin/max: " << dataMin << " " << dataMax << endl;
    }
    else if(arg == "-minBound")
    {
        if(!(getDoubleArg(i, args, x) && !getDoubleArg(i, args, y) && !getDoubleArg(i, args, z)))
                throw IllegalArgument("scene volumeTest -minBound",i,args);
        minBound = Vector(x,y,z);
    }
    else if(arg == "-maxBound")
    {
        if(!(getDoubleArg(i, args, x) && !getDoubleArg(i, args, y) && !getDoubleArg(i, args, z)))
                throw IllegalArgument("scene volumeTest -maxBound",i,args);
        maxBound = Vector(x,y,z);
    }
    else if(arg == "-i")
    {
        if(!getStringArg(i, args, volFile))
                throw IllegalArgument("scene volumeTest -i", i, args);
    }
    else if (arg == "-depth") {
      if (!getIntArg(i, args, depth))
        throw IllegalArgument("scene volumeTest -depth", i, args);
        cout << "i: " << i << endl;
    }
    else if (arg == "-cMapFile")
    {
        if(!getStringArg(i, args, cMapFile))
                throw IllegalArgument("scene volumeTest -cMapFile",i,args);
        ifstream in(cMapFile.c_str());
        float pos,r,g,b,a;
        while (in >> pos && in >> r && in >> g && in >> b && in >> a)
                slices.push_back(ColorSlice(pos, RGBAColor(r,g,b,a)));
        in.close();
        cMap = new RGBAColorMap(slices);
    }
    else if (arg == "-cMapType")
    {
        if(!getStringArg(i, args, cMapType))
                throw IllegalArgument("scene volumeTest -cMapType",i,args);
        if(cMapType == "Rainbow")
                cMap = new RGBAColorMap(RGBAColorMap::Rainbow);
        else if(cMapType == "InvRainbow")
                cMap = new RGBAColorMap(RGBAColorMap::InvRainbow);
        else if(cMapType == "BlackBody")
                cMap = new RGBAColorMap(RGBAColorMap::BlackBody);
        else if(cMapType == "InvBlackBody")
                cMap = new RGBAColorMap(RGBAColorMap::InvBlackBody);
        else if(cMapType == "GreyScale")
                cMap = new RGBAColorMap(RGBAColorMap::GreyScale);
        else if(cMapType == "InvGreyScale")
                cMap = new RGBAColorMap(RGBAColorMap::InvGreyScale);
        else if(cMapType == "InvRainbowIso")
                cMap = new RGBAColorMap(RGBAColorMap::InvRainbowIso);
        else
                throw IllegalArgument("scene volumeTest -cMapType",i,args);

    } else {
      cerr<<"Valid options for scene pcgt:\n";
      cerr<<"  -depth <int>       depth of macrocells (1-5 is good, 2 default)\n";
      cerr<<"  -i <string>        input filename\n";
      cerr<<"  -minBound <float> <float> <float> minimum corner of bounding box\n";
      cerr<<"  -maxBound <float> <float> <float> maximum corner of bounding box\n";
      cerr<<"  -t <float>         t increment value for volume\n";
      cerr<<"  -cMapType <string> type of colormap:  Rainbow/InvRainbow/BlackBody/InvBlackBody/GreyScale/InvRainbowISo/InvGreyScale\n";
      cerr<<"  -cMapFile <string> file containing colormap info, in format of position(0-1) r g b a\n";
      throw IllegalArgument("scene volumeTest", i, args);
    }
  }

        Group* group = new Group();

        float div = 1.0/255.0;
        float a = 1.0;

if(cMap == NULL)
{
  slices.push_back(ColorSlice(0.0f, RGBAColor(Color(RGB(0, 0, 0)) * div, 0*a)));
  slices.push_back(ColorSlice(0.11f, RGBAColor(Color(RGB(52, 0, 0)) * div, 0*a)));
  slices.push_back(ColorSlice(0.4f, RGBAColor(Color(RGB(200, 41, 0)) * div, 0.23*a)));
  slices.push_back(ColorSlice(0.5f, RGBAColor(Color(RGB(230, 71, 0)) * div, 0.27*a)));
  slices.push_back(ColorSlice(0.618367f, RGBAColor(Color(RGB(255, 120, 0)) * div, 0.3375*a)));
  slices.push_back(ColorSlice(0.68f, RGBAColor(Color(RGB(255, 163, 20)) * div, 0.35*a)));



  slices.push_back(ColorSlice(0.72f, RGBAColor(Color(RGB(255, 204, 55)) * div, 0.37*a)));
  slices.push_back(ColorSlice(0.79f, RGBAColor(Color(RGB(255, 228, 80)) * div, 0.39*a)));
  slices.push_back(ColorSlice(0.85f, RGBAColor(Color(RGB(255, 247, 120)) * div, 0.43*a)));
  slices.push_back(ColorSlice(0.92f, RGBAColor(Color(RGB(255, 255, 180)) * div, 0.47*a)));
  slices.push_back(ColorSlice(1.0f, RGBAColor(Color(RGB(255, 255, 255)) * div, 0.5*a)));

  cMap = new RGBAColorMap(slices);
};


  cMap->scaleAlphas(t_inc);
  Volume<float>* mat = new Volume<float>(loadNRRDToGrid<float>(volFile), cMap, BBox(minBound, maxBound), t_inc, depth, dataMin, dataMax);
  Primitive* prim = new Cube(mat, minBound, maxBound);
  group->add(prim);

  Scene* scene = new Scene();
  scene->setBackground(new LinearBackground(Color(RGB(0.2, 0.4, 0.9)),
                                            Color(RGB(0.0,0.0,0.0)),
                                            Vector(0,1,0)));
  scene->setObject(group);

  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(5,5,8), Color(RGB(1,1,1))*2));
  Color cup(RGB(0.1, 0.3, 0.8));
  Color cdown(RGB(0.82, 0.62, 0.62));
  Vector up(0,1,0);
  lights->setAmbientLight(new ArcAmbient(cup, cdown, up));
  scene->setLights(lights);


  return scene;
}

