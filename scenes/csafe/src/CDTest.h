//! Filename: CDTest.h
/*!
 This file is a "bridge" between python code and c++ code to simplify dealing with
 templated classes, etc. for the csafe demo.
 */

#ifndef CDTEST_H
#define CDTEST_H

#include "CDGridSpheres.h"
#include "CSAFEObject.h"

#include <Model/Groups/RecursiveGrid.h>
//#include <Model/Materials/AmbientOcclusion.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/Volume.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Textures/ValueColormap.h>

#include <Core/Color/Color.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Geometry/Vector.h>
#include <Core/Util/Args.h>
#include <Core/Thread/Thread.h>

#include <Engine/Factory/Factory.h>

#include <Interface/Context.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/LightSet.h>
#include <Interface/MantaInterface.h>
#include <Interface/MantaInterface.h>

#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Cameras/PinholeCamera.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/MiscObjects/CuttingPlane.h>
#include <Model/MiscObjects/KeyFrameAnimation.h>
#include <Model/Primitives/Cube.h>
#include <Model/Readers/ParticleNRRD.h>
#include <Model/Readers/UDAReader.h>
#include <Model/Readers/VolumeNRRD.h>

#include <teem/nrrd.h>

#include <math.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#define PRECOMPUTE_GRIDS 1
#define USE_GRIDSPHERES 1

using namespace std;
using namespace Manta;

#if USE_GRIDSPHERES
typedef CDGridSpheres GridType;
#else
  typedef RecursiveGrid GridType;
#endif


/*!
  \class CDTest
  \brief c++ Bridge for running csafe code
  \author Carson Brownlee
  Description: used for running c++ heavy code for csafe demo
*/
class CDTest
{
 public:
  //! CDTest constructor
  /*!
    \param pass in the scene, or NULL and set it later
    \param pass in MantaInterface used for rendering
  */
  CDTest(Scene* scene, MantaInterface* interface, Vector  volumeMinBound, Vector volumeMaxBound)
    {
      _sphereAnimation=NULL;
      _volCMap=NULL;
      _volAnimation = NULL;
      _scene = scene;
      _ridx = 6;
      _cidx = 4;
      _radius = 0.0003;
      numFrames = numFrames1 = numFrames2 = 0;
      //_readContext = context;
      _manta_interface = interface;
      _forceDataMin = -FLT_MAX;
      _forceDataMax = -FLT_MAX;
      _useAO = false;
      _spheresVisible = true;
      _volVisible = true;
      _sphereCMap = NULL;
      _sphereDMins = new float[16];
      _sphereDMaxs = new float[16];
      _minBound = volumeMinBound;
      _maxBound = volumeMaxBound;
      _stepSize = 0.0015;
      _num_vars = 0;
      _forceVolumeData = false;
      _is_loaded = false;
    }

  ~CDTest()
    {
      delete[] _sphereDMins;
      delete[] _sphereDMaxs;
    }

  //! sets the scene object, necessary for initializing scene and loading spheres/volume
  /*!
    \param scene to set
  */
  void setScene(Scene* scene)
    {
      _scene = scene;
    }

