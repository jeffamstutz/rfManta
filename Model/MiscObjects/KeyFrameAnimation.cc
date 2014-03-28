#include <Model/MiscObjects/KeyFrameAnimation.h>
#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Core/Thread/Time.h>
#include <Core/Util/UpdateGraph.h>

#include <assert.h>
#include <cmath>
#include <stdio.h>

using namespace Manta;
using namespace std;

KeyFrameAnimation::KeyFrameAnimation(InterpolationMode interpolation) :
  as(NULL), interpolation(interpolation), spareGroup(NULL),
  currGroup(NULL), currTime(-1), updateToCurrTime(false),
  paused(false), barrier("keyframe animation barrier"), duration(1),
  lockedFrames(false), startFrame(0), endFrame(0), numFrames(0),
  framesClipped(false),
  loop(true), repeatLastFrame(0.0), repeating(false), forceUpdate(false)
{
}

KeyFrameAnimation::~KeyFrameAnimation()
{
  delete spareGroup;
}

void KeyFrameAnimation::push_back(Group *objects)
{
  if (spareGroup == NULL && interpolation != truncate) {
    spareGroup = objects->clone(shallow);
    currGroup = spareGroup;
    if (as)
      as->setGroup(currGroup);
  }
  frames.push_back(objects);
  if (!framesClipped)
  {
    endFrame = frames.size() - 1;
    numFrames = frames.size();
  }
}

void KeyFrameAnimation::setInterpolation(InterpolationMode mode)
{
  if (mode != truncate) {
    if (spareGroup == NULL && !frames.empty())
      spareGroup = frames[0]->clone(shallow);
    currGroup = spareGroup;
    if (as)
      as->setGroup(currGroup);
  }
  interpolation = mode;
}

void KeyFrameAnimation::startAnimation()
{
  startTime = Time::currentSeconds();
  paused = false;

  if (spareGroup == NULL && interpolation != truncate && !frames.empty()) {
    spareGroup = frames[startFrame]->clone(shallow);
    currGroup = spareGroup;
    if (as)
      as->setGroup(currGroup);
  }

  setTime(0);
}

void KeyFrameAnimation::pauseAnimation()
{
  if (!paused)
    pauseTime = Time::currentSeconds();
  paused = true;
}

void KeyFrameAnimation::resumeAnimation()
{
  if (paused)
    startTime += Time::currentSeconds() - pauseTime;
  paused = false;
  //assume that if resume is called when the animation
  // isn't looping that the user want's to start from
  // the beginning
  if (!loop && (currTime - startTime) >= duration) {
    startTime = Time::currentSeconds();
    updateToCurrTime = true;
  }
}

void KeyFrameAnimation::update(Temp_Callback context)
{
  if (frames.empty())
        return;
  if (context.proc == 0) {
      if (forceUpdate)
      {
        forceUpdate = false;
        currGroup = frames[startFrame];
      }
    }

  //only one thread can do serial code
  if (context.proc == 0) {
    if (repeating) {
      if ((context.time - startTime) >= (duration+repeatLastFrame)) {
        repeating = false;
        updateToCurrTime = true;
      }
    }
    else {
      if (updateToCurrTime) {
        updateToCurrTime = false;
        differentFrame = isDifferentFrame(newTime);
        startTime = context.time - newTime;
        currTime = newTime;
        if (paused) {
          pauseTime = startTime;
        }
      }
      else if (paused) {
        differentFrame = false;
      }
      else {
        if (interpolation == fixed) {
          float newTime = currTime + 1.0;
          if (!loop && newTime >= duration)
            pauseAnimation();
          else
          {
            newTime = wrapTime(newTime);
            differentFrame = isDifferentFrame(newTime);
            currTime = newTime;
            //cerr << "Current frame: " << currTime << "/" << duration << endl;
          }
        }
        else if (lockedFrames) {
          if (interpolation == linear) {
            const int fps = 30;
            float oldTime = currTime;
            float frameIncr = 1.0/fps;
            currTime = wrapTime(currTime + frameIncr);
            differentFrame = isDifferentFrame(oldTime);
          }
          else if (interpolation == truncate) {
            //it's possible that direclty adding in the extra time
            //will result in a time that is just epsilon shy of the
            //next keyframe. To avoid that, we have to figure out at
            //what time the next frame would have occured and then
            //work backwards.
            float nextFrame = 0;
            if (duration > 0.0)
              nextFrame = (currTime/duration)*numFrames+1;
            if (numFrames > 0)
              currTime = (nextFrame/numFrames)*duration;
            else
              currTime = 0;
            differentFrame = numFrames > 1;
            currTime = wrapTime(currTime);
          }
        }
        else {
          float newTime = wrapTime(context.time - startTime);
          differentFrame = isDifferentFrame(newTime);
          currTime = newTime;
        }
      }
      if (!loop && (context.time - startTime) >=duration) {
        pauseAnimation();
      }
      if (repeatLastFrame > 0.0f && (context.time - startTime) >= duration) {
        repeating = true;
      }
    }
  }
  //need to make sure proc 0 has updated the currTime.
  //a barrier is a little heavy for this, but it's cleaner code (and I'm lazy).
  barrier.wait(context.numProcs);
  if (!loop && (context.time - startTime) >=duration) {
    return;
  }

  if (differentFrame) {
    float frame = currTime/duration * numFrames;
    int lastFrame = static_cast<int>(frame);

    if (interpolation == linear || interpolation == fixed) {
      //do interpolation in parallel
      int start = lastFrame;
      int end   = loop ? (start+1) % (numFrames) : min(start+1, numFrames-1);
      float t = frame - start;


      assert(frames[start+startFrame]->size() == frames[end+startFrame]->size());
      std::vector<keyframe_t> keyframes(2);
      keyframes[0].keyframe = frames[start+startFrame];
      keyframes[0].t = 1-t;
      keyframes[1].keyframe = frames[end+startFrame];
      keyframes[1].t = t;

      // NOTE(boulos): This doesn't actually change the group pointer,
      // so we don't need to inform the AccelStruct to do a setGroup
      InterpErr errCode = currGroup->parallelInterpolate(keyframes,
                                                         context.proc, context.numProcs);
      if (errCode != success)
        printf("KeyFrameAnimation had trouble interpolating!\n");
    }
    else if (interpolation == truncate) {
      // NOTE(boulos): Only proc 0 should change the currGroup pointer
      if (context.proc == 0) {
        currGroup = frames[(int) frame+startFrame];
        // TODO(boulos): Who should check for identical pointers?
        if (as)
          as->setGroup(currGroup);
      }
      //make sure the as->setGroup has occured before all the threads
      //go do an update.
      barrier.wait(context.numProcs);

    }

    if (as)
      as->rebuild(context.proc, context.numProcs);
  }
}

