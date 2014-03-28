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

#ifndef Manta_UserInterface_SyncFrameAutomator_h
#define Manta_UserInterface_SyncFrameAutomator_h

#include <UserInterface/AutomatorUI.h>
#include <Engine/Display/SyncDisplay.h>

#include <string>
#include <vector>

namespace Manta {

  using std::string;
  using std::vector;

  class SyncFrameAutomator : public AutomatorUI {
  private:
    SyncDisplay* syncd;
  public:
    SyncFrameAutomator( MantaInterface* manta_interface_,
                        SyncDisplay* syncd);
    virtual ~SyncFrameAutomator();

    // Create method called by RTRT_register.
    static UserInterface* create( const vector<string>& args,
                                  MantaInterface* manta_interface_ );

    // Implementation of the interpolator.
    virtual void run_automator();

  };
} // end namespace Manta

#endif // ifndef Manta_UserInterface_SyncFrameAutomator_h