  //! initializes the scene, must do before rendering
  /*!
   */
  void initScene()
    {
      if (!_sphereCMap)
    _sphereCMap = LinearColormap<float>::createColormap(Rainbow,400,600);
      ValueColormap<float>* vcmap = new ValueColormap<float>(_sphereCMap);
      _sphereMatl = new Phong(vcmap,vcmap, 10, NULL);
      _world = new Group();
      _scene->setBackground(new ConstantBackground(Color(RGB(.5, .5, .5))));
      _scene->setObject(_world);

      LightSet* lights = new LightSet();
      lights->add(new PointLight(Vector(-500, 300, -300), Color(RGB(.8,.8,.8))));

      lights->setAmbientLight(new ConstantAmbient(Color(RGB(.4,.4,.4))));
      _scene->setLights(lights);
      Group*   group = new Group();
      Primitive* prim = new Cube( new Lambertian(Color(RGBColor(1,0,0))), Vector(-0.1, -0.1, 0.4)*0, Vector(0.1, 0.1, 0.7)*0);
      if (_sphereAnimation)
    delete _sphereAnimation;
      _sphereAnimation= new KeyFrameAnimation();
#if !PRECOMPUTE_GRIDS
      _sphereAnimation->useAccelerationStructure(new RecursiveGrid(3));
#endif
      if (_volAnimation)
    delete _volAnimation;
      _volAnimation = new KeyFrameAnimation();
      Primitive* prim2 = new Cube( new Lambertian(Color(RGBColor(0,0,1))), Vector(-0.1, -0.1, 0.4), Vector(0.1, 0.1, .7));
      group->add(prim);
      Group* group2 = new Group();
      group2->add(prim2);
      _world->add(_sphereAnimation);
      _world->add(_volAnimation);
      _sphereAnimation->setDuration(15);
      _volAnimation->setDuration(15);
      duration = 10;
      numFrames1 = numFrames2 = numFrames = 1;


      if (!_volCMap)
    {
      vector<ColorSlice> slices;
      float div = 1.0/255.0;
      float a = 0.1;
      slices.push_back(ColorSlice(0.0f, RGBAColor(Color(RGB(0, 0, 0)) * div, 0*a)));
      slices.push_back(ColorSlice(0.109804f, RGBAColor(Color(RGB(52, 0, 0)) * div, 0*a)));
      slices.push_back(ColorSlice(0.01f, RGBAColor(Color(RGB(102, 2, 0)) * div, 0.1*a)));
      slices.push_back(ColorSlice(0.328571f, RGBAColor(Color(RGB(153, 18, 0)) * div, 0.216667*a)));
      slices.push_back(ColorSlice(0.4f, RGBAColor(Color(RGB(200, 41, 0)) * div, 0.23*a)));
      slices.push_back(ColorSlice(0.5f, RGBAColor(Color(RGB(230, 71, 0)) * div, 0.27*a)));
      slices.push_back(ColorSlice(0.618367f, RGBAColor(Color(RGB(255, 120, 0)) * div, 0.3375*a)));
      slices.push_back(ColorSlice(0.68f, RGBAColor(Color(RGB(255, 163, 20)) * div, 0.35*a)));
      slices.push_back(ColorSlice(0.72f, RGBAColor(Color(RGB(255, 204, 55)) * div, 0.37*a)));
      slices.push_back(ColorSlice(0.79f, RGBAColor(Color(RGB(255, 228, 80)) * div, 0.39*a)));
      slices.push_back(ColorSlice(0.85f, RGBAColor(Color(RGB(255, 247, 120)) * div, 0.43*a)));
      slices.push_back(ColorSlice(0.92f, RGBAColor(Color(RGB(255, 255, 180)) * div, 0.47*a)));
      slices.push_back(ColorSlice(1.0f, RGBAColor(Color(RGB(255, 255, 255)) * div, 0.5*a)));

      _volCMap = new RGBAColorMap(slices, 64);
    }
      //Group* cutWorld = new Group();
      //cutWorld->add(new CuttingPlane(Vector(0,0,0), Vector(0,1,0), _world));
      //_scene->setObject(cutWorld);
      _sphereAnimationCut = new Group();
      _cuts[0] = new CuttingPlane(Vector(0,1,0), Vector(0,1,0), _sphereAnimation);
      _cuts[1] = new CuttingPlane(Vector(0,0,0), Vector(0,-1,0), _cuts[0]);
      _cuts[2] = new CuttingPlane(Vector(0,0,0), Vector(-1,0,0), _cuts[1]);
      _cuts[3] = new CuttingPlane(Vector(0,0,0), Vector(1,0,0), _cuts[2]);
      _cuts[4] = new CuttingPlane(Vector(0,0,0), Vector(0,0,1), _cuts[3]);
      _cuts[5] = new CuttingPlane(Vector(0,0,0), Vector(0,0,-1), _cuts[4]);
      _sphereAnimationCut->add(_cuts[0]);
    }
  void setVolumePositionSize(Vector position, Vector size)
    {
      _volPosition = position;
      _volSize = size;
      _minBound = position - size/2.0;
      _maxBound = position + size/2.0;
      _manta_interface->addOneShotCallback(MantaInterface::Relative, 2, Callback::create(this, &CDTest::setVolumePositionSizeHelper));
    }
  void setVolumeMinMaxBounds(Vector min, Vector max)
    {
      _minBound = min;
      _maxBound = max;
      Vector pos = min + (max-min)/2.0;
      Vector size = max - min;
      setVolumePositionSize(pos,size);
    }
  Vector tempMin, tempMax;
  void setClippingBBox(Vector min, Vector max)
    {
      tempMin = min; tempMax = max;
      //_manta_interface->addOneShotCallback(MantaInterface::Relative, 1, Callback::create(this, &CDTest::setClippingBBoxHelper));
    }
  void setClippingBBoxHelper(int, int)
    {
      /*Vector min = tempMin;
      Vector max = tempMax;
      for(int i =0; i < 3; i++)
        setClipMinMax(i, min[i], max[i]);
      Vector minB(_minBound);
      Vector maxB(_maxBound);
      for(int i=0;i<3;i++)
    {
      minB[i] = std::max(min[i], minB[i]);
      maxB[i] = std::min(max[i], maxB[i]);
          if (minB[i] >= maxB[i])
            minB[i] = maxB[i] - T_EPSILON;
    }
      for(int i = 0; i < int(_volPrims.size()); i++)
    {
          BBox bounds = _vols[i]->getBounds();
          Vector tmin = minB;
          Vector tmax = maxB;
          for(int j = 0; j < 3; j++){
            tmin[j] = std::max(minB[j], bounds[0][j]) + T_EPSILON;
            tmax[j] = std::min(maxB[j], bounds[1][j]) - T_EPSILON;
          }
      _volPrims[i]->setMinMax(tmin, tmax);
          }*/
    }
  void useClippingBBox(bool st)
    {
      /* _useClippingBBox = st;
     if (st == true)
     {
     _world = new Group();
     if (_volVisible)
     _world->add(_volAnimation);
     if (_spheresVisible)
     _world->add(_sphereAnimationCut);
     _scene->setObject(_world);
     }
     else
     {
     _world = new Group();
     if (_volVisible)
     _world->add(_volAnimation);
     if (_spheresVisible)
     _world->add(_sphereAnimation);
     }*/
    }
  void setVisibility(bool spheres, bool volume)
    {
      _spheresVisible = spheres;
      _volVisible = volume;
      _world = new Group();
      if (spheres)
    {
      _world->add(_sphereAnimation);
      /*if(_useClippingBBox)
        _world->add(_sphereAnimationCut);
            else
            _world->add(_sphereAnimation);*/
    }
      if (volume)
    _world->add(_volAnimation);
      _scene->setObject(_world);
    }