//! make sure every frame is shown at least once
void KeyFrameAnimation::lockFrames(bool st)
{
    lockedFrames = st;
}

void KeyFrameAnimation::clipFrames(int start, int end)
{
    if (start < 0 || end >= int(frames.size()) || end < start)
    {
        cerr << "clipFrames out of bounds\n";
        return;
    }
    startFrame = start;
    endFrame = end;
    numFrames = end - start + 1;
    framesClipped = true;
    forceUpdate = true;
}

void KeyFrameAnimation::repeatLastFrameForSeconds(float time)
{
    assert(time >= 0.0f);
    repeatLastFrame = time;
}

void KeyFrameAnimation::loopAnimation(bool st)
{
    loop = st;
}

float KeyFrameAnimation::wrapTime(float time) const
{
  //TODO: add other wrapping modes.

  if (duration == 0)
    return 0;

  //let's assume the animation wraps at the end
  //first we need to handle negative times
  time += static_cast<int>(fabs(time)/duration+1) * duration;
  //now we can safely do the wrapping
  time = fmodf(time, duration);

  return time;
}

bool KeyFrameAnimation::setTime(float time)
{
  updateToCurrTime = true;

  if (frames.empty())
    return true;

  differentFrame = isDifferentFrame(time);

  newTime = wrapTime(time);

  if (!differentFrame)
    return false;
  else
    return true;
}

bool KeyFrameAnimation::isDifferentFrame(float time) const
{
  if (currGroup == NULL) return true;

  time = wrapTime(time);

  if (interpolation == linear || interpolation == fixed)
    return time != currTime;

  if (interpolation == truncate) {
    int i = static_cast<int>(time/duration * numFrames);
    int j = static_cast<int>(currTime/duration * numFrames);
    return i != j;
  }

  else //should never get here
    return true;
}

void KeyFrameAnimation::temporaryUpdate(int proc, int numProcs, bool &changed)
{
  Temp_Callback context;
  context.proc = proc;
  context.numProcs = numProcs;
  context.time = Time::currentSeconds();
  update(context);
  changed = differentFrame;
}

void KeyFrameAnimation::intersect(const RenderContext& context, RayPacket& rays) const
{
  if (as)
    as->intersect(context, rays);
  else if (currGroup)
    currGroup->intersect(context, rays);
}

void KeyFrameAnimation::preprocess(const PreprocessContext& context)
{
  for (unsigned int i=0; i < frames.size(); ++i) {
    frames[i]->preprocess(context);
  }
  if (spareGroup)
    spareGroup->preprocess(context);

  if (as)
    as->preprocess(context);

  if (context.isInitialized() && context.proc == 0) {
    context.manta_interface->registerParallelAnimationCallback
      (Callback::create(this, &KeyFrameAnimation::temporaryUpdate));
  }

  context.done();
}

void KeyFrameAnimation::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  if (currGroup)
    currGroup->computeBounds(context, bbox);
  else if (!frames.empty())
    frames[0]->computeBounds(context, bbox);
}

void KeyFrameAnimation::useAccelerationStructure(AccelerationStructure* _as)
{
  as = _as;
  as->setGroup(currGroup);
  //XXX need to do some more stuff to make sure the switch happens cleanly...
}

void KeyFrameAnimation::addToUpdateGraph(ObjectUpdateGraph* graph,
                                         ObjectUpdateGraphNode* parent) {
  // Insert myself underneath my parent
  ObjectUpdateGraphNode* node = graph->insert(this, parent);
  // Ask the underlying objects to insert themselves underneath me if they want
  for (size_t i = 0; i < frames.size(); i++) {
    frames[i]->addToUpdateGraph(graph, node);
  }
}
