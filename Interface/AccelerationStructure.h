#ifndef Manta_Interface_AccelerationStructure_h
#define Manta_Interface_AccelerationStructure_h

#include <Interface/Object.h>
#include <string>

namespace Manta {

  class Group;

  class AccelerationStructure : public Object {
  public:
    AccelerationStructure() : needToSaveFile(false) { /*empty*/ }

    // Tell the acceleration structure that it's working on a new
    // group.  However, you should not actually perform any
    // "construction" work yet as that is reserved for the update and
    // rebuild functions which are called during the preprocess phases
    // of Manta.  setGroup can be called when "building" your scene.
    virtual void setGroup(Group* new_group) = 0;
    // If you modify the group (such as add or remove a piece of geometry), you
    // should call this function to let the acceleration structure know.
    virtual void groupDirty() = 0;

    virtual Group* getGroup() const = 0;

    // update tells the acceleration structure that the underlying
    // geometry has probably "moved" so that the bounds need to be
    // updated. No actual changes in the amount of geometry has
    // occurred (so do not call this function for that purpose).  For
    // some data structures (e.g. a BVH) this can be more efficient
    // than rebuilding. By default, however, this will be a call to
    // rebuild.
    virtual void update(int proc=0, int numProcs=1) {
      rebuild(proc, numProcs);
    }

    // rebuild the data structure over the current group.
    virtual void rebuild(int proc=0, int numProcs=1) = 0;

    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent) = 0;

    virtual bool buildFromFile(const std::string &fileName) {
      return false; //not implemented
    }
    // If the AS is not yet built, the AS will record this request so that when
    // it does get built, it can save itself out to the file when rebuild gets
    // called.
    virtual bool saveToFile(const std::string &fileName) {
      return false; //not implemented
    }

    // The underlying data (group) is parallelizable.
    virtual bool isParallel() const { return true; }

    virtual Interpolable::InterpErr
    serialInterpolate(const std::vector<keyframe_t>& keyframes)
    {
      return parallelInterpolate(keyframes, 0, 1);
    }

  protected:
    bool needToSaveFile;
    std::string saveFileName;
  };
}
#endif //Manta_Interface_AccelerationStructure_h