  //! add a sphere file to be loaded
  /*!
    \param file to be loaded later
  */
  void addSphereNrrd(string file)
    {
      _nrrdFilenames.push_back(file);
    }

  //! clear list of sphere files
  /*!
   */
  void clearSphereNrrds()
    {
      _nrrdFilenames.clear();
      numFrames1 = 0;
      numFrames = numFrames2;
    }

  //! add volume nrrd file to be loaded
  /*!
    \param file to add to be loaded later
  */
  void addVolNrrd(string file)
    {
      _nrrdFilenames2.push_back(file);
    }

  //! clear list of volume files
  /*!
   */
  void clearVolNrrds()
    {
      _nrrdFilenames2.clear();
      numFrames2 = 0;
      numFrames = numFrames1;
    }

  //! load list of sphere nrrd files
  /*!
   */
  void loadSphereNrrds()
    {
      LightSet* lights = _scene->getLights();
      PreprocessContext context(_manta_interface, 0, 1, lights);
      _nrrds.clear();
      for(int i = 0; i < 16; i++) {
    _sphereDMins[i] = -FLT_MAX;
    _sphereDMaxs[i] = FLT_MAX;
      }

      for(vector<string>::iterator itr = _nrrdFilenames.begin(); itr != _nrrdFilenames.end(); itr++)
    {
      cout << "Loading Nrrd file: " << *itr << "...\n";
      ParticleNRRD* pnrrd = new ParticleNRRD();
      pnrrd->readFile(*itr);
          _num_vars = int(pnrrd->getNVars());
      if (_cidx >= int(pnrrd->getNVars())) {
        cerr << "error: color index not in data, settting to index 0\n";
        _cidx = 0;
      }
      if (_ridx >= int(pnrrd->getNVars())) {
        cerr << "error: radius index not in data, setting to constant radius\n";
        _ridx = -1;
      }
      if (pnrrd->getNVars() < 3) {
        cerr << "fatal IO error: expected a sphere file with at least 3 data indices\n";
        exit(2);
      }
          int xindex = 0;
          key_value_pairs = pnrrd->getKeyValuePairs();
          for(size_t i =0; i < key_value_pairs.size(); i++) {
            if (key_value_pairs[i].first == "p.x (x) index") {
              stringstream stream(key_value_pairs[i].second);
              stream >> xindex;
              cout << "found xindex: " << xindex << endl;
            }
          }
      _spherePNrrds.push_back(pnrrd);
      Group* group = new Group();

#if USE_GRIDSPHERES
      RGBAColorMap* cmap = new RGBAColorMap(1);
      CDGridSpheres* grid = new CDGridSpheres(pnrrd->getParticleData(), pnrrd->getNParticles(), pnrrd->getNVars(), 4, 2,_radius, _ridx, cmap , _cidx, xindex);
      //grid->setCMinMax(4, 299.50411987304688, 500.59423828125);
      //Material* matl = new Phong(Color(RGB(1,0,0)), Color(RGB(1,0,0)), 10);
      //Material* matl = new AmbientOcclusion(colormap, 0.01, 10);
      //grid->preprocess(context);

#else
      Group* g = new Group();
      _sphereGroups.push_back(g);
      CSAFEPrim::init(pnrrd->getNVars(), _sphereDMins, _sphereDMaxs, _cidx);
      float radius = _radius;
      for(size_t i = 0; i < pnrrd->getNParticles(); i++) {
        float* data = pnrrd->getParticleData() + i*pnrrd->getNVars();
        if (_ridx > -1)
          radius = data[_ridx];
            g->add(new Sphere(new Lambertian(Color(RGB(1,0,0))), Vector(data[xindex], data[xindex+1], data[xindex+2]), radius));
        //g->add(new CSAFEPrim(new Sphere(_sphereMatl, Vector(data[xindex],data[xindex+1],data[xindex+2]),radius), data));
        //      g->add( new ValuePrimitive<float>(new Sphere(matl, Vector(data[0],data[1],data[2]),data[_ridx]), data[_cidx]));
        //              g->add(new Sphere(matl, Vector(data[0], data[1], data[2]), data[_ridx]));
      }
#if PRECOMPUTE_GRIDS
      RecursiveGrid* grid = new RecursiveGrid();
      //g->preprocess(context);
      //ParticleGrid* grid = new ParticleGrid(pnrrd, _radius, _ridx, _cidx);
      grid->setGroup(g);
      //grid->preprocess(context);
      cout << "building grid\n";
      grid->rebuild();
      cout << "done building grid\n";
#endif
#endif  //USE_GRIDSPHERES

      group->add(grid);
      _sphereGrids.push_back(grid);
#if PRECOMPUTE_GRIDS
      _sphereAnimation->push_back(group);
#else
      _sphereAnimation->push_back(g);
#endif  //PRECOMPUTE_GRIDS
      if (pnrrd->getNVars() > _sphereMins.size())
        {
          for(size_t j = _sphereMins.size(); j < pnrrd->getNVars(); j++)
        {
          _sphereMins.push_back(FLT_MAX);
          _sphereMaxs.push_back(-FLT_MAX);
        }
        }
      for(int j = 0; j < int(_sphereMins.size()); j++)
        {
          float min,max;
              grid->getMinMax(j, min, max);
          _sphereMins[j] = Min(_sphereMins[j], min);
          _sphereMaxs[j] = Max(_sphereMaxs[j], max);
        }
      //_sphereAnimation->useAccelerationStructure(grid);
      numFrames1++;
      //Nrrd *new_nrrd = nrrdNew();
      ////////////////////////////////////////////////////////////////////////////
      // Attempt to open the nrrd
      //if (nrrdLoad( new_nrrd, i->c_str(), 0 )) {
      //  char *reason = biffGetDone( NRRD );
      //  std::cout << "WARNING Loading Nrrd Failed: " << reason << std::endl;
      //  exit(__LINE__);
      // }

      // Check to make sure the nrrd is the proper dimensions.
      // if (new_nrrd->dim != 3) {
      //  std::cout << "WARNING Nrrd must three dimension RGB" << std::endl;
      //  exit(__LINE__);
      //  }

      //_nrrds.push_back(new_nrrd);
      cout << "Loading " << *itr << " done.\n";
    }
      //  _sphereAnimation->preprocess(context);
      updateFrames();
      if (_clipFrames && _endFrame < int(_sphereGrids.size()))
    _sphereAnimation->clipFrames(_startFrame, _endFrame);
    }
  //! clear list of volume files
  /*!
   */
  void loadVolNrrds()
    {
      for(vector<string>::iterator i = _nrrdFilenames2.begin(); i != _nrrdFilenames2.end(); i++)
    {
      cout << "Loading Nrrd file: " << *i << "..." << endl;
      Group* group = new Group();
          vector<pair<string, string> > keyValuePairs;
          Vector min = _minBound;
          Vector max = _maxBound;
          GridArray3<float>* grid = loadNRRDToGrid<float>(*i, &keyValuePairs);
          for(vector<pair<string, string> >::iterator itr = keyValuePairs.begin(); itr != keyValuePairs.end(); itr++) {
            if (itr->first == "extents") {
              cout << "parsing keyvalue: \"" << itr->first << "\" \"" << itr->second
                   << "\"\n";
              sscanf(itr->second.c_str(), "[%f %f %f]..[%f %f %f]",
                   &min[0], &min[1], &min[2], &max[0], &max[1], &max[2]);
              printf("\nfound extent: %f %f %f .. %f %f %f\n", min[0], min[1],
                    min[2], max[0], max[1], max[2]);
              _minBound = min;
              _maxBound = max;
              _volSize = max - min;
              _volPosition = min + _volSize/2.0;
            }
          }
          Volume<float>* mat;
          //if (_forceVolumeData) {
          //  cout << "forcing volumemin and max \n";
          //  mat = new Volume<float>(grid, _volCMap, BBox(min, max), _stepSize, 3, _forceDataMin, _forceDataMax);
          // }
          //else
          // {
          mat = new Volume<float>(grid, _volCMap, BBox(min, max), _stepSize, 3);
          // }
      Cube* vol = new Cube(mat, min, max);
          group->add(vol);
          //#  group->add(vol);
      _volAnimation->push_back(group);
      _vols.push_back(mat);
      _volPrims.push_back(vol);
      numFrames2++;
      cout << "done Loading " << *i << ".\n";
    }
      updateFrames();
      if (_clipFrames && _endFrame < int(_vols.size()))
    _volAnimation->clipFrames(_startFrame, _endFrame);
      setClippingBBox(Vector(-1,-1,-1), Vector(1,1,1));
    }
  void readUDAHeader(string directory)
    {
      uda.readUDAHeader(directory);
    }
  int getUDANumVars() { return uda.getNumVariables(); }
  string getUDAVarName(int i) { return uda.getVarHeader(i).name; }
  void loadUDA(string file, string volName)
    {
      //TODO: clear out existing data
      LightSet* lights = _scene->getLights();
      PreprocessContext context(_manta_interface, 0, 1, lights);
      uda.readUDA(file, volName);
      for (std::vector<UDAReader::Timestep>::iterator itr =  uda.timesteps.begin(); itr != uda.timesteps.end(); itr++)
    {
      float* sdata = itr->sphereData;

      for (int i = 0; i < 100; i++)
        {
              cout << "sphere data: " << sdata[i] << endl;
        }
      Group* group = new Group();
      RGBAColorMap* cmap = new RGBAColorMap(1);
      int ridx = _ridx;
      if (ridx >= itr->numSphereVars)
        ridx = -1;
      int cidx = _cidx;
      if (cidx >= itr->numSphereVars)
        cidx = 0;
      CDGridSpheres* grid = new CDGridSpheres(itr->sphereData, itr->numSpheres, itr->numSphereVars, 6, 2,_radius, ridx, cmap , cidx);
#if USE_GRIDSPHERES
      group->add(grid);
      _sphereGrids.push_back(grid);
#endif
      if (itr->numSphereVars > int(_sphereMins.size()))
        {
          for(int j = int(_sphereMins.size()); j < itr->numSphereVars; j++)
        {
          _sphereMins.push_back(FLT_MAX);
          _sphereMaxs.push_back(-FLT_MAX);
        }
        }
      for(int j = 0; j < int(_sphereMins.size()); j++)
        {
          float min,max;
          grid->getMinMax(j, min, max);
          if (_sphereMins[j] > min)
        _sphereMins[j] = min;
          if (_sphereMaxs[j] < max)
        _sphereMaxs[j] = max;
        }
      _sphereAnimation->push_back(group);
      numFrames1++;

      Group* group2 = new Group();
      Volume<float>* mat = new Volume<float>(itr->volume, _volCMap, BBox(_minBound, _maxBound), 0.00125, 3, _forceDataMin, _forceDataMax);
      Cube* vol = new Cube(mat, _minBound - Vector(0.001, 0.001, 0.001), _maxBound + Vector(0.001, 0.001, 0.001));
      group2->add(vol);
      _volAnimation->push_back(group2);
      _vols.push_back(mat);
      _volPrims.push_back(vol);
      numFrames2++;
    }
      updateFrames();
      if (_clipFrames && _endFrame < int(_vols.size()))
    _volAnimation->clipFrames(_startFrame, _endFrame);
      if (_clipFrames && _endFrame < int(_sphereGrids.size()))
    _sphereAnimation->clipFrames(_startFrame, _endFrame);
    }

