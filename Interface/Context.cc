#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Core/Thread/Barrier.h>

using namespace Manta;
// NOTE(boulos): I believe these default values will result in correct
// behavior when people create "dummy" contexts that are mixed with
// the appropriate ones that call done.
PreprocessContext::PreprocessContext() :
  manta_interface(0),
  proc(0),
  numProcs(1),
  globalLights(0),
  barrier(0)
{
}

PreprocessContext::PreprocessContext(const PreprocessContext& context) :
  manta_interface(context.manta_interface),
  proc(context.proc),
  numProcs(context.numProcs),
  globalLights(context.globalLights),
  barrier(context.barrier)
{
}

PreprocessContext::PreprocessContext(MantaInterface* manta_interface,
                                     int proc,
                                     int numProcs,
                                     LightSet* globalLights) :
  manta_interface(manta_interface),
  proc(proc),
  numProcs(numProcs),
  globalLights(globalLights),
  barrier(manta_interface->getPreprocessBarrier())
{
}

PreprocessContext::~PreprocessContext() {
  // Don't delete barrier.  We didn't allocate it.
}

void PreprocessContext::done() const {
  if (barrier && numProcs > 1) barrier->wait(numProcs);
}

Barrier* PreprocessContext::setBarrier(Barrier* new_barrier)
{
  Barrier* old = barrier;
  barrier = new_barrier;
  return old;
}


UpdateContext::UpdateContext(MantaInterface* manta_interface,
                             unsigned int proc,
                             unsigned int num_procs) :
  manta_interface(manta_interface),
  proc(proc),
  num_procs(num_procs) {
}

void UpdateContext::finish(Object* which_object) const {
  const UpdateContext& context = *this;
  manta_interface->finishUpdates(which_object, context);
}
void UpdateContext::insertWork(TaskList* new_work) const {
  manta_interface->insertWork(new_work);
}
