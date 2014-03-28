
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>
#include <Interface/CameraPath.h>
#include <Interface/MantaInterface.h>

#include <fstream>
#include <iostream>

using namespace Manta;

CameraPath::CameraPath(MantaInterface* interface, string const& fname,
                       IOMode mode, LoopMode loop, int max_count, int offset) :
  interface(interface), fname(fname), loop(loop), max_count(max_count), offset(offset), handle(0),
  anim(false), frame(0), max_frame(0)
{
  if (mode==ReadKnots) {
    readKnots();
    anim=true;
    handle=interface->registerSerialAnimationCallback(Callback::create(this, &CameraPath::interpolate));
    count=0;
  }
}

CameraPath::~CameraPath(void)
{
  knots.clear();
  eyes.reset();
  lookats.reset();
  ups.reset();
  hfovs.reset();
  vfovs.reset();
}

void CameraPath::interpolate(int, int, bool&)
{
  // Loop control
  switch (loop) {
  case Count:
    if (count < max_count - 1 && frame >= max_frame) {
      frame = 0;
      ++count;
    }

    break;
  case CountAndQuit:
    if (count < max_count - 1 && frame >= max_frame) {
      frame = 0;
      ++count;
    }

    break;
  case Infinite:
  default:
    if (frame >= max_frame)
      frame = 0;

    break;
  };

  if (frame < max_frame) {
    Camera* camera=interface->getCamera(0);
    camera->setBasicCameraData(BasicCameraData(eyes.interpolate(frame),
                                               lookats.interpolate(frame),
                                               ups.interpolate(frame),
                                               hfovs.interpolate(frame),
                                               vfovs.interpolate(frame)));
    ++frame;
  } else {
    anim=false;
    interface->unregisterCallback(handle);
    cerr<<"Reached end of camera path";
    if (loop==CountAndQuit) {
      cerr<<"... quitting\n";
      interface->finish();
    } else
      cerr<<'\n';
  }
}

void CameraPath::animate(void)
{
  anim = !anim;

  if (anim)
    handle=interface->registerSerialAnimationCallback(Callback::create(this, &CameraPath::interpolate));
  else
    interface->unregisterCallback(handle);
}

void CameraPath::addKnot(unsigned int const knot_frame, BasicCameraData const& data)
{
  Knot knot;
  // Force a control point at frame zero (prevents camera from going berzerk
  // on playback)
  knot.frame=(knots.size() == 0)? 0: knot_frame;
  knot.data=data;
  knots.push_back(knot);

  // Add knot to paths
  eyes.addKnot(knot_frame, data.eye);
  lookats.addKnot(knot_frame, data.lookat);
  ups.addKnot(knot_frame, data.up);
  hfovs.addKnot(knot_frame, data.hfov);
  vfovs.addKnot(knot_frame, data.vfov);

  if (knot_frame > max_frame)
    max_frame = knot_frame;

  cout << "Adding knot at frame "<<knot_frame<<"\n";
}

void CameraPath::readKnots(void)
{
  // Open input file
  std::ifstream in(fname.c_str());
  if (!in)
    throw InputError("Failed to open \"" + fname + "\" for reading");

  // Read control point data
  unsigned int total_knots;
  in>>total_knots;

  max_frame=0;
  unsigned int nknots=0;
  while (nknots<total_knots) {
    unsigned int knot_frame;
    BasicCameraData data;
    
    // Frame number (+/- offset)
    in>>knot_frame;
    knot_frame += offset;

    // Camera parameters
    in>>data.eye[0]>>data.eye[1]>>data.eye[2];
    in>>data.lookat[0]>>data.lookat[1]>>data.lookat[2];
    in>>data.up[0]>>data.up[1]>>data.up[2];
    in>>data.hfov;
    in>>data.vfov;

    addKnot(knot_frame, data);
    
    ++nknots;
  }

  in.close();

  std::cerr<<"Read "<<nknots<<" control points from \""<<fname<<"\"\n";
}

void CameraPath::writeKnots(void) const
{
  if (knots.size()==0) {
    std::cerr<<"No control points have been added, ignoring\n";
    return;
  }

  // Open output file
  std::ofstream out(fname.c_str());
  if (!out)
    throw OutputError("Failed to open \"" + fname + "\" for writing");

  // Write data
  out<<knots.size()<<'\n';
  for (unsigned int i=0; i<knots.size(); ++i) {
    out<<knots[i].frame<<' '
       <<knots[i].data.eye<<' '
       <<knots[i].data.lookat<<' '
       <<knots[i].data.up<<' '
       <<knots[i].data.hfov<<' '
       <<knots[i].data.vfov<<'\n';
  }

  out.close();

  std::cerr<<"Wrote "<<knots.size()<<" control points to \""<<fname<<"\"\n";
}