  //! calls destructors on all loaded data, then reloads sphere and volume files
  /*!
   */
  void reloadData()
    {
      //_scene = new Scene();
      //TODO: i don't delete the old scene object... I don't want to to do a recursive delete
      //_manta_interface->setScene(_scene);
      //initScene();
      //_volsOld = _vols;
      //_sphereGridsOld = _sphereGrids;
      //_spherePNrrdsOld = _spherePNrrds;
      //_vols.clear();
      //_spherePNrrds.clear();
      //_sphereGrids.clear();
      _is_loaded = false;
      loadSphereNrrds();
      loadVolNrrds();
      _is_loaded = true;
      //_manta_interface->addOneShotCallback(MantaInterface::Relative, 2, Callback::create(this, &CDTest::reloadDataHelper));
    }



  //! update the frame cieling
  /*!
   */
  void updateFrames()
    {
      numFrames = std::max(numFrames1, numFrames2);
    }

  //! pause animation
  /*!
   */
  void pauseAnimation() { _sphereAnimation->pauseAnimation(); _volAnimation->pauseAnimation(); }

  //! resume animation
  /*!
   */
  void resumeAnimation() { _sphereAnimation->resumeAnimation(); _volAnimation->resumeAnimation();}

  //! go to a time in animation
  /*!
    \param time to go to
  */
  void gotoTime(float time) { _sphereAnimation->setTime(time); _volAnimation->setTime(time); }

