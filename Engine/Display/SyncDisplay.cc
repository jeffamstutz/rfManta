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

#include <Engine/Display/SyncDisplay.h>
#include <Interface/ImageDisplay.h>
#include <Interface/Context.h>
#include <Core/Thread/Semaphore.h>
#include <iostream>
#include <Core/Thread/Mutex.h>

using namespace Manta;
using namespace std;
Mutex sync_lock("SyncDisplay io lock");

SyncDisplay::SyncDisplay(const vector<string>& args)
  : child_display(NULL),
    self_created_child(false),
    current_context(NULL),
    current_image(NULL),
    _frameready("SyncDisplay::_frameready", 1),
    _framedone("SyncDisplay::_framedone", 1)
{
  _frameready.down();
  _framedone.down();

  // Possibly add a factory call to make child_display from args.
}

SyncDisplay::~SyncDisplay()
{
  if (self_created_child) {
    delete child_display;
  }
}

void
SyncDisplay::setupDisplayChannel(SetupContext& context)
{
  if (child_display) child_display->setupDisplayChannel(context);
}

void
SyncDisplay::displayImage(const DisplayContext& context,
                          const Image* image)
{
  if(context.proc != 0)
    return;
  current_context = &(context);
  current_image = image;
  //  sync_lock.lock(); cerr << "di::_frameready.up() .. \n"; sync_lock.unlock();
  _frameready.up();
  //  sync_lock.lock(); cerr << "di::_frameready.up() .. done\n"; sync_lock.unlock();
  //  sync_lock.lock(); cerr << "di::_framedone.down() .. \n"; sync_lock.unlock();
  _framedone.down();
  //  sync_lock.lock(); cerr << "di::_framedone.down() .. done\n"; sync_lock.unlock();
}

void
SyncDisplay::setChild(ImageDisplay* child_in)
{
  if (self_created_child) {
    // destroy the old child
    delete child_display;
    self_created_child = false;
  }
  child_display = child_in;
}

bool
SyncDisplay::frameReady()
{
  return _frameready.tryDown();
}

void
SyncDisplay::waitOnFrameReady()
{
  _frameready.down();
}

void
SyncDisplay::renderFrame()
{
  // _frameready should already be locked at this point by the
  // frameReady call.
  child_display->displayImage(*current_context, current_image);
}

void
SyncDisplay::doneRendering() {
  //  sync_lock.lock(); cerr << "dr::_framedone.up() .. \n"; sync_lock.unlock();
  _framedone.up();
  //  sync_lock.lock(); cerr << "dr::_framedone.up() .. done\n"; sync_lock.unlock();
}

void SyncDisplay::abort() {
  _frameready.up();
}
