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

#ifndef Manta_Engine_RTRT_h
#define Manta_Engine_RTRT_h

#include <Interface/MantaInterface.h>
#include <Interface/FrameState.h>
#include <Interface/Object.h>
#include <Parameters.h>
#include <Core/Thread/AtomicCounter.h>
#include <Core/Thread/Barrier.h>
#include <Core/Thread/CrowdMonitor.h>
#include <Core/Thread/Mutex.h>
#include <Core/Thread/Semaphore.h>
#include <Core/Thread/Runnable.h>
#include <Core/Util/AlignedAllocator.h>
#include <Core/Util/ThreadStorage.h>
#include <map>
#include <set>
#include <vector>
#include <list>

namespace Manta {
  using namespace std;

  class Thread;
  class CallbackHandle;
  class Camera;
  class Image;
  class Scene;
  class RandomNumberGenerator;
  class TaskQueue;

  class RTRT : public MantaInterface,
               public AlignedAllocator<RTRT, MAXCACHELINESIZE> {
  public:
    RTRT();
    ~RTRT();

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Pipeline Components.
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Image Modes (opengl, file, mpeg, etc.)
    virtual int createChannel(ImageDisplay *image_display, Camera* camera, bool stereo, int xres, int yres);

    virtual ImageDisplay *getImageDisplay( int channel );
    virtual void setImageDisplay( int channel, ImageDisplay *display );

    virtual Camera* getCamera(int channel) const;
                virtual void    setCamera(int channel, Camera *camera );

    virtual void getResolution(int channel, bool& stereo, int& xres, int& yres);
    virtual void changeResolution(int channel, int xres, int yres, bool changePipeline);

    virtual void setDisplayBeforeRender(bool setting) {
      displayBeforeRender = setting;
    }
    virtual bool getDisplayBeforeRender() {
      return displayBeforeRender;
    }

    // Image Traversers
    virtual void setImageTraverser( ImageTraverser *image_traverser_ );
    virtual ImageTraverser *getImageTraverser() const;

    // Image Types (rgb, rgba, float, etc.)
    virtual void setCreateImageCallback( CreateImageCallback * const callback );
    virtual CreateImageCallback * getCreateImageCallback() const;

    // Random Number Generator
    virtual void setCreateRNGCallback( CreateRNGCallback* const callback );
    virtual CreateRNGCallback* getCreateRNGCallback() const;

    // Load Balancers
    virtual void setLoadBalancer( LoadBalancer *load_balancer_ );
    virtual LoadBalancer *getLoadBalancer() const;

    // PixelSamplers
                virtual void setPixelSampler( PixelSampler *sampler_ );
    virtual PixelSampler *getPixelSampler() const;

    // Renderers
    virtual void setRenderer( Renderer *renderer_ );
    virtual Renderer* getRenderer() const;

    // Sample Generators
    virtual void setSampleGenerator(SampleGenerator* sample_gen);
    virtual SampleGenerator* getSampleGenerator() const;

    // Shadow Algorithms
    virtual void setShadowAlgorithm(ShadowAlgorithm* shadows);
    virtual ShadowAlgorithm* getShadowAlgorithm() const;

    // Idle modes
    virtual IdleModeHandle addIdleMode( IdleMode *idle_mode );
    virtual IdleMode *getIdleMode( IdleModeHandle i ) const;

    // Scenes
    virtual void setScene(Scene* scene);
                virtual Scene *getScene();

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Time Mode
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Control of time/animation
    virtual void setTime( double time_ );
    virtual void setTimeMode(TimeMode tm, double rate);
    virtual void startTime();
    virtual void stopTime();
    virtual bool timeIsStopped();

    // Query functions
    virtual const FrameState& getFrameState() const { return animFrameState; };

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Control
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Parallel processing
    virtual void changeNumWorkers(int workers);
    virtual TValue<int>& numWorkers();

    // Control
    virtual void beginRendering(bool blockUntilFinished) throw (Exception &);
    virtual void blockUntilFinished();
    virtual void finish();

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Transactions
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Transactions
    virtual void addTransaction(TransactionBase* );

        // The object has finished it's updating, let someone know
    virtual void finishUpdates(Object* which_object, const UpdateContext& context);
    // There is new work to be done for this Object
    virtual void insertWork(TaskList* new_work);

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Callbacks.
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Callbacks
    virtual void addOneShotCallback(OneShotTime whence, FrameNumber frame, CallbackBase_2Data<int, int>* callback);
    virtual void addParallelOneShotCallback(OneShotTime whence, FrameNumber frame, CallbackBase_2Data<int, int>* callback);

    virtual CallbackHandle* registerSetupCallback(SetupCallback*);
    virtual CallbackHandle* registerSerialAnimationCallback(CallbackBase_3Data<int, int, bool&>*);
    virtual CallbackHandle* registerParallelAnimationCallback(CallbackBase_3Data<int, int, bool&>*);
    virtual CallbackHandle* registerSerialPreRenderCallback(CallbackBase_2Data<int, int>*);
    virtual CallbackHandle* registerParallelPreRenderCallback(CallbackBase_2Data<int, int>*);
    virtual CallbackHandle* registerTerminationCallback( CallbackBase_1Data< MantaInterface *> *);
    virtual void unregisterCallback(CallbackHandle*);

    //
    virtual Barrier* getPreprocessBarrier();

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Debug
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Debug:
    virtual void shootOneRay( Color &result_color, RayPacket &result_rays, Real image_x, Real image_y, int channel_index );

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // End of Public Interface.

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

        protected:
    void internalRenderLoop(int workerIndex, bool lateComerFlag);

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Channel Structure.
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    struct Channel {
      int id;                   // The index into the list of
                                // channels, use this index to access
                                // channel specifics through other
                                // functions.
      ImageDisplay* display;
      int xres, yres;
      bool stereo;
      int pipelineDepth;
      bool active;
      CreateImageCallback* create_image;
      vector<Image*> images;
      Camera* camera;
    };
    typedef vector<Channel*> ChannelListType;
    ChannelListType channels;

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Worker class for the RTRT Pipeline.
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    class Worker : public Runnable {
    public:
      Worker(RTRT* rtrt, int workerIndex, bool lateComerFlag);
      virtual ~Worker();

      virtual void run();
    private:
      RTRT* rtrt;
      int workerIndex;
      bool lateComerFlag;
    };

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Pipeline Configuration.
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    ImageTraverser*  currentImageTraverser;
    LoadBalancer*    currentLoadBalancer;
    PixelSampler*    currentPixelSampler;
    Renderer*        currentRenderer;
    SampleGenerator* currentSampleGenerator;
    ShadowAlgorithm* currentShadowAlgorithm;
    vector<IdleMode*> currentIdleModes;
    bool displayBeforeRender;

    CreateImageCallback* create_image;
    CreateRNGCallback*   create_rng;

  private:
    RTRT(const RTRT&);
    RTRT& operator=(const RTRT&);

    // Callback Helpers.
    void doParallelAnimationCallbacks(bool& changed, int proc, int numProcs);
    void doSerialAnimationCallbacks(bool& changed, int proc, int numProcs);

    void doPreprocess(int proc, int numProcs);
    void doParallelPreRenderCallbacks(int proc, int numProcs);
    void doSerialPreRenderCallbacks(int proc, int numProcs);
    void doUpdates(bool& changed, int proc, int numProcs);

    void doIdleModeCallbacks(bool changed, bool firstFrame, bool& pipelineNeedsSetup, int proc, int numProcs);
    void doTerminationCallbacks();

    void resizeImages(long frameNumber);
    void setupPipelines(int numProcs);

    // Worker Thread Info.
    TValue<int> workersWanted;
    int workersRendering;
    int workersAnimAndImage;
    bool workersChanged;
    vector<Thread*> workers;

    // Running flag.
    Mutex runningLock;
    bool running;

    // Pipeline Synchronization barriers.
    CrowdMonitor callbackLock;
    Barrier barrier1;
    Barrier barrier2;
    Barrier barrier3;
    Barrier workers_changed_barrier;
    Barrier preprocess_barrier;
#if defined(__APPLE__)
    // For some reason, on OS X 10.7 (maybe it's the default gcc compiler with
    // xcode 4) we get a seg fault at startup if alignment is 32 or larger.
    // Until we figure that out, here's a quick less than ideal fix.
    struct MANTA_ALIGN(8)
#else
    struct MANTA_ALIGN(MAXCACHELINESIZE)
#endif
    ReductionData {
      bool changed;
    };
    vector<ReductionData> changedFlags;
    bool lastChanged;

    ///////////////////////////////////////////////////////////////////////////
    // Callbacks Queues

    typedef multimap<long, CallbackBase_2Data<int, int>*, less<long> > OneShotMapType;
    typedef multimap<long, CallbackBase_2Data<int, int>*, less<long> > ParallelOneShotMapType;

    OneShotMapType         oneShots;
    ParallelOneShotMapType parallelOneShots;

    // Helper function.
    void deleteParallelOneShot( ParallelOneShotMapType::iterator iter );

    typedef vector<CallbackBase_2Data<int, int>*> PRCallbackMapType;
    typedef vector<CallbackBase_3Data<int, int, bool&>*> ACallbackMapType;
    typedef vector<CallbackBase_1Data<MantaInterface *>*> TCallbackMapType;
    typedef vector<SetupCallback *>                       SCallbackMapType;

    // Callback lists.
    PRCallbackMapType parallelPreRenderCallbacks;
    PRCallbackMapType serialPreRenderCallbacks;
    ACallbackMapType  parallelAnimationCallbacks;
    ACallbackMapType  serialAnimationCallbacks;
    SCallbackMapType  setupCallbacks;
    TCallbackMapType  terminationCallbacks;

    ///////////////////////////////////////////////////////////////////////////
    // Transactions
    typedef list<TransactionBase*> TransactionListType;

    TransactionListType transactions;
    Mutex transaction_lock;

    // Transaction helper.
    void postTransactions(bool& changed);
    bool verbose_transactions;

    // The pipeline
    bool pipelineNeedsSetup;

    // Time control
    TimeMode timeMode;
    double timeScale;
    double timeOffset;
    double stoptime;
    bool time_is_stopped;
    double frameRate;

    MANTA_ALIGN(MAXCACHELINESIZE) char pad0;
    MANTA_ALIGN(MAXCACHELINESIZE) FrameState animFrameState;
    MANTA_ALIGN(MAXCACHELINESIZE) char pad1;
    MANTA_ALIGN(MAXCACHELINESIZE) FrameState renderFrameState;
    MANTA_ALIGN(MAXCACHELINESIZE) char pad2;

    // Thread local storage allocator.
    ThreadStorage *thread_storage;

    // RandomNumberGenerator
    vector<RandomNumberGenerator*> rngs;

    Scene* scene;

    Mutex channel_create_lock;

    ObjectUpdateGraph* update_graph;
    TaskQueue* update_work_queue;
  };
}

#endif