  // 0-based
  void gotoFrame(int frame) {
    if (frame < 0 || frame >= numFrames)
      {
    cerr << "ERROR: frame: " << frame << " out of bounds\n";
    return;
      }
    float step;
    if (numFrames == 0)
      step = 0;
    else
      step = duration/float(numFrames);
    _sphereAnimation->setTime( step*float(frame) + 0.0001f);
    _volAnimation->setTime(step*float(frame) + 0.0001f);
  }

  void lockFrames(bool st) { _sphereAnimation->lockFrames(st); _volAnimation->lockFrames(st); }
  void loopAnimations(bool st) {_sphereAnimation->loopAnimation(st); _volAnimation->loopAnimation(st); }
  void clipFrames(int start, int end) {
    _startFrame = start; _endFrame = end;  _clipFrames = true;
    if (end < numFrames1) _sphereAnimation->clipFrames(start,end); if (end < numFrames2) _volAnimation->clipFrames(start,end);
  }
  void repeatLastFrame(float time) {
    _sphereAnimation->repeatLastFrameForSeconds(time); _volAnimation->repeatLastFrameForSeconds(time);
  }

  //! skip ahead one frame
  /*!
   */
  void forwardAnimation()
    {
      float step;
      if (numFrames == 0)
    step = 0;
      else
    step = duration/float(numFrames);
      float st = _sphereAnimation->getTime();
      st = std::max(st, 0.0f);
      float vt = _volAnimation->getTime();
      vt = std::max(vt, 0.0f);

      _sphereAnimation->setTime( _sphereAnimation->getTime() + step);
      _volAnimation->setTime(_volAnimation->getTime()+step);
    }

