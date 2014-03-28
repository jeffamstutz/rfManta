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

#include <UserInterface/SyncFrameAutomator.h>
#include <iostream>
#include <Core/Thread/Mutex.h>
#include <Core/Thread/Time.h>

using namespace Manta;
using namespace std;

extern Mutex sync_lock;

SyncFrameAutomator::SyncFrameAutomator( MantaInterface* manta_interface_,
                                        SyncDisplay* syncd)
  : AutomatorUI(manta_interface_),
    syncd(syncd)
{
}

SyncFrameAutomator::~SyncFrameAutomator()
{
}

// Create method called by RTRT_register.
UserInterface*
SyncFrameAutomator::create( const vector<string>& args,
                            MantaInterface* manta_interface_ )
{
#if 0
  // Create the SyncDisplay
  SyncDisplay* syncd = new Syncd(args);
  // Create the ImageDisplay
  ImageDisplay* id = NULL; // ??(args);
  return new SyncFrameAutomator(manta_interface_, syncd);
#else
  return NULL;
#endif
}

// Implementation of the interpolator.
void
SyncFrameAutomator::run_automator()
{
  cerr << "SyncFrameAutomator::run_automator(): start\n";
  for(;;) {
    if (syncd->frameReady()) {
      syncd->renderFrame();
      syncd->doneRendering();
    } else {
      // Wait for a little bit (10 microseconds)
      Time::waitFor(10. / 1e6);
    }
  }
  cerr << "SyncFrameAutomator::run_automator(): end\n";
}

