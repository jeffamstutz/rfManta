
#include <Engine/PixelSamplers/TimeViewSampler.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/MantaInterface.h>
#include <MantaSSE.h>
#include <Core/Util/Args.h>
#include <Core/Exceptions/IllegalArgument.h>

#define USE_SCIRUN_TIME 0
#if USE_SCIRUN_TIME
#  include <Core/Thread/Time.h>
#else
#  include <Core/Util/CPUTime.h>
#endif

#include <iostream>

using namespace Manta;

PixelSampler* TimeViewSampler::create(const vector<string>& args)
{
  return new TimeViewSampler(args);
}

TimeViewSampler::TimeViewSampler(const vector<string>& args)
  : child(0),
    scale(1.e5),
    state(0),
    active(true)
{
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-scale"){
      if(!getArg<ColorComponent>(i, args, scale))
        throw IllegalArgument("TimeViewSampler -scale", i, args);
    } else if (arg == "-help") {
      usage();
    }
    else {
      usage();
      throw IllegalArgument("TimeViewSampler", i, args);
    }
  }
}

void TimeViewSampler::usage()
{
  cerr << "TimeViewSampler args:\n";
  cerr << "\t-scale <Real>      - defaults to 100,000 (1e5)\n";
}

TimeViewSampler::~TimeViewSampler()
{
}

void TimeViewSampler::setupBegin(const SetupContext& context, int numChannels)
{
  if (child) child->setupBegin(context, numChannels);
}

void TimeViewSampler::setupDisplayChannel(SetupContext& context)
{
  if (child) child->setupDisplayChannel(context);
}

void TimeViewSampler::setupFrame(const RenderContext& context)
{
  if (child) child->setupFrame(context);
}

void TimeViewSampler::renderFragment(const RenderContext& context,
                                     Fragment& fragment)
{
  double startTime = 0;
  if (active) {
#if USE_SCIRUN_TIME
    startTime = Time::currentSeconds();
#else
    startTime = CPUTime::currentSeconds();
#endif
  }
  if (child) child->renderFragment(context, fragment);
  if (active) {
#if USE_SCIRUN_TIME
    double endTime = Time::currentSeconds();
#else
    double endTime = CPUTime::currentSeconds();
#endif
    double deltaTime = (endTime - startTime);
    // Make sure to normalize the color based on the number of
    // fragments.  Note that if there are no fragments (must be a
    // really rare case), you will end up with a inf for color.
    // That's OK, because you won't use it in that case.
    for(int i=fragment.begin();i<fragment.end();++i) {

      if (state == 0 && Color::NumComponents == 3) {
        float R, G, B;
        float x = deltaTime*scale/(fragment.end()-fragment.begin());
        float S = 1;
        if (x > 2) //set to x>1 to go straight to white.
          S = 0;
        else if (x > 1) {
          S = 2-x;
          x = 1;
        }
        HeightColorMap::hsv_to_rgb(R, G, B, x, S, x);
        fragment.color[0][i] = R;
        fragment.color[1][i] = G;
        fragment.color[2][i] = B;
      }
      else {
        ColorComponent color = deltaTime*scale/(fragment.end()-fragment.begin());
        for ( int c = 0; c < Color::NumComponents; c++ )
          fragment.color[c][i] = color;
      }
    }
  }
}

void TimeViewSampler::setPixelSampler(PixelSampler* new_child)
{
  child = new_child;
}
    
void TimeViewSampler::setScale(ColorComponent new_scale)
{
  scale = new_scale;
}

void TimeViewSampler::setActive(bool activeOn)
{
  active = activeOn;
}

// This will turn on and off the timeView display on the
// manta_interface.  Both parameters must be non-NULL.  If
// non-NULL the function returns the TimeViewSampler associated
// with the manta_interface, otherwise it returns timeView.
TimeViewSampler*
TimeViewSampler::toggleTimeView(MantaInterface* manta_interface,
                                TimeViewSampler* timeView)
{
  ASSERT(manta_interface != NULL);
  ASSERT(timeView != NULL);
  PixelSampler* ps = manta_interface->getPixelSampler();
  // Check to see if there is a TimeViewSampler already running
  TimeViewSampler* tvps = dynamic_cast<TimeViewSampler*>(ps);
  if (tvps != NULL) {
    // We have a winner.  Let's update the state and swap it out if
    // it's old.
    timeView = tvps;
    if (++timeView->state >= NUM_STATES) {
      ps = tvps->getPixelSampler();
      // If the child is NULL, don't stuff it back in.
      if (ps != NULL) {
        manta_interface->setPixelSampler(ps);
      } else {
        cerr << "TimeViewSampler has no child, so it can't be turned off\n";
      }
    }
  } else {
    // The pixel sampler in the engine isn't a TimeViewSampler, so
    // let's make it a child of one.
    timeView->setPixelSampler(ps);
    manta_interface->setPixelSampler(timeView);
    timeView->state = 0;
  }
  return timeView;
}