  //! skip back one frame
  /*!
   */
  void backAnimation()
    {
      float step;
      if (numFrames == 0)
    step = 0;
      else
    step     = duration/float(numFrames);
      _sphereAnimation->setTime( _sphereAnimation->getTime() - step);
      _volAnimation->setTime(_volAnimation->getTime()-step);
    }

  //! get the duration of the animation in seconds
  /*!
    \return duration in seconds of animation
  */
  float getDuration()
    {
      return duration;
    }

  //! set the duration of animation in seconds
  /*!
    \param number of seconds animation takes
  */
  void setDuration(float time)
    {
      duration = time;
      _sphereAnimation->setDuration(time);
      _volAnimation->setDuration(time);
    }

  //! computes histogram of spherefiles
  /*!
    \param index of data to compute histogram of
    \param number of buckets to compute, ie 100 bars
    \param an allocated list of ints to store the values, must be size numBuckets
    \param will return min
    \param will return max
  */
  void getHistogram(int index, int numBuckets, int* histValues, float* min, float* max)
    {
      for(int i = 0; i < numBuckets; i++)
    histValues[i] = 0;
      for(size_t i = 0; i < _spherePNrrds.size(); i++)
    {
      ParticleNRRD* pnrrd = _spherePNrrds[i];
      float dataMin = _sphereMins[index];
      float dataMax = _sphereMaxs[index];
      cout << "histo dataMin/Max: " << dataMin << " " << dataMax << endl;
      float scale;
      if (dataMin != dataMax)
        scale = (numBuckets-1)/(dataMax - dataMin);
      else
        continue;
      for(int j = 0; j < int(pnrrd->getNParticles()); j++)
        {
          float val = *(pnrrd->getParticleData() + j*pnrrd->getNVars() + index);
          int bucket = int((val-dataMin)*scale);
          histValues[bucket]++;
        }
    }
      if (int(_sphereMins.size()) > index)
    {
      *min = _sphereMins[index];
      *max = _sphereMaxs[index];
    }
      else
    {
      *min = -FLT_MAX;
      *max = FLT_MAX;
    }
    }

  //! compute histogram for volume data
  /*!
    \param number of buckets to compute, ie 100 bars
    \param an allocated list of ints to store the values, must be size numBuckets
    \param will return min
    \param will return max
  */
  void getVolHistogram(int numBuckets, int* histValues, float* min, float* max)
    {
      cout << "computing volume histogram for " << _vols.size() << " volumes\n";
      *min = FLT_MAX;
      *max = -FLT_MAX;
      for(int i = 0; i < numBuckets; i++)
    histValues[i] = 0;
      for (int i = 0; i < int(_vols.size()); i++)
    {
      if (_vols[i] == NULL)
        continue;
      int* histValues2 = new int[numBuckets];
      _vols[i]->computeHistogram(numBuckets, histValues2);
      for(int j = 0; j < numBuckets; j++)
        histValues[j] += histValues2[j];
      double nmin, nmax;
      _vols[i]->getMinMax(&nmin, &nmax);
      *min= std::min(float(nmin), *min);
      *max = std::max(float(nmax), *max);
      cout << "compared min/max: " << nmin << " " << nmax << endl;
    }
      if (int(_vols.size()) == 0) {
        *min = *max = 0.0;
      }
      cout << "histogram computed: min/max: " << *min << " " << *max << endl;
    }

  //! get the colormap used for the volume
  /*!
    \return colormap used for volume
  */
  RGBAColorMap* getVolCMap() { return _volCMap; }

  //! set the colormap for volume
  /*!
    \param colormap
  */
  void setVolCMap(RGBAColorMap* map) { _volCMap = map;
  for(vector<Volume<float>*>::iterator i = _vols.begin(); i != _vols.end(); i++)
    {
      (*i)->setColorMap(map);

    }
  }

  //! set the colormap for spheres
  /*!
    \param colormap
  */
  void setSphereCMap(RGBAColorMap* map)   {
#if USE_GRIDSPHERES
    for(int i =0; i < int(_sphereGrids.size()); i++)
      _sphereGrids[i]->setCMap(map);
#else
    _setCMapHelper = map;
    _manta_interface->addOneShotCallback(MantaInterface::Relative, 1, Callback::create(this, &CDTest::setSphereCMapHelper));
#endif
  }

  void updateSphereCMap() {
#if !USE_GRIDSPHERES
    _manta_interface->addOneShotCallback(MantaInterface::Relative, 1, Callback::create(this, &CDTest::setSphereCMapHelper));
#endif
  }

