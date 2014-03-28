#ifndef KeyFrameAnimation_h
#define KeyFrameAnimation_h

#include <Interface/AccelerationStructure.h>
#include <Model/Groups/Group.h>
#include <Core/Thread/Barrier.h>

namespace Manta {
  struct Temp_Callback {int proc, numProcs;
    float time;};//temporary so I can get code to compile

  class KeyFrameAnimation : public Object{
  public:
    enum InterpolationMode{truncate, linear, fixed};

    KeyFrameAnimation(InterpolationMode interpolation=truncate);
    ~KeyFrameAnimation();

    //add a keyframe
    void push_back(Group *objects);

    //number of seconds the animation takes from start to end
    void setDuration(float time) { duration = time; };
    float getDuration() const { return duration; }

    void startAnimation();
    void pauseAnimation();
    void resumeAnimation();

    void temporaryUpdate(int proc, int numProcs, bool &); //this will be replaced by update
    void update(Temp_Callback context);

    //! make sure every frame is shown at least once
    void lockFrames(bool st);
    //! clip animation to loop from start to end
    void clipFrames(int start, int end);
    void repeatLastFrameForSeconds(float time);
    void loopAnimation(bool st);

    //set animation to a specific time (frame)
    //returns whether the frame is different from the previous frame.
    bool setTime(float time);
    void setTimeVoid(float time) { setTime(time); }

    //get the time of the frame currently being worked on.
    float getTime() const { return currTime; }

    //get the group for the frame currently being worked on.
    void getCurrGroup(Group *&group) const {
      group = currGroup;
    }

    bool isDifferentFrame(float time) const;

    void setInterpolation(InterpolationMode mode);

    void intersect(const RenderContext& context, RayPacket& rays) const;

    void preprocess(const PreprocessContext& context);

    void computeBounds(const PreprocessContext& context, BBox& bbox) const;

    //TODO: should we instead have a collection of acceleration structures?
    void useAccelerationStructure(AccelerationStructure* as);

    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent);

    virtual bool isParallel() const { return true; }

    AccelerationStructure *as;

  private:

    float wrapTime(float time) const;

    InterpolationMode interpolation;

    Group *spareGroup;  //used for interpolated frame -- has already allocated memory.
    Group *currGroup;   //The group for the frame specified by setTime()

    //The time associated with the current frame being rendered.
    //Make sure this is from [0, duration).
    float currTime;

    double startTime;    //world time of animation start
    double pauseTime;    //world time at pause

    bool updateToCurrTime; //use user supplied time instead of system time
    double newTime;        //the user supplied time, normalized to [0, duration].

    bool paused;

    Barrier barrier;
    bool differentFrame; //used by setTime()

    vector<Group*> frames;
    float duration; //how many seconds long

    bool lockedFrames; // make sure that no frames are skipped when setting time, each frame will be shown at least once
    int startFrame;
    int endFrame;
    int numFrames;
    bool framesClipped;
    bool loop;
    float repeatLastFrame; //this will repeat the last frame for the given number of seconds (on top of the duration)
    bool repeating;
    bool forceUpdate;
  };
}
#endif
