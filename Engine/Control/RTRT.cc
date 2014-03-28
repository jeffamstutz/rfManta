
/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2006
  Scientific Computing and Imaging Institute, University of Utah

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include <Engine/Control/RTRT.h>
#include <Interface/AmbientLight.h>
#include <Interface/Background.h>
#include <Core/Util/Callback.h>
#include <Core/Util/CallbackHandle.h>
#include <Core/Util/CallbackHelpers.h>
#include <Interface/Context.h>
#include <Interface/IdleMode.h>
#include <Interface/Image.h>
#include <Interface/ImageDisplay.h>
#include <Interface/ImageTraverser.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/LoadBalancer.h>
#include <Interface/Object.h>
#include <Interface/PixelSampler.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
#include <Interface/Scene.h>
#include <Interface/SetupCallback.h>
#include <Interface/Task.h>
#include <Interface/TaskQueue.h>
#include <Interface/UserInterface.h>
#include <Interface/ShadowAlgorithm.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Exceptions/InvalidState.h>


#include <Core/Thread/Thread.h>
#include <Core/Thread/Time.h>
#include <Core/Util/Assert.h>
#include <Core/Util/NotFinished.h>
#include <Core/Util/Preprocessor.h>
#include <Core/Util/UpdateGraph.h>
#include <MantaSSE.h>

#include <sstream>
#include <iostream>
#include <algorithm>

#include <Interface/Material.h>

#include <Core/Math/CheapRNG.h>
#include <Core/Math/MT_RNG.h>

#include <Model/Groups/Group.h>
// NOTE(boulos): This is temporary to get the code to compile and
// allow us to discuss the interface (so that I don't have to do all
// the Factory work just yet)
#include <Engine/SampleGenerators/ConstantSampleGenerator.h>
#include <Engine/SampleGenerators/Stratified2D.h>
#include <Engine/SampleGenerators/UniformRandomGenerator.h>

#include <queue>

using namespace Manta;
using namespace std;

#define RENDER_THREAD_STACKSIZE 8*1024*1024
#define USE_UPDATE_GRAPH 0


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Thread Worker Class
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// RTRT Worker class.
RTRT::Worker::Worker(RTRT* rtrt, int workerIndex, bool lateComerFlag)
  : rtrt(rtrt),
    workerIndex(workerIndex),
    lateComerFlag(lateComerFlag)
{
}

RTRT::Worker::~Worker()
{
}