  //! called by setSphereCMap
  /*!
   */
  void setSphereCMapHelper(int, int)
    {
      std::vector<Color> colors;
      RGBAColorMap* map = _setCMapHelper;
      if (!map)
    return;
      for(int i = 0; i < map->GetNumSlices(); i++) {
    colors.push_back(map->GetSlice(i).color.color);
      }
      cout << "setting sphere colors\n";
      _sphereCMap->setColors(colors);
    }

  //! set minimum and maximum clipping region for a specific index into spheres
  /*!
    \param index into data
    \param min
    \param max
  */
  void setClipMinMax(int index, float min, float max)
    {
      if (index >= int(_sphereMins.size()))
    return;
#if USE_GRIDSPHERES
      for(int i = 0; i < int(_sphereGrids.size()); i++)
    if (_sphereGrids[i]) _sphereGrids[i]->setClipMinMax(index, min, max);
#else
      _sphereDMins[index] = min;
      _sphereDMaxs[index] = max;
      for(size_t i = 0; i < _sphereGroups.size(); i++) {
    for(size_t s = 0; s < _sphereGroups[i]->size(); s++) {
      CSAFEPrim* prim = dynamic_cast<CSAFEPrim*>(_sphereGroups[i]->get(s));
      if (prim)
        prim->updateOcclusion();
    }
      }
#endif
    }

  //! set the minimum and maximum data values used for normalizing color data
  /*!
    \param index into data
    \param min
    \param max
  */
  void setSphereColorMinMax(int index, float min, float max)   //set the min max for normalizing color data
    {
#if USE_GRIDSPHERES
      for(int i =0; i < int(_sphereGrids.size()); i++)
    _sphereGrids[i]->setCMinMax(index, min, max);
#else
      if (!_sphereCMap)
    return;
      cout << "setting color min/max to: " << min << " " << max << endl;
      _sphereCMap->setRange(min, max);
#endif
    }

  //! get the min and max being used for normalizing color values
  /*!
    \param index into data
    \param returns min
    \param returns max
  */
  void getSphereColorMinMax(int index, float& min, float& max)
    {
#if USE_GRIDSPHERES
      for(int i =0;i<int(_sphereGrids.size());i++)
    _sphereGrids[i]->getCMinMax(index, min, max);
#else
      min = _sphereCMap->getMin();
      max = _sphereCMap->getMax();
#endif

    }

  //! get the min and max of spheres
  /*!
    \param index into data
    \param returns min
    \param returns max
  */
  void getSphereDataMinMax(int index, float& min, float& max)
    {
#if USE_GRIDSPHERES
      for(int i =0;i<int(_sphereGrids.size());i++)
    _sphereGrids[i]->getCMinMax(index, min, max);
#else
      if(index >= int(_sphereMins.size()))
    {
      min = -FLT_MAX;
      max = FLT_MIN;
      return;
    }
      min = _sphereMins[index];
      max = _sphereMaxs[index];
#endif
    }

  //! set minimum and maximum data values for volume data, should be called before loading
  /*!
    \param min
    \param max
  */
  void setVolColorMinMax(float min, float max)
    {
      _forceVolumeData = true;
      _forceDataMin = min;
      _forceDataMax = max;
    }

  //! the clipping range of the volume narrows the range of the colormap
  /*!
    \param actual min
    \param actual max
    \param altered min
    \param altered max
  */
  void setVolClipMinMax(float globalMin, float globalMax, float min, float max)
    {
      if (_volCMap)
       _volCMap->squish(globalMin, globalMax, min, max);
    }

  //! get the min and max data values used for normalizing color values
  /*!
    \param returns min
    \param returns max
  */
  void getVolColorMinMax(float& min, float& max)
    {
      min = _forceDataMin;
      max = _forceDataMax;
    }

  //! get the min and max data values of the volume
  /*!
    \param returns min
    \param returns max
  */
  void getVolDataMinMax(float* min, float* max)
    {
      *min = FLT_MAX;
      *max = -FLT_MAX;
      for(int i =0;i<int(_vols.size());i++)
    {
      if (_vols[i] == NULL)
        continue;
      double min1, max1;
      _vols[i]->getMinMax(&min1, &max1);
      *min = std::min(*min, float(min1));
      *max = std::max(*max, float(max1));
    }
    }

  void setStepSize(float t)
    {
      _stepSize = t;
      for(int i =0;i<int(_vols.size());i++)
    {
      if (_vols[i] == NULL)
        continue;
          _vols[i]->setStepSize(t);
        }
      if (_volCMap)
        _volCMap->scaleAlphas(t);
    }

  //! sets radius index used for spheredata
  /*!
    \param radius index
  */
  void setRidx(int ridx) { _ridx = ridx; }

  //! get radius index
  /*!
    \return radius index
  */
  int getRidx() { return _ridx; }

  //! set default radius of spheres
  /*!
    \param radius
  */
  void setRadius(float radius) { _radius = radius; }

  //! get the default radius of spheres
  /*!
    \return default radius
  */
  float getRadius() { return _radius; }

