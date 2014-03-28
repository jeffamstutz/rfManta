/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

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

#ifndef __AUTOMATOR_UI_H__
#define __AUTOMATOR_UI_H__

#include <Interface/MantaInterface.h>
#include <Interface/UserInterface.h>

#include <Core/Thread/Thread.h>
#include <Core/Thread/Runnable.h>
#include <Core/Thread/Semaphore.h>

namespace Manta {

  /////////////////////////////////////////////////////////////////////////////
  // An Automator is an automated user interface. Classes which implement the
  // run method may send transactions to Manta or interact with
  // the renderer in any way that a normal user interface would.
  //
  // The Automator runs in a seperate thread and is completely asynchronous
  // after it is started. The thread is created by a startup but will block so that it
  // doesn't come online until the renderer is ready to accept transactions.
  //
  // Abe Stephens
  class AutomatorUI : public UserInterface, public Runnable {
  private:
    // Thread to run the automator.
    Thread *this_thread;
    
    // Flag to signal that the renderer is ready to begin accepting connections.
    Semaphore startup_semaphore;

    // Interface to communicate with the renderer.
    MantaInterface *manta_interface;
    
    // Number of frames to wait before starting.
    int warmup_frames;

    // Is the thread detached (should it wait for a join?)
    bool is_detached;

    // Should the automator thread wait for a restart message after
    // the run_automator method terminates?
    int automator_mode;

    // Callback to invoke at end of run_automator. Use to notify gui that automator
    // is complete and ready to be restarted.
    CallbackBase_0Data *terminate_callback;
    
  protected:
    inline MantaInterface *getMantaInterface() { return manta_interface; };

  public:
    enum { AUTOMATOR_EXIT, AUTOMATOR_KEEPALIVE };
    
    // Renderer will release the semaphore after "warmup_frames" have been rendered
    AutomatorUI( MantaInterface *manta_interface_, int warmup_frames_ = 0, bool is_detached_ = true );
    virtual ~AutomatorUI();

    // This method must be implemented by Automator implementations.
    virtual void run_automator() = 0;
    
    // Runnable interface.
    void run();

    // UserInterface interface.
    // Start up if not already started (called automatically by restart if necessary).
    void startup();
    
    // Restart the automator if it is blocked waiting for the restart signal.
    void restart();

    // Called by manta to release semaphore.
    void release_automator( int, int );

    // Accessors.
    inline void set_automator_mode( int mode_ ) { automator_mode = mode_; };
    inline int  get_automator_mode() { return automator_mode; };

    inline void set_terminate_callback( CallbackBase_0Data *callback_ ) {
      if (callback_ == 0) {
        delete terminate_callback;
      }
      terminate_callback = callback_;
    };

  };
};

#endif