void RTRT::Worker::run()
{
  rtrt->internalRenderLoop(workerIndex, lateComerFlag);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Default Constructor.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

RTRT::RTRT()
  : currentImageTraverser(NULL),
    currentLoadBalancer(NULL),
    currentPixelSampler(NULL),
    currentRenderer(NULL),
    currentSampleGenerator(NULL),
    currentShadowAlgorithm(NULL),
    create_image(NULL),
    create_rng(NULL),
    runningLock("RTRT running mutex"),
    callbackLock("RTRT callback r/w lock"),
    barrier1("RTRT frame barrier #1"),
    barrier2("RTRT frame barrier #2"),
    barrier3("RTRT frame barrier #3"),
    workers_changed_barrier("RTRT workers changed barrier"),
    preprocess_barrier("Barrier for preprocessing"),
    transaction_lock("RTRT transaction lock"),
    thread_storage( 0 ),
    channel_create_lock("RTRT channel creation lock"),
    update_graph(new ObjectUpdateGraph()),
    update_work_queue(new TaskQueue())
{
  workersWanted=0;
  workersRendering=0;
  workersAnimAndImage=0;
  workersChanged = false;
  lastChanged = false;
  running=false;
  animFrameState.frameSerialNumber = 0;
  animFrameState.animationFrameNumber = 0;
  animFrameState.frameTime = 0;
  animFrameState.shutter_open = 0;
  animFrameState.shutter_close = 0;
  timeMode = MantaInterface::RealTime;
  timeScale = 1;
  timeOffset = 0;
  time_is_stopped = false;
  frameRate = 15;
  pipelineNeedsSetup = true;
  scene = 0;
  verbose_transactions = false;
  displayBeforeRender = true;

#if 1
  create_rng = Callback::create(&CheapRNG::create);
#else
  create_rng = Callback::create(&MT_RNG::create);
#endif

  //currentSampleGenerator = new ConstantSampleGenerator(0.f);
  currentSampleGenerator = new UniformRandomGenerator();
  //currentSampleGenerator = new Stratified2D(81);
}

RTRT::~RTRT()
{
  for(ChannelListType::iterator iter = channels.begin();
      iter != channels.end(); iter++){
    Channel* channel = *iter;
    delete channel->display;
    for(vector<Image*>::iterator iter = channel->images.begin();
        iter != channel->images.end(); iter++)
      delete *iter;
  }
  delete currentImageTraverser;
  delete currentLoadBalancer;
  delete currentPixelSampler;
  delete currentRenderer;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Control Methods.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RTRT::changeNumWorkers(int newNumWorkers)
{
  if(newNumWorkers < 0)
    throw IllegalValue<int>("RTRT::changeNumWorkers, number of workers should be > 0", newNumWorkers);

  workersWanted=newNumWorkers;
}

TValue<int>& RTRT::numWorkers()
{
  return workersWanted;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Register/Add Callback Functions.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RTRT::addOneShotCallback(OneShotTime whence, FrameNumber frame,
                              CallbackBase_2Data<int, int>* callback)
{
#if NOTFINISHED
  // What about adding callbacks during anim cycle?
#endif
  callbackLock.writeLock();
  // #if NOTFINISHED

  if(whence == MantaInterface::Relative) {
    frame += animFrameState.frameSerialNumber;
  }
  // #endif

  oneShots.insert(OneShotMapType::value_type(frame, callback));
  callbackLock.writeUnlock();
}

void RTRT::addParallelOneShotCallback(OneShotTime whence, FrameNumber frame,
                                      CallbackBase_2Data<int, int>* callback)
{
  callbackLock.writeLock();

  if(whence == MantaInterface::Relative) {
    frame += animFrameState.frameSerialNumber;
  }

  parallelOneShots.insert(OneShotMapType::value_type(frame, callback));
  callbackLock.writeUnlock();
}

CallbackHandle* RTRT::registerSetupCallback(SetupCallback* callback)
{
  setupCallbacks.push_back(callback);
  return callback;
}

CallbackHandle* RTRT::registerParallelAnimationCallback(CallbackBase_3Data<int, int, bool&>* cb)
{
  callbackLock.writeLock();
  parallelAnimationCallbacks.push_back(cb);
  callbackLock.writeUnlock();
  return cb;
}

CallbackHandle* RTRT::registerSerialAnimationCallback(CallbackBase_3Data<int, int, bool&>* cb)
{
  callbackLock.writeLock();
  serialAnimationCallbacks.push_back(cb);
  callbackLock.writeUnlock();
  return cb;
}

CallbackHandle* RTRT::registerSerialPreRenderCallback(CallbackBase_2Data<int, int>* cb)
{
  callbackLock.writeLock();
  serialPreRenderCallbacks.push_back(cb);
  callbackLock.writeUnlock();
  return cb;
}

CallbackHandle* RTRT::registerParallelPreRenderCallback(CallbackBase_2Data<int, int>* cb)
{
  callbackLock.writeLock();
  parallelPreRenderCallbacks.push_back(cb);
  callbackLock.writeUnlock();
  return cb;
}

CallbackHandle* RTRT::registerTerminationCallback( CallbackBase_1Data< MantaInterface *> *cb ) {
  callbackLock.writeLock();
  terminationCallbacks.push_back(cb);
  callbackLock.writeUnlock();
  return cb;
}

void RTRT::unregisterCallback(CallbackHandle* callback)
{
  callbackLock.writeLock();

  // Search setup callbacks
  {
    SetupCallback* cb=dynamic_cast<SetupCallback*>(callback);
    if (cb) {
      vector<SetupCallback*>::iterator iter;
      iter=find(setupCallbacks.begin(), setupCallbacks.end(), cb);
      if (iter != setupCallbacks.end()) {
        setupCallbacks.erase(iter);
        callbackLock.writeUnlock();
        return;
      }
    }
  }

  // Search serial animation callbacks
  {
    typedef CallbackBase_3Data<int, int, bool&> callback_t;
    callback_t* cb=dynamic_cast<callback_t*>(callback);
    if (cb) {
      vector<callback_t*>::iterator iter;
      iter=find(serialAnimationCallbacks.begin(), serialAnimationCallbacks.end(), cb);
      if (iter != serialAnimationCallbacks.end()) {
        serialAnimationCallbacks.erase(iter);
        callbackLock.writeUnlock();
        return;
      }
    }
  }

  // Search parallel animation callbacks
  {
    typedef CallbackBase_3Data<int, int, bool&> callback_t;
    callback_t* cb=dynamic_cast<callback_t*>(callback);
    if (cb) {
      vector<callback_t*>::iterator iter;
      iter=find(parallelAnimationCallbacks.begin(), parallelAnimationCallbacks.end(), cb);
      if (iter != parallelAnimationCallbacks.end()) {
        parallelAnimationCallbacks.erase(iter);
        callbackLock.writeUnlock();
        return;
      }
    }
  }

  // Search serial prerender callbacks
  {
    typedef CallbackBase_2Data<int, int> callback_t;
    callback_t* cb=dynamic_cast<callback_t*>(callback);
    if (cb) {
      vector<callback_t*>::iterator iter;
      iter=find(serialPreRenderCallbacks.begin(), serialPreRenderCallbacks.end(), cb);
      if (iter != serialPreRenderCallbacks.end()) {
        serialPreRenderCallbacks.erase(iter);
        callbackLock.writeUnlock();
        return;
      }
    }
  }

  // Search parallel prerender callbacks
  {
    typedef CallbackBase_2Data<int, int> callback_t;
    callback_t* cb=dynamic_cast<callback_t*>(callback);
    if (cb) {
      vector<callback_t*>::iterator iter;
      iter=find(parallelPreRenderCallbacks.begin(), parallelPreRenderCallbacks.end(), cb);
      if (iter != parallelPreRenderCallbacks.end()) {
        parallelPreRenderCallbacks.erase(iter);
        callbackLock.writeUnlock();
        return;
      }
    }
  }

  // Search termination callbacks
  {
    typedef CallbackBase_1Data<MantaInterface*> callback_t;
    callback_t* cb=dynamic_cast<callback_t*>(callback);
    if (cb) {
      vector<callback_t*>::iterator iter;
      iter=find(terminationCallbacks.begin(), terminationCallbacks.end(), cb);
      if (iter != terminationCallbacks.end()) {
        terminationCallbacks.erase(iter);
        callbackLock.writeUnlock();
        return;
      }
    }
  }

  // Callback not found (!)

  callbackLock.writeUnlock();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Time Methods
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RTRT::setTime( double time_ )
{
  bool was_stopped = time_is_stopped;

  // Reset the stopped time.
  stopTime();
  stoptime = time_;
  startTime();

  if (was_stopped) {
    stopTime();
  }
}

void RTRT::setTimeMode(TimeMode tm, double ts)
{
  // If this gets used often, we may need to add a lock to avoid shear if part
  // of the data gets picked up at just the wrong moment
  if(timeMode == tm){
    // Since the time modes are the same, assume that the rate is changing and
    // solve for a new offset that will keep the current time the same.
    double r1, r2;
    switch(timeMode){
    case RealTime: {
      r1 = timeScale;
      r2 = ts;
    } break;
    case FixedRate: {
      r1 = 1./frameRate;
      r2 = 1./ts;
    } break;
    case Static:
    default: {
      r1 = r2 = 1;
    } break;
    }
    double time = Time::currentSeconds();
    timeOffset += (r2-r1)*time;
    timeScale = ts;
    timeMode = tm;
  } else {
    timeMode = tm;
    switch(timeMode){
    case RealTime: {
      timeScale = ts;
    } break;
    case FixedRate: {
      frameRate = ts;
    } break;
    case Static: {
    } break;
    }
  }
}

void RTRT::startTime()
{
  if(time_is_stopped){
    double time = Time::currentSeconds();
    timeOffset += time - stoptime * timeScale;
    time_is_stopped = false;
  }
}

void RTRT::stopTime()
{
  if(!time_is_stopped){
    stoptime = Time::currentSeconds();
    time_is_stopped = true;
  }
}

bool RTRT::timeIsStopped()
{
  return time_is_stopped;
}



#define MANTA_CHECK_POINTER(ptr) if (ptr == 0) throw InvalidState( #ptr );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// BEGIN RENDERING
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RTRT::beginRendering(bool blockUntilFinished) throw (Exception &)
{
  runningLock.lock();
  if(running) {
    runningLock.unlock();
    throw InvalidState("renderLoop started while it is already running");
  }
  running=true;
  runningLock.unlock();

  if(workersWanted <= 0){
    throw IllegalValue<int>("workersWanted should be positive", workersWanted);
  }

  if(workersRendering != 0){
    throw IllegalValue<int>("workersRendering should be zero", workersRendering);
  }

  workersRendering=workersWanted;

  // Start up all of the Worker threads.
  workers.resize(workersWanted);
  rngs.resize(workersWanted);
  changedFlags.resize(workersWanted);

  // Initialize random number generators.
  if (create_rng) {
    for (int i=0;i<workersWanted;++i) {
      create_rng->call(rngs[i]);
    }
  }

  // Startup rendering threads.
  for(int i=0;i<workersWanted;i++){
    if(i>0 || !blockUntilFinished){
      ostringstream name;
      name << "RTRT Worker " << i;
      Thread* t = workers[i] =
        new Thread(new Worker(this, i, false), name.str().c_str(),
                   0, Thread::NotActivated);
      t->migrate(i);
      t->setStackSize(RENDER_THREAD_STACKSIZE);
      t->activate(false);
    }
  }

  // All rendering threads active.

  // Block until finished is set while running in bin/manta
  if(blockUntilFinished)
    internalRenderLoop(0, false);

}

// NOTE(boulos): This function can be called multiple times
void RTRT::blockUntilFinished()
{
  while (1) {
    runningLock.lock();
    if (running == false) {
      // Now that nobody is running, workers.size() should be exactly 1
      // (since we've already brought down all other threads) or should be
      // 0 upon future calls into this function.
      ASSERT(workers.size() == 0 ||
             workers.size() == 1);
      for (size_t i = 0; i < workers.size(); i++) {
        workers[i]->join();
      }
      workers.resize(0);
      runningLock.unlock();
      break;
    }
    runningLock.unlock();
  }
}

void RTRT::doPreprocess(int proc, int numProcs) {
  // Preprocess
  MANTA_CHECK_POINTER(scene)

  LightSet* lights = scene->getLights();
  MANTA_CHECK_POINTER(lights)

  PreprocessContext context(this, proc, numProcs, lights);
  lights->preprocess(context);

  MANTA_CHECK_POINTER(scene->getBackground())
  scene->getBackground()->preprocess(context);

  //cerr << "About to preprocess the scene (proc " << proc << ")...\n";
  MANTA_CHECK_POINTER(scene->getObject())
  scene->getObject()->preprocess(context);

  for(int index = 0;index < static_cast<int>(channels.size());index++){
    Channel* channel = channels[index];
    MANTA_CHECK_POINTER(channel)

    channel->camera->preprocess(context);
    MANTA_CHECK_POINTER(channel->camera)
  }
}

void RTRT::finishUpdates(Object* which_object,
                         const UpdateContext& context) {
  // Pass this along to the update graph, so that the node is marked
  // as done. If the parent of the node is now ready to be processed,
  // next_node will be non-null.
  //cerr << MANTA_FUNC << " object = " << which_object << endl;
  ObjectUpdateGraphNode* next_node = update_graph->finishUpdate(which_object);
  if (next_node) {
    // Follow this guy immediately
    Object* next_object = next_node->key;
    next_object->performUpdate(context);
  }
}

void RTRT::insertWork(TaskList* new_work) {
  update_work_queue->insert(new_work);
}

void RTRT::doUpdates(bool& changed, int proc, int numProcs) {
  // While the update_graph isn't done, we should keep checking for
  // work
  double start_time = Time::currentSeconds();
  bool had_update = false;
  UpdateContext context(this, proc, numProcs);
  while (!update_graph->finished()) {
    // Some update actually happened, let the pipeline know
    had_update = true;

    // Ask the work queue for a task to run
    Task* work = update_work_queue->grabWork();

    if (work) {
      work->run();
    } else {
      // Lock the update graph and see if a leaf node can be expanded
      // into more work
      Object* object = NULL;
      update_graph->lock();
      ObjectUpdateGraph::leaf_iterator leaf_it = update_graph->leaf_nodes.begin();
      if (leaf_it != update_graph->leaf_nodes.end()) {

        ObjectUpdateGraphNode* node = *leaf_it;
        object = node->key;
        // Remove the leaf from the set of active leaf_nodes
        update_graph->leaf_nodes.erase(leaf_it);
      }
      update_graph->unlock();

      if (object) {
        // We found a leaf object, start running the update
        object->performUpdate(context);
      }
    }
    // We now loop back to find more work.
  }
  if (had_update) {
    changed = true;
    if (proc == 0) {
      double end_time = Time::currentSeconds();
      cerr << "Finished some updates (in " << end_time - start_time << " seconds)\n";
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MAIN RENDERING LOOP
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if USE_UPDATE_GRAPH
void PrintHello() {
}
#endif

void RTRT::internalRenderLoop(int proc, bool lateComerFlag)
{
#ifdef MANTA_SSE

#ifndef _MM_DENORM_ZERO_ON
#define _MM_DENORM_ZERO_ON 0x0040
#endif

  // Disables denormal handling and set flush to zero
  int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
  int newMXCSR = oldMXCSR | _MM_FLUSH_ZERO_ON | _MM_DENORM_ZERO_ON; // set DAZ and FZ bits
  _mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
#endif

  bool changed = true;
  bool firstFrame = true;
  if(lateComerFlag){
    firstFrame = false;
    goto skipToRendering;
  }

  // Check to see if an image creator has been specified.
  if(create_image==0)
    throw InvalidState("Image type was not selected" );

#if NOTFINISHED
  check for existence of other components;
#endif

#if 1
  doPreprocess(proc, workersRendering); {
#else
  // TODO(boulos): Go through and ask each thread to preprocess the
  // scene.  For now, only thread 0 will do this and pretend there is
  // only a single thread to emulate the old behavior until we can
  // ensure everything is thread-safe throughout the tree.
  if (proc == 0) {
    doPreprocess(proc, 1);
#endif

#if USE_UPDATE_GRAPH
    #warning "Adding objects to the update graph"
    scene->getObject()->addToUpdateGraph(update_graph, NULL);
#if 1
#warning "Sending update transaction for first Object in group for frame 0"
      // Temporary
      Group* top_level_group = dynamic_cast<Group*>(getScene()->getObject());
      addUpdateTransaction("Test Update Graph",
                           Callback::create(PrintHello),
                           top_level_group->get(0));
#endif
#endif
    //update_graph->print(scene->getObject());
  }

  for(;;){
    // P0 update frame number, time, etc.
    if(proc == 0){
      animFrameState.frameSerialNumber++;
      if(time_is_stopped){
        // Time is stopped - leave the frame state where it is
      } else {
        animFrameState.animationFrameNumber++;

        switch(timeMode){
        case RealTime:
          animFrameState.shutter_open = animFrameState.shutter_close;
          animFrameState.frameTime = (Time::currentSeconds() - timeOffset) * timeScale;
          animFrameState.shutter_close = animFrameState.frameTime;
          break;
        case FixedRate:
          animFrameState.shutter_open = animFrameState.shutter_close;
          animFrameState.frameTime = ((static_cast<double>(animFrameState.animationFrameNumber) / frameRate) - timeOffset) * timeScale;
          animFrameState.shutter_close = animFrameState.frameTime;
          break;
        case Static:
          break;
        }
      }

      // Update the number of workers to be used for the animation and
      // image display portion
      workersAnimAndImage = workersRendering;
    }

    // Start of non-rendering portion of the loop. Make callbacks
    // that could possibly change state and get everything set up to
    // render the next frame
    barrier1.wait(workersRendering);

    // Copy over the frame state
    if(proc == 0)
      renderFrameState = animFrameState;

    // Do callbacks
    changed=false;
    //callbackLock.readLock();

    if(proc == 0)
      postTransactions(changed);

    barrier1.wait(workersRendering);

#if !(USE_UPDATE_GRAPH)
    doSerialAnimationCallbacks(changed, proc, workersAnimAndImage);
    doParallelAnimationCallbacks(changed, proc, workersAnimAndImage);
#else
#warning "Using UpdateGraph instead of animation callbacks"
    doSerialAnimationCallbacks(changed, proc, workersAnimAndImage);
    doParallelAnimationCallbacks(changed, proc, workersAnimAndImage);
    doUpdates(changed, proc, workersRendering);
    barrier1.wait(workersRendering);
    if (proc == 0) {
      // NOTE(boulos): Use this code to rebuild the first Object in the group every frame
#warning "Sending update transaction for first Object in group"
      // Temporary
      Group* top_level_group = dynamic_cast<Group*>(getScene()->getObject());
      addUpdateTransaction("Test Update Graph",
                           Callback::create(PrintHello),
                           top_level_group->get(0));
    }
#endif

    if(!firstFrame){
      for(size_t index = 0;index < channels.size();index++){
        Channel* channel = channels[index];
        RenderContext myContext(this, index, proc, workersAnimAndImage,
                                &animFrameState,
                                currentLoadBalancer, currentPixelSampler,
                                currentRenderer, currentShadowAlgorithm,
                                channel->camera, scene, thread_storage,
                                rngs[proc],
                                currentSampleGenerator);
        currentImageTraverser->setupFrame(myContext);
      }
    }
    //callbackLock.readUnlock();

    // P0 resize images for next frame
    if(proc == 0)
      resizeImages(animFrameState.frameSerialNumber);

    // Update the number of workers to be used for the rendering of this frame
    if(proc == 0){
      // Copy the number out of memory - we want to read it only once
      // in case it changes
      int newWorkers = workersWanted;
      if(newWorkers > workersRendering){
        // We must start new threads, if the number increased.  Mark
        // these threads as "latecomers", which will skip to the rendering
        // section of this loop for the first frame
        workersChanged = true;
        workers.resize(newWorkers);
        rngs.resize(newWorkers);
        int oldworkers = workersRendering;
        workersRendering = workersWanted;
        for(int i=oldworkers;i<newWorkers;i++){
          ostringstream name;
          name << "RTRT Worker " << i;
          workers[i] = new Thread(new Worker(this, i, true), name.str().c_str(),
                                  0, Thread::NotActivated);

          workers[i]->migrate(i);
          // Set the stack size.
          workers[i]->setStackSize(RENDER_THREAD_STACKSIZE);

          // Create a random number generator.
          if (create_rng) {
            create_rng->call(rngs[i]);
          }

          // Active the thread.
          workers[i]->activate(false);
        }
      } else if(newWorkers < workersRendering) {
        workersChanged = true;
        workersRendering = workersWanted;
        // TODO(bigler,boulos): should we get rid of our random number generators.
      } else {
        // Don't set it to false if it is already false - avoids
        // flushing the other caches
        if(workersChanged)
          workersChanged = false;
      }
      if (workersChanged) pipelineNeedsSetup = true;
    }

    // New scope to control variable lifetimes
    {
      // Reduce on changed flag
      // Save the number of animator processors because it may not be valid
      // after the barrier, and we need to reduce on that number
      int numProcs = workersAnimAndImage;
      changedFlags[proc].changed = changed;
      barrier2.wait(numProcs);
      changed=false;
      for(int i=0;i<numProcs;i++){
        if(changedFlags[i].changed){
          changed=true;
          break;
        }
      }

      if(changed != lastChanged || firstFrame){
        if(proc == 0)
          doIdleModeCallbacks(changed, firstFrame, pipelineNeedsSetup,
                              proc, numProcs);
        barrier2.wait(numProcs);
        lastChanged = changed;
      }
      if(firstFrame)
        firstFrame=false;

      if(pipelineNeedsSetup){

        // Negotiate the image pipeline for each channel
        if(proc == 0){
          // Send the number of workers wanted to setupPipelines, so
          // that we can allocate memory for the new set of render
          // threads.  You can't send numProcs, because new threads
          // need to be accounted for.  Make sure we have at least one
          // thread, so allocators inside setupPipelines don't blow
          // up.
          setupPipelines(workersRendering == 0?1:workersRendering);
          resizeImages(renderFrameState.frameSerialNumber);
        }

        // Wait for processor zero to setup the image pipeline
        barrier3.wait(numProcs);

        // Allocate thread local storage for each thread.
        if(proc < workersRendering) {
          // Crappy hack just to keep it from dying.
          thread_storage->allocateStorage( proc );
        }

        for(size_t index = 0;index < channels.size();index++){
          Channel* channel = channels[index];
          RenderContext myContext(this, index, proc, (workersRendering == 0?1:workersRendering),
                                  &animFrameState,
                                  currentLoadBalancer,
                                  currentPixelSampler,
                                  currentRenderer,
                                  currentShadowAlgorithm,
                                  channel->camera,
                                  scene,
                                  thread_storage,
                                  rngs[proc],
                                  currentSampleGenerator);

          currentImageTraverser->setupFrame(myContext);
        }
        barrier3.wait(numProcs);
        pipelineNeedsSetup = false;
      }

      if (displayBeforeRender) {
        // Image display, if image is valid
        for(ChannelListType::iterator iter = channels.begin();
            iter != channels.end(); iter++) {
          Channel* channel = *iter;
          long displayFrame = (renderFrameState.frameSerialNumber-1)%channel->pipelineDepth;
          Image* image = channel->images[displayFrame];
          if(image && image->isValid()){
            DisplayContext myContext(proc, workersAnimAndImage, displayFrame, channel->pipelineDepth);
            channel->display->displayImage(myContext, image);
          }
        }
      }

      // Possibly change # of workers
      if(proc == 0 && workersRendering < numProcs){
        for(int i = workersRendering; i<numProcs; i++){
          // Clean up after worker.  Don't attempt to join with self
          if(i != 0)
            workers[i]->join();
        }
        workers.resize(workersRendering == 0?1:workersRendering);
      }

      if(proc >= workersRendering) {

        // If proc 0 is exiting invoke the termination callbacks.
        if (proc == 0) {
          doTerminationCallbacks();

          // Set the running flag to false.
          runningLock.lock();
          running = false;
          runningLock.unlock();
        }

        break;
      }
    }
  skipToRendering:
    // Pre-render callbacks
    //callbackLock.readLock();
    doSerialPreRenderCallbacks(proc, workersRendering);
    doParallelPreRenderCallbacks(proc, workersRendering);
    //callbackLock.readUnlock();

    if(workersChanged){
      workers_changed_barrier.wait(workersRendering);
      if(proc == 0)
        changedFlags.resize(workersRendering);
    }

    // #if NOTFINISHED
    //if(!idle){
    // #endif
    {
      for(size_t index = 0;index < channels.size();index++){
        Channel* channel = channels[index];
        long renderFrame = renderFrameState.frameSerialNumber%channel->pipelineDepth;
        Image* image = channel->images[renderFrame];
        RenderContext myContext(this, index, proc, workersRendering, &renderFrameState,
                                currentLoadBalancer, currentPixelSampler,
                                currentRenderer, currentShadowAlgorithm,
                                channel->camera, scene, thread_storage,
                                rngs[proc],
                                currentSampleGenerator);
        currentImageTraverser->renderImage(myContext, image);
      }
    }
    // #if NOTFINISHED
    //    how to set rendering complete flag?;
    //  } else {
    //callbackLock.readLock();
    //    idle callbacks;
    //callbackLock.readUnlock();
    //  }
    //#endif

    if (!displayBeforeRender) {
      barrier1.wait(workersRendering);

      // Image display, if image is valid
      for(ChannelListType::iterator iter = channels.begin();
          iter != channels.end(); iter++){
        Channel* channel = *iter;
        long displayFrame = (renderFrameState.frameSerialNumber)%channel->pipelineDepth;
        Image* image = channel->images[displayFrame];
        if(image && image->isValid()){
          DisplayContext myContext(proc, workersAnimAndImage, displayFrame, channel->pipelineDepth);
          channel->display->displayImage(myContext, image);
        }
      }
    }
  }

#ifdef MANTA_SSE
  // Restore floating point register flags
  _mm_setcsr( oldMXCSR ); //write the new MXCSR setting to the MXCSR
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// PARALLEL ANIMATION CALLBACKS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RTRT::doParallelAnimationCallbacks(bool& changed, int proc, int numProcs) {

  // Parallel one shot callbacks.
  ParallelOneShotMapType::iterator iter = parallelOneShots.begin();
  while(iter != parallelOneShots.end() && iter->first < animFrameState.frameSerialNumber){
    iter->second->call(proc, numProcs);

    // Add a deletion transaction.
    if (proc == 0) {
      static_cast<MantaInterface *>
        (this)->addTransaction( "delete oneshot",
                                Callback::create( this,
                                                  &RTRT::deleteParallelOneShot,
                                                  iter ) );
    }

    ++iter;
  }

  // All threads do the parallel animation callbacks
  for(ACallbackMapType::iterator iter = parallelAnimationCallbacks.begin();
      iter != parallelAnimationCallbacks.end(); iter++){
    (*iter)->call(proc, numProcs, changed);
  }
}

void RTRT::deleteParallelOneShot( RTRT::ParallelOneShotMapType::iterator iter )
{
  cerr<<"deleting parallelOneShot\n";

  // Delete the callback
  delete iter->second;

  // Delete the iterator.
  parallelOneShots.erase(iter);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SERIAL ANIMATION CALLBACKS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RTRT::doSerialAnimationCallbacks(bool& changed, int proc, int numProcs)
{
  if(proc == 0){
    // One-shots are always done by p0, since we do not always know
    // how many there will be, and we need to potentially update the map
    OneShotMapType::iterator iter = oneShots.begin();
    while(iter != oneShots.end() && iter->first < animFrameState.frameSerialNumber){
      iter->second->call(proc, numProcs);
      delete iter->second;
      oneShots.erase(iter);
      iter = oneShots.begin();
    }
  }
  unsigned long totalCallbacks = serialAnimationCallbacks.size();
  unsigned long start = proc*totalCallbacks/numProcs;
  unsigned long end = (proc+1)*totalCallbacks/numProcs;
  ACallbackMapType::iterator iter = start+serialAnimationCallbacks.begin();
  ACallbackMapType::iterator stop = end+serialAnimationCallbacks.begin();
  for(; iter != stop; iter++) {
    (*iter)->call(proc, numProcs, changed);
  }
}

void RTRT::doTerminationCallbacks() {

  // All threads have terminated.
  TCallbackMapType::iterator iter = terminationCallbacks.begin();
  while (iter != terminationCallbacks.end()) {

    // Invoke the callback and pass in the MantaInterface.
    (*iter)->call( this );
    ++iter;
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// POST TRANSACTIONS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RTRT::postTransactions(bool& changed)
{
  bool updated_graph = false;
  if(transactions.size() > 0){
    // Lock the queue.
    transaction_lock.lock();

    while (transactions.size()) {
      // Pop the first transaction from the queue.
      TransactionBase *transaction = *(transactions.begin());
      transactions.pop_front();

      // Apply the transaction.
      transaction->apply();
      int flag = transaction->getFlag();

      UpdateTransaction* update = dynamic_cast<UpdateTransaction*>(transaction);
      if (update) {
        update_graph->markObject(update->getObject());
        updated_graph = true;
      }

      // Delete the transaction.
      delete transaction;

      // Determine if the transaction contains any queue operations.
      if (flag == TransactionBase::CONTINUE) {
        // Stop processing transactions and continue the pipeline.
        changed = true;
        break;
      }
      else if (flag == TransactionBase::PURGE) {
        // Remove all remaining transactions from the queue.
        changed = true;
        while (transactions.size()) {
          transaction = *(transactions.begin());
          transactions.pop_front();
          delete transaction;
        }
      }
      else if (flag == TransactionBase::NO_UPDATE) {
        // Do nothing and don't set changed
      } else if (flag == TransactionBase::DEFAULT) {
        changed = true;
      }
    }

    if (updated_graph) {
      //update_graph->print(getScene()->getObject());
    }
    // Unlock the queue.
    transaction_lock.unlock();
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Execute Callback Helpers
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RTRT::doParallelPreRenderCallbacks(int proc, int numProcs)
{
  // All threads do the parallel pre-render callbacks
  for(PRCallbackMapType::iterator iter = parallelPreRenderCallbacks.begin();
      iter != parallelPreRenderCallbacks.end(); iter++){
    (*iter)->call(proc, numProcs);
  }
}

void RTRT::doSerialPreRenderCallbacks(int proc, int numProcs)
{
  unsigned long totalCallbacks = serialPreRenderCallbacks.size();
  unsigned long start = proc*totalCallbacks/numProcs;
  unsigned long end = (proc+1)*totalCallbacks/numProcs;
  PRCallbackMapType::iterator iter = start+serialPreRenderCallbacks.begin();
  PRCallbackMapType::iterator stop = end+serialPreRenderCallbacks.begin();
  for(; iter != stop; iter++)
    (*iter)->call(proc, numProcs);
}

void RTRT::doIdleModeCallbacks(bool changed, bool firstFrame,
                               bool& pipelineNeedsSetup,
                               int proc, int numProcs)
{
  int numChannels = static_cast<int>(channels.size());
  SetupContext globalcontext(this, numChannels, proc, numProcs,
                             currentLoadBalancer, currentPixelSampler,
                             currentRenderer, currentSampleGenerator);
  for(vector<IdleMode*>::iterator iter = currentIdleModes.begin();
      iter != currentIdleModes.end(); iter++){
    IdleMode* im = *iter;
    im->changeIdleMode(globalcontext, changed, firstFrame, pipelineNeedsSetup);
  }
}

void RTRT::finish()
{
#if NOTFINISHED
  decide what to do based on whether we are in animation section or not or an external thread...  How to do this efficiently? Thread-local data?;
  how to do this everywhere without massive code duplication?;
#endif
  workersWanted = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Create Channel.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int RTRT::createChannel(ImageDisplay *image_display, Camera* camera, bool stereo, int xres, int yres)
{

  // Create the channel.
  Channel* channel = new Channel;

  // Add the channel to the renderer.
  channel_create_lock.lock();
  channel->id      = static_cast<int>(channels.size());
  channels.push_back(channel);
  channel_create_lock.unlock();

  // Set up the parameters
  channel->display = image_display;
  channel->stereo  = stereo;
  channel->xres    = xres;
  channel->yres    = yres;
  channel->active  = true;
  channel->create_image  = create_image;
  channel->pipelineDepth = 2;
  channel->images.resize(channel->pipelineDepth);
  channel->camera = camera;

  // Set up aspect ratio on camera
  channel->camera->setAspectRatio(static_cast<float>(xres)/yres);

  // Setup images for pipeline.
  for(int i=0;i<channel->pipelineDepth;i++)
    channel->images[i] = 0;

  pipelineNeedsSetup = true;

  return channel->id;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Setup Pipelines by invoking setup callbacks.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RTRT::setupPipelines(int numProcs)
{
  // Total number of channels.
  int numChannels = static_cast<int>(channels.size());

  // Create a setup context.
  SetupContext globalcontext(this, numChannels, 0, numProcs,
                             currentLoadBalancer, currentPixelSampler,
                             currentRenderer, currentSampleGenerator);

  // Setup the image traverser.
  currentImageTraverser->setupBegin(globalcontext, numChannels);

  // Proceess setup callbacks.
  for(vector<SetupCallback*>::iterator iter = setupCallbacks.begin(); iter != setupCallbacks.end(); iter++)
    (*iter)->setupBegin(globalcontext, numChannels);

  // Allocate/resize per thread local storage.
  if (thread_storage != 0)
    delete thread_storage;
  thread_storage = new ThreadStorage( numProcs );

  // Setup each channel.
  for(int index = 0;index < static_cast<int>(channels.size());index++){
    Channel* channel = channels[index];
    SetupContext context(this, index, numChannels, 0, numProcs,
                         channel->stereo, channel->xres, channel->yres,
                         currentLoadBalancer,
                         currentPixelSampler,
                         currentRenderer,
                         currentSampleGenerator,
                         thread_storage );

    // Setup the channel iteratively, until context.isChanged() is false.
    int iteration = 100;

    do {
      context.setChanged(false);
      context.clearMasterWindow();

      // Call setup callbacks.
      for(vector<SetupCallback*>::iterator iter = setupCallbacks.begin(); iter != setupCallbacks.end(); iter++)
        (*iter)->setupDisplayChannel(context);

      // Setup the image display.
      currentImageTraverser->setupDisplayChannel(context);
      channel->display->setupDisplayChannel(context);

    } while(context.isChanged() && --iteration > 0);

    // Check to for errors.
    if(!iteration)
      throw InternalError("Pipeline/resolution negotiation failed");
    context.getResolution(channel->stereo, channel->xres, channel->yres);

    if(channel->xres <= 0)
      throw IllegalValue<int>("Resolution should be positive", channel->xres);
    if(channel->yres <= 0)
      throw IllegalValue<int>("Resolution should be positive", channel->yres);
    if(context.getMinDepth() > context.getMaxDepth())
      throw InternalError("Pipeline depth negotiation failed");

    int depth = context.getMinDepth();
    if(depth == 1 && context.getMaxDepth() > 1)
      depth = 2; // Prefer double-buffering

    //cerr << "RTRT::setupPipelines:: depth = "<< depth << "\n";

    // Set the pipeline depth.
    channel->pipelineDepth = depth;
    unsigned long osize = channel->images.size();
    for(unsigned long i=depth;i<osize;i++)
      delete channel->images[i];

    // Zero pointers to images.
    for(unsigned long i=osize;i<static_cast<unsigned long>(depth);i++)
      channel->images[i]=0;

    // Specify the number of frames needed for the entire pipeline.
    channel->images.resize(depth);
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Resize Images
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RTRT::resizeImages(long frame)
{

  for(ChannelListType::iterator iter = channels.begin();
      iter != channels.end(); iter++){
    Channel* channel = *iter;
    // Potentially resize the current rendering frame
    long which = frame%channel->pipelineDepth;
    bool stereo;
    int xres, yres;

    if(channel->images[which])
      channel->images[which]->getResolution(stereo, xres, yres);
    if(!channel->images[which] || channel->stereo != stereo
       || channel->xres != xres || channel->yres != yres){

      // Delete the image if it already exists.
      if(channel->images[which])
        delete channel->images[which];

      // Invoke the create image callback.
      channel->create_image->call( channel->stereo,
                                   channel->xres,
                                   channel->yres,
                                   channel->images[which] );

    } else {
      if (channel->pipelineDepth > 1)
        channel->images[which]->setValid(false);
    }
  }
}




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Pipeline Get / Set Methods.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Image Display

ImageDisplay *RTRT::getImageDisplay( int channel ) {
  return channels[channel]->display; }

void RTRT::setImageDisplay( int channel, ImageDisplay *display ) {

  channels[channel]->display = display;

  // Setup the image display with the pipeline.
  pipelineNeedsSetup = true;
}

// Camera

void RTRT::setCamera(int channel, Camera *camera ) {
  channels[channel]->camera = camera;
}

Camera* RTRT::getCamera(int channel) const {
  return channels[channel]->camera;
}

// Resolution.

void RTRT::getResolution(int channel, bool& stereo, int& xres, int& yres) {
  stereo = channels[channel]->stereo;
  xres = channels[channel]->xres;
  yres = channels[channel]->yres;
}

void RTRT::changeResolution(int channel, int xres, int yres, bool changePipeline) {
  channels[channel]->xres = xres;
  channels[channel]->yres = yres;
  channels[channel]->camera->setAspectRatio(static_cast<float>(xres)/yres);

  if (changePipeline)
    pipelineNeedsSetup = true;
}

// Image Traverser

void RTRT::setImageTraverser( ImageTraverser* image_traverser_ ) {
  currentImageTraverser = image_traverser_;
}

ImageTraverser* RTRT::getImageTraverser() const {
  return currentImageTraverser;
}

// Image Type

void RTRT::setCreateImageCallback( CreateImageCallback* const callback ) {
  create_image = callback;
}
MantaInterface::CreateImageCallback * RTRT::getCreateImageCallback() const {
  return create_image;
}

// Random Number Generator
void RTRT::setCreateRNGCallback( CreateRNGCallback* const callback ) {
  // TODO(bigler): clean up the old ones and allocate the new ones.
  create_rng = callback;
}
MantaInterface::CreateRNGCallback* RTRT::getCreateRNGCallback() const {
  return create_rng;
}


// Load Balancer.

void RTRT::setLoadBalancer( LoadBalancer* load_balancer_ ) {
  currentLoadBalancer = load_balancer_;
}

LoadBalancer* RTRT::getLoadBalancer() const {
  return currentLoadBalancer;
}

// Pixel Sampler

void RTRT::setPixelSampler( PixelSampler* sampler_ ) {
  currentPixelSampler = sampler_;
}

PixelSampler* RTRT::getPixelSampler() const {
  return currentPixelSampler;
}

// Renderer

void RTRT::setRenderer( Renderer* renderer_ ) {
  currentRenderer = renderer_;
}

Renderer* RTRT::getRenderer(void) const {
  return currentRenderer;
}

// Sample Generators
void RTRT::setSampleGenerator(SampleGenerator* sample_gen) {
  currentSampleGenerator = sample_gen;
}
SampleGenerator* RTRT::getSampleGenerator() const {
  return currentSampleGenerator;
}

// Idle Mode

RTRT::IdleModeHandle RTRT::addIdleMode(IdleMode* idle_mode)
{
  currentIdleModes.push_back(idle_mode);
  return currentIdleModes.size()-1;
}

IdleMode *RTRT::getIdleMode( IdleModeHandle i ) const {
  return currentIdleModes[i];
}

void RTRT::setShadowAlgorithm(ShadowAlgorithm* shadows)
{
  currentShadowAlgorithm = shadows;
}

ShadowAlgorithm* RTRT::getShadowAlgorithm(void) const
{
  return currentShadowAlgorithm;
}

void RTRT::setScene(Scene* newScene)
{
  scene = newScene;
}

Scene* RTRT::getScene() { return scene; }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Transactions.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RTRT::addTransaction(TransactionBase* transaction)
{
  transaction_lock.lock();
  transactions.push_back(transaction);
  transaction_lock.unlock();
}

#include <Interface/RayPacket.h>

//
Barrier* RTRT::getPreprocessBarrier()
{
  return &preprocess_barrier;
}

///////////////////////////////////////////////////////////////////////////////

// Experimental. Shoot one ray from the calling thread. Populates the result ray
// packet datastructure. Useful for debugging. Very un-thread-safe.

void RTRT::shootOneRay( Color &result_color, RayPacket &result_rays, Real image_x, Real image_y, int channel_index ) {

  // Only shoot one ray.
  //  result_rays.resize( 1 );

  // Set the image space coordinates of the pixel.
  result_rays.setPixel(0, 0, image_x, image_y);
  result_rays.setFlag ( RayPacket::HaveImageCoordinates );

  // Get a pointer to the channel.
  Channel *channel = channels[ channel_index ];

  // Create a render context.
  RenderContext render_context(this, channel_index, workersRendering, 0, 0,
                               currentLoadBalancer, currentPixelSampler,
                               currentRenderer, currentShadowAlgorithm,
                               channel->camera, scene, thread_storage,
                               rngs[0],
                               currentSampleGenerator);

  // Send this to the renderer.  It will fill in the colors for us.
  currentRenderer->traceEyeRays( render_context, result_rays );

  result_color = result_rays.getColor(0);
}




