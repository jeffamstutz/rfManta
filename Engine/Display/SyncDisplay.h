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

#ifndef Manta_Engine_SyncDisplay_h
#define Manta_Engine_SyncDisplay_h

#include <Interface/ImageDisplay.h>
#include <Core/Thread/Semaphore.h>

#include <string>
#include <vector>

/*
  The code that wants to render should have something that looks like this:

  for(;;) {
    if (syncd->frameReady()) {
      syncd->renderFrame();
      syncd->doneRendering();
    }
  }

  If you don't want to render the frame you must still call
  doneRendering to relsease the lock on the frame resources.
  
    if (syncd->frameReady()) {
      if (really_want_to_render)
        syncd->renderFrame();
      syncd->doneRendering();
    }
*/

namespace Manta {
  class SetupContext;
  class DisplayContext;
  class ImageDisplay;
  class Image;

  using namespace std;

  class SyncDisplay: public ImageDisplay {
  public:
    SyncDisplay(const vector<string>& args);
    virtual ~SyncDisplay();
    virtual void setupDisplayChannel(SetupContext&);
    virtual void displayImage(const DisplayContext& context,
			      const Image* image);

    void setChild(ImageDisplay* child_in);
    bool frameReady();
    void waitOnFrameReady();    // blocks until a frame is ready
    void renderFrame();
    void doneRendering();
    void abort(); // Frees a thread waiting on frameready.

    Image const* getCurrentImage() { return current_image; }
  private:
    SyncDisplay(const SyncDisplay&);
    SyncDisplay& operator=(const SyncDisplay&);

    // This is what will actually do the displaying
    ImageDisplay* child_display;
    // Whether this class created the child or not
    bool self_created_child;
    // Temporaries to be passed to the child's displayImage
    DisplayContext const* current_context;
    Image const* current_image;

    Semaphore _frameready;
    Semaphore _framedone;
  };
}

#endif