  //! set color index, what index colormap uses
  /*!
    \param color index to use
  */
  void setCidx(int cidx) {
#if USE_GRIDSPHERES
    for(int i =0; i < int(_sphereGrids.size()); i++)
      _sphereGrids[i]->setCidx(cidx);
#else
    CSAFEPrim::setCIndex(cidx);
#endif
    _cidx = cidx;
  }

  int getFrame()
    {
      if(!_sphereAnimation)
        return 0;
      else
        return static_cast<int>(_sphereAnimation->getTime()/duration*numFrames);
    }

  int getNumKeyValuePairs()
    {
      return key_value_pairs.size();
    }

  string getKey(int i)
    {
      if (i >= getNumKeyValuePairs() || i < 0)
        return "";
      return key_value_pairs[i].first;
    }

  string getValue(int i)
    {
      if (i >= getNumKeyValuePairs() || i < 0)
        return "";
      return key_value_pairs[i].second;
    }
  bool isLoaded() { return _is_loaded; }

  //! get color index
  /*!
    \return color index
  */
  int getCidx() { return _cidx; }

  int getNumVars() { return _num_vars; }

  //! set to use ambient occlusion on spheres
  void useAO(bool st)
    {
#if USE_GRIDSPHERES

      for(int i = 0; i < int(_sphereGrids.size()); i++)
    _sphereGrids[i]->useAmbientOcclusion(st);
#endif
      if (st == _useAO)
    return;
      _useAO = st;
    }

  //! set the cutoff distance and number of generated rays
  /*!
    \param cutoff distance, distance to check if rays hit objects
    \param number of generated rays
  */
  void setAOVars(float cutoff, int numDirs)
    {
#if USE_GRIDSPHERES
      for(int i = 0; i < int(_sphereGrids.size()); i++)
    _sphereGrids[i]->setAmbientOcclusionVariables(cutoff, numDirs);
#endif
    }

 protected:
  //! called by reloadData
  /*!
   */
  void reloadDataHelper(int, int)
    {
      for (vector<Volume<float>*>::iterator itr = _volsOld.begin(); itr != _volsOld.end(); itr++)
    delete *itr;
      _volsOld.clear();
#if USE_GRIDSPHERES
      for (vector<CDGridSpheres*>::iterator itr = _sphereGridsOld.begin(); itr != _sphereGridsOld.end(); itr++)
    delete *itr;
#endif
      _sphereGridsOld.clear();
      for (vector<ParticleNRRD*>::iterator itr = _spherePNrrdsOld.begin(); itr != _spherePNrrdsOld.end(); itr++)
    delete *itr;
      _spherePNrrdsOld.clear();
    }
  void setVolumePositionSizeHelper(int, int)
    {
      BBox bounds(_volPosition - _volSize/2.0, _volPosition + _volSize/2.0);
      for(vector<Volume<float>*>::iterator itr = _vols.begin(); itr != _vols.end(); itr++) {
    (*itr)->setBounds(bounds);
      }
      for(vector<Cube*>::iterator itr = _volPrims.begin(); itr != _volPrims.end(); itr++) {
    (*itr)->setMinMax(bounds.getMin(), bounds.getMax());
      }
    }

  Group* _sphereAnimationCut;
  CuttingPlane* _cuts[6]; //up down left right forward back
  bool _useClippingBBox, _spheresVisible, _volVisible;
  MantaInterface* _manta_interface;
  RGBAColorMap* _volCMap;
  LinearColormap<float>* _sphereCMap;
  Material* _sphereMatl;
  Vector _minBound;
  Vector _maxBound;
  float _forceDataMin;
  float _forceDataMax;
  int _cidx;
  int _ridx;
  float _radius;
  float duration;  //number of seconds for animation
  int numFrames;   //number of keyframes
  int numFrames1, numFrames2;  //number of frames for sphere/vol data
  ReadContext* _readContext;
  vector<string> _nrrdFilenames;
  vector<string> _nrrdFilenames2;
  vector<Nrrd*> _nrrds;
  vector<ParticleNRRD*> _spherePNrrds;
  vector<GridType*> _sphereGrids;
  vector<Volume<float>*> _vols;
  vector<Cube*> _volPrims;
  vector<ParticleNRRD*> _spherePNrrdsOld;
  vector<GridType*> _sphereGridsOld;
  vector<Volume<float>*> _volsOld;
  vector<Group*> _sphereGroups;
  vector<float> _sphereMins;
  vector<float> _sphereMaxs;
  Group* _world;
  Scene* _scene;
  KeyFrameAnimation* _sphereAnimation;
  KeyFrameAnimation* _volAnimation;
  int _startFrame, _endFrame, _clipFrames;
  bool _useAO;
  RGBAColorMap* _setCMapHelper;
  UDAReader uda;
  float* _sphereDMins, *_sphereDMaxs;
  Vector _volPosition, _volSize;
  float _stepSize;
  int _num_vars;
  bool _forceVolumeData;
  std::vector<std::pair<string, string> > key_value_pairs;
  bool _is_loaded;
};

#endif
