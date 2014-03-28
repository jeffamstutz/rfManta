
#ifndef Manta_Interface_Context_h
#define Manta_Interface_Context_h

namespace Manta {
  class Barrier;
  class Camera;
  class FrameState;
  class LightSet;
  class LoadBalancer;
  class Object;
  class PixelSampler;
  class ReadContext;
  class Renderer;
  class MantaInterface;
  class SampleGenerator;
  class Scene;
  class ShadowAlgorithm;
  class TaskList;
  class XWindow;
  class ThreadStorage;
  class RandomNumberGenerator;

  class ReadContext {
  public:
    ReadContext(MantaInterface* rtrt_int)
      : manta_interface(rtrt_int)
    {
    }
    mutable MantaInterface* manta_interface;

  private:
    ReadContext(const ReadContext&);
    ReadContext& operator=(const ReadContext&);
  };




  /////////////////////////////////////////////////////////////////////////////
  // Setup context is used to configure rendering stack components.



  class SetupContext {
  public:
    SetupContext(MantaInterface* rtrt_int,
                 int channelIndex, int numChannels, int proc, int numProcs,
                 bool stereo, int xres, int yres,
                 LoadBalancer* loadBalancer,  PixelSampler* pixelSampler,
                 Renderer* renderer,
                 SampleGenerator* sample_gen,
                 ThreadStorage *storage_allocator_)
      : rtrt_int(rtrt_int),
        channelIndex(channelIndex), numChannels(numChannels),
        proc(proc), numProcs(numProcs),
        loadBalancer(loadBalancer), pixelSampler(pixelSampler),
        renderer(renderer), sample_generator(sample_gen),
        storage_allocator( storage_allocator_ ),
        stereo(stereo), xres(xres), yres(yres)
    {
      init();
    }
    SetupContext(MantaInterface* rtrt_int, int numChannels,
                 int proc, int numProcs,
                 LoadBalancer* loadBalancer,  PixelSampler* pixelSampler,
                 Renderer* renderer,
                 SampleGenerator* sample_gen)
      : rtrt_int(rtrt_int), channelIndex(-1), numChannels(numChannels),
        proc(proc), numProcs(numProcs),
        loadBalancer(loadBalancer), pixelSampler(pixelSampler),
        renderer(renderer),
        sample_generator(sample_gen),
        stereo(false), xres(-1), yres(-1)
    {
      init();
    }
    void init() {
      minPipelineDepth =1;
      maxPipelineDepth = 1000;
      masterWindow = 0;
    }
    MantaInterface* rtrt_int;
    int channelIndex;
    int numChannels;
    int proc;
    int numProcs;
    double currentTime;
    LoadBalancer* loadBalancer;
    PixelSampler* pixelSampler;
    Renderer* renderer;
    SampleGenerator* sample_generator;
    ThreadStorage* storage_allocator;

    mutable XWindow* masterWindow;

    void changeResolution(bool new_stereo, int new_xres, int new_yres) {
      if(new_stereo != stereo || new_xres != xres || new_yres != yres){
        stereo = new_stereo;
        xres = new_xres;
        yres = new_yres;
        changed=true;
      }
    }
    void getResolution(bool& out_stereo, int& out_xres, int& out_yres) const {
      out_stereo = stereo;
      out_xres = xres;
      out_yres = yres;
    }

    void constrainPipelineDepth(int newmin, int newmax) {
      if(newmin > minPipelineDepth)
        minPipelineDepth = newmin;
      if(newmax < maxPipelineDepth)
        maxPipelineDepth = newmax;
    }
    int getMinDepth() const {
      return minPipelineDepth;
    }
    int getMaxDepth() const {
      return maxPipelineDepth;
    }

    bool isChanged() const {
      return changed;
    }
    void setChanged(bool to) {
      changed = to;
    }
    void clearMasterWindow() {
      masterWindow = 0;
    }
  private:
    SetupContext(const SetupContext&);
    SetupContext& operator=(const SetupContext&);

    bool stereo;
    int xres, yres;
    int minPipelineDepth, maxPipelineDepth;
    bool changed;

  };
  class DisplayContext {
  public:
    DisplayContext(int proc, int numProcs, int frameIndex, int pipelineDepth)
      : proc(proc), numProcs(numProcs), frameIndex(frameIndex), pipelineDepth(pipelineDepth)
    {
    }
    int proc;
    int numProcs;
    int frameIndex;
    int pipelineDepth;
  private:
    DisplayContext(const DisplayContext&);
    DisplayContext& operator=(const DisplayContext&);
  };
  class RenderContext {
  public:
    RenderContext(MantaInterface* rtrt_int,
                  int channelIndex, int proc, int numProcs,
                  const FrameState* frameState,
                  LoadBalancer* loadBalancer, PixelSampler* pixelSampler,
                  Renderer* renderer, ShadowAlgorithm* shadowAlgorithm,
                  const Camera* camera, const Scene* scene,
                  ThreadStorage *storage_allocator_,
                  RandomNumberGenerator* rng,
                  SampleGenerator* sample_gen)
      : rtrt_int(rtrt_int), channelIndex(channelIndex),
        proc(proc), numProcs(numProcs),
        frameState(frameState),
        loadBalancer(loadBalancer), pixelSampler(pixelSampler),
        renderer(renderer),
        sample_generator(sample_gen),
        shadowAlgorithm(shadowAlgorithm),
        camera(camera), scene(scene),
        storage_allocator( storage_allocator_ ),
        rng(rng)
    {
    }
    MantaInterface* rtrt_int;
    int channelIndex;
    int proc;
    int numProcs;
    const FrameState* frameState;
    LoadBalancer* loadBalancer;
    PixelSampler* pixelSampler;
    Renderer* renderer;
    SampleGenerator* sample_generator;
    ShadowAlgorithm* shadowAlgorithm;
    const Camera* camera;
    const Scene* scene;

    mutable ThreadStorage *storage_allocator;

    RandomNumberGenerator* rng;
  private:
    RenderContext(const RenderContext&);
    RenderContext& operator=(const RenderContext&);
  };

  class PreprocessContext {
  public:
    PreprocessContext();
    PreprocessContext(const PreprocessContext& context);
    PreprocessContext(MantaInterface* manta_interface,
                      int proc,
                      int numProcs,
                      LightSet* globalLights);
    ~PreprocessContext();

    // Set the barrier.  You shouldn't need to call this if you used
    // the constructor that takes a manta_interface.  It returns the
    // old barrier.
    Barrier* setBarrier(Barrier* barrier);

    // Call this when you're done changing your state values during a
    // preprocess call.  It ensures that all the threads finish
    // together.
    void done() const;

    // Does the context actually contain useful information?
    bool isInitialized() const { return manta_interface != 0; }

    MantaInterface* manta_interface;
    int proc;
    int numProcs;
    LightSet* globalLights;
  private:
    mutable Barrier* barrier;

    PreprocessContext& operator=(const PreprocessContext&);
  };

  class UpdateContext {
  public:
    UpdateContext(MantaInterface* manta_interface,
                  unsigned int proc,
                  unsigned int num_procs);

    void finish(Object* which_object) const;
    void insertWork(TaskList* new_work) const;

    MantaInterface* manta_interface;
    unsigned int proc;
    unsigned int num_procs;
  private:
    UpdateContext() {}
  };
}

#endif
