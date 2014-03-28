
#include <Core/Util/Preprocessor.h>
#include <Core/Util/UpdateGraph.h>
#include <Interface/Context.h>
#include <Model/Groups/Group.h>
#include <Interface/InterfaceRTTI.h>
#include <Core/Persistent/stdRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Util/Assert.h>
#include <algorithm>
#include <iostream>
using std::cerr;
using std::endl;

using namespace Manta;

Group* Group::create(const vector<string>& args)
{
  return new Group(args);
}

Group::Group(const vector<string>& /* args */)
  : dirtybbox(true),
    barrier("group barrier"),
    mutex("group mutex")
{
}

Group::Group()
  : dirtybbox(true),
    barrier("group barrier"),
    mutex("group mutex")
{
}

Group::~Group()
{
}

Group* Group::clone(CloneDepth depth, Clonable* incoming)
{
  Group* copy;
  if (incoming)
    copy = dynamic_cast<Group*>(incoming);
  else
    copy = new Group();

  //since we need to make clones of the things contained in the group,
  //we can't just do copy->objs = objs;
  for(vector<Object*>::iterator iter = objs.begin(); iter != objs.end(); ++iter) {
    Object* obj = *iter;
    copy->objs.push_back(dynamic_cast<Object*>(obj->clone(depth)));
  }

  copy->bbox = bbox;
  copy->dirtybbox = dirtybbox;

  return copy;
}

Interpolable::InterpErr
Group::serialInterpolate(const std::vector<keyframe_t>& group_keyframes)
{
  return parallelInterpolate(group_keyframes, 0, 1);
}

Interpolable::InterpErr
Group::parallelInterpolate(const std::vector<keyframe_t>& group_keyframes,
                           int proc, int numProc)
{
  if (proc == 0)
    setDirty();

  InterpErr worstError = success;
  vector<keyframe_t> keyframes(group_keyframes);

  Group** groups = MANTA_STACK_ALLOC(Group*, group_keyframes.size());
  for(size_t frame=0; frame < keyframes.size(); ++frame) {
    Group *group = dynamic_cast<Group*>(group_keyframes[frame].keyframe);
    if (group == NULL)
      return notInterpolable;
    groups[frame] = group;
    ASSERT(group->size() == size());
  }

  //Do the serial objects in parallel
  size_t serialSize = (parallelSplit - objs.begin());
  size_t start = proc*serialSize/numProc;
  size_t end = (proc+1)*serialSize/numProc;
  for (size_t i=start; i < end; ++i) {
    for(size_t frame=0; frame < keyframes.size(); ++frame) {
      keyframes[frame].keyframe = groups[frame]->get(i);
    }
    InterpErr retcode = objs[i]->serialInterpolate(keyframes);
    if (retcode != success)
      worstError = retcode;
  }

  //now do the parallel objects
  for (size_t i=serialSize; i < objs.size(); ++i) {
    for(size_t frame=0; frame < keyframes.size(); ++frame) {
      keyframes[frame].keyframe = groups[frame]->get(i);
    }
    InterpErr retcode = objs[i]->parallelInterpolate(keyframes, proc, numProc);
    if (retcode != success)
      worstError = retcode;
  }

  barrier.wait(numProc);

  return worstError;
}

void Group::add(Object* obj)
{
  objs.push_back(obj);
  setDirty();
}

void Group::set(size_t i, Object* obj) {
  ASSERT( i < objs.size() );
  objs[i] = obj;
  setDirty();
}

void Group::remove(Object* obj, bool delete_ptr) {
  vector<Object*>::iterator iter = find(objs.begin(), objs.end(), obj);
  if (iter != objs.end()) {
    objs.erase(iter);
    if (delete_ptr) {
      delete obj;
    }
    setDirty();
  }
}

bool Group::isDirty() const{
  return dirtybbox;
}

void Group::setDirty()
{
  dirtybbox = true;
}

void Group::shrinkTo(size_t firstNumObjs, bool deleteRemainder)
{
  if (deleteRemainder)
    for(size_t i=firstNumObjs;i < objs.size(); i++)
      delete objs[i];
  objs.resize(firstNumObjs);
  setDirty();
}

//used by partition
bool isSerial(Interpolable *i)
{
  return !i->isParallel();
}

void Group::preprocess(const PreprocessContext& context)
{
  //partition so that objs are serial and then parallel
  if (context.proc == 0) {
    parallelSplit = partition(objs.begin(), objs.end(), isSerial);
    setDirty();
  }
  // NOTE(boulos): This is actually a barrier wait, and it's important
  // to use that here since we don't want the objs changing underneath
  // us for other threads
  context.done();

  size_t serialSize = (parallelSplit - objs.begin());
  size_t start = context.proc*serialSize/context.numProcs;
  size_t end = (context.proc+1)*serialSize/context.numProcs;
  PreprocessContext serialContext(context);
  serialContext.proc = 0;
  serialContext.numProcs = 1;
  for (size_t i=start; i < end; ++i) {
    objs[i]->preprocess(serialContext);
  }

  //now do the parallel objects
  for (size_t i=serialSize; i < objs.size(); ++i) {
    objs[i]->preprocess(context);
  }

  context.done(); // barrier
}

void Group::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  if (dirtybbox) {
    computeBounds(context, context.proc, context.numProcs);
  }
  // We need the locking and barrier in case bbox is the same across all
  // threads.
  mutex.lock();
  bbox.extendByBox(this->bbox);
  mutex.unlock();
  context.done();
}

void Group::computeBounds(const PreprocessContext& context,
                          int proc, int numProcs) const
{
  BBox myBBox;
  PreprocessContext dummyContext;

  if (proc == 0) {
    this->bbox.reset();
  }

  //Compute Bounding boxes in parallel
  size_t start = proc*size()/numProcs;
  size_t end = (proc+1)*size()/numProcs;
  for (size_t i=start; i < end; ++i) {
    objs[i]->computeBounds(dummyContext, myBBox);
  }

  //this barrier enforces that bbox has been initialized before
  //threads start writing to it.
  barrier.wait(numProcs);

  mutex.lock();
  this->bbox.extendByBox(myBBox);
  mutex.unlock();

  if (proc == 0) {
    dirtybbox = false;
  }

  //Need to wait for other threads to finish computing bbox
  barrier.wait(numProcs);
}


void Group::intersect(const RenderContext& context, RayPacket& rays) const
{
  for(vector<Object*>::const_iterator iter = objs.begin(); iter != objs.end(); ++iter)
    (*iter)->intersect(context, rays);
}

void Group::addToUpdateGraph(ObjectUpdateGraph* graph,
                             ObjectUpdateGraphNode* parent) {
  // Insert myself underneath my parent
  ObjectUpdateGraphNode* node = graph->insert(this, parent);
  // Ask the underlying objects to insert themselves underneath me if they want
  for (size_t i = 0; i < objs.size(); i++) {
    objs[i]->addToUpdateGraph(graph, node);
  }
}

namespace Manta {
  MANTA_REGISTER_CLASS(Group);
}

void Group::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Object>::readwrite(archive, *this);
  archive->readwrite("objs", objs);
}

