
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

#ifndef Manta_Interface_MantaInterface_H
#define Manta_Interface_MantaInterface_H

#include <Core/Util/Callback.h>
#include <Interface/Transaction.h>
#include <Core/Exceptions/Exception.h>
#include <MantaTypes.h>
#include <string>
#include <vector>

namespace Manta {

  using namespace std;

  class Barrier;
  class Camera;
  class CameraPath;
  class FrameState;
  class Group;
  class IdleMode;
  class Image;
  class ImageDisplay;
  class ImageTraverser;
  class LoadBalancer;
  class Object;
  class PixelSampler;
  class RandomNumberGenerator;
  class RayPacket;
  class Renderer;
  class SampleGenerator;
  class Scene;
  class SetupCallback;
  class ShadowAlgorithm;
  class TaskList;
  class UpdateContext;
  class UserInterface;

  class MantaInterface {
  public:

    virtual ~MantaInterface();

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Pipeline Components.
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Image Displays (opengl, file, mpeg, etc.)

    // Create a channel given a pointer to the ImageDisplay for the channel.
    virtual int createChannel( ImageDisplay *image_display, Camera *camera, bool stereo, int xres, int yres) = 0;
    virtual ImageDisplay *getImageDisplay( int channel ) = 0;
    virtual void setImageDisplay( int channel, ImageDisplay *display ) = 0;

    virtual Camera* getCamera(int channel) const = 0;
    virtual void    setCamera(int channel, Camera *camera ) = 0;
    virtual void getResolution(int channel, bool& stereo, int& xres, int& yres) = 0;

    // You can change the resolution of the rendered image without
    // having to change the pipeline.  If you want the pipeline
    // changed, then set this parameter to true.
    virtual void changeResolution(int channel, int xres, int yres, bool changePipeline) = 0;

    // An option which changes the order of the two stages in the pipeline.
    virtual void setDisplayBeforeRender(bool setting) { cerr << "ignoring pipeline stage command." << endl; };
    virtual bool getDisplayBeforeRender() { return false; }

    // Idle modes
    typedef unsigned int IdleModeHandle;
    virtual IdleModeHandle addIdleMode( IdleMode* idle_mode_ ) = 0;
    virtual IdleMode* getIdleMode( IdleModeHandle i ) const = 0;

    /*
     * Render Stack Components.
     */

    // Image Traversers
    virtual void setImageTraverser( ImageTraverser* image_traverser_ ) = 0;
    virtual ImageTraverser* getImageTraverser() const = 0;

    // Image Types (rgb, rgba, float, etc.)
    typedef CallbackBase_4Data<bool,int,int,Image*&> CreateImageCallback;
    virtual void setCreateImageCallback( CreateImageCallback* const callback ) = 0;
    virtual CreateImageCallback* getCreateImageCallback() const = 0;

    // Random Number Generator
    //typedef CacheLonelyRNG*(*RNGCreator)();
    typedef CallbackBase_1Data<RandomNumberGenerator*&> CreateRNGCallback;
    virtual void setCreateRNGCallback( CreateRNGCallback* const callback ) = 0;
    virtual CreateRNGCallback* getCreateRNGCallback() const = 0;

    // Load Balancers
    virtual void setLoadBalancer( LoadBalancer* load_balancer_ ) = 0;
    virtual LoadBalancer* getLoadBalancer() const = 0;

    // PixelSamplers
    virtual void setPixelSampler( PixelSampler* sampler_ ) = 0;
    virtual PixelSampler* getPixelSampler() const = 0;

    // Renderers
    virtual void setRenderer( Renderer* renderer_ ) = 0;
    virtual Renderer* getRenderer() const = 0;

    // Sample Generators
    virtual void setSampleGenerator(SampleGenerator* sample_gen) = 0;
    virtual SampleGenerator* getSampleGenerator() const = 0;

    // Shadow Algorithms
    virtual void setShadowAlgorithm(ShadowAlgorithm* shadows) = 0;
    virtual ShadowAlgorithm* getShadowAlgorithm(void) const = 0;

    // Scene
    virtual void setScene(Scene* scene) = 0;
    virtual Scene* getScene() = 0;

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Time Mode
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    enum TimeMode {
      RealTime,
      FixedRate,
      Static
    };
    virtual void setTime( double time_ ) = 0;
    virtual void setTimeMode(TimeMode tm, double rate) = 0;
    virtual void startTime() = 0;
    virtual void stopTime() = 0;
    virtual bool timeIsStopped() = 0;

    // Query functions
    virtual const FrameState& getFrameState() const = 0;

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Control
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Parallel processing
    virtual void changeNumWorkers(int) = 0;
    virtual TValue<int>& numWorkers() = 0;

    // Control
    virtual void beginRendering(bool blockUntilFinished) throw (Exception&) = 0;
    virtual void blockUntilFinished() = 0;

    // Control
    virtual void finish() = 0;

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Transactions
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    virtual void addTransaction(TransactionBase*) = 0;

    template<class T, class Op>
    void addTransaction(const char* name, TValue<T>& value, Op op, int flag = TransactionBase::DEFAULT ) {
      addTransaction(new Transaction<T, Op>(name, value, op, flag));
    }

    void addTransaction(const char* name, CallbackBase_0Data* callback, int flag = TransactionBase::DEFAULT ) {
      addTransaction(new CallbackTransaction(name, callback, flag));
    }

    void addUpdateTransaction(const char* name, CallbackBase_0Data* callback, Object* object, int flag = TransactionBase::DEFAULT ) {
      addTransaction(new UpdateTransaction(name, callback, object, flag));
    }

    // The object has finished it's updating, let someone know
    virtual void finishUpdates(Object* which_object, const UpdateContext& context) = 0;
    // There is new work to be done for this Object
    virtual void insertWork(TaskList* new_work) = 0;

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Callbacks.
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    typedef unsigned long FrameNumber;

    enum OneShotTime {
      // Callbacks
      Absolute,
      Relative
    };

    virtual void addOneShotCallback(OneShotTime whence, FrameNumber frame, CallbackBase_2Data<int, int>* callback) = 0;
    virtual void addParallelOneShotCallback(OneShotTime whence, FrameNumber frame, CallbackBase_2Data<int, int>* callback) = 0;

    virtual CallbackHandle* registerSetupCallback(SetupCallback*) = 0;
    virtual CallbackHandle* registerSerialAnimationCallback(CallbackBase_3Data<int, int, bool&>*) = 0;
    virtual CallbackHandle* registerParallelAnimationCallback(CallbackBase_3Data<int, int, bool&>*) = 0;
    virtual CallbackHandle* registerSerialPreRenderCallback(CallbackBase_2Data<int, int>*) = 0;
    virtual CallbackHandle* registerParallelPreRenderCallback(CallbackBase_2Data<int, int>*) = 0;
    virtual CallbackHandle* registerTerminationCallback( CallbackBase_1Data< MantaInterface *> *) = 0;
    virtual void unregisterCallback(CallbackHandle*) = 0;

    //
    virtual Barrier* getPreprocessBarrier() = 0;

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Debug
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // This should only be called from within a transaction called function.
    virtual void shootOneRay( Color &result_color, RayPacket &result_rays, Real image_x, Real image_y, int channel_index ) = 0;

  protected:
    MantaInterface();

  private:
    MantaInterface(const MantaInterface&);
    MantaInterface& operator=(const MantaInterface&);
  };
  MantaInterface* createManta();
}

#endif
