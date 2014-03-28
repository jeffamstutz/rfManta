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

#include <UserInterface/AutomatorUI.h>
#include <Core/Thread/Thread.h>
#include <Core/Util/Callback.h>

using namespace Manta;

AutomatorUI::AutomatorUI( MantaInterface *manta_interface_,
                          int warmup_frames_, bool is_detached_ ) :

  this_thread( 0 ),
  startup_semaphore( "AutomatorUI semaphore", 0 ),
  manta_interface( manta_interface_ ),
  warmup_frames( warmup_frames_ ),
  is_detached( is_detached_ ),
  automator_mode( AUTOMATOR_EXIT ),
  terminate_callback( 0 )
{
}

AutomatorUI::~AutomatorUI() {

  // Delete the terminate callback if it was ever set.
  if (terminate_callback != 0) {
    delete terminate_callback;
  }
}

void AutomatorUI::run() {

  // Have manta call release_automator after warmup_frames.
  manta_interface->addOneShotCallback( MantaInterface::Relative, warmup_frames,
                                       Callback::create(this, &AutomatorUI::release_automator) );

  do {

    // Wait for the manta rendering threads to signal the semaphore.
    startup_semaphore.down();

    // Run the automator function.
    run_automator();

    // Send a final transaction to make sure all of the state changes sent from
    // run_automator are processed before the thread exits.
    getMantaInterface()->addTransaction("AutomatorUI finished",
                                        Callback::create(this, &AutomatorUI::release_automator,
                                                         0, 0));

    // Wait for manta to process the transaction.
    startup_semaphore.down();

    // Call the termination callback if it exists.
    if (terminate_callback) {
      // terminate_callback->call();
      getMantaInterface()->addTransaction("AutomatorUI Termination",
                                          terminate_callback );      
    }
  }
  while (automator_mode == AUTOMATOR_KEEPALIVE);

  // Thread will terminate.
  this_thread = 0;
}

void AutomatorUI::startup() {

  // Check to see if the automator is already started.
  if (this_thread == 0) {

    // Create a thread for the automator.
    this_thread = new Thread( this, "AutomatorUI" );

    if (is_detached) {
      this_thread->setDaemon( true );
      this_thread->detach();
    }
  }
}

void AutomatorUI::restart() {

  // Make sure a thread was created.
  if (this_thread) {

    // Have manta call release_automator immediately.
    manta_interface->addOneShotCallback( MantaInterface::Relative, 0,
                                         Callback::create(this, &AutomatorUI::release_automator) );
  }

  // Otherwise start a thread.
  else {
    startup();
  }
}

void AutomatorUI::release_automator(int, int) {

  // Signal the semaphore.
  startup_semaphore.up();
}







