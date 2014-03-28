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

#ifndef RTRT_PromptUI_h
#define RTRT_PromptUI_h

#include <Interface/MantaInterface.h>
#include <Interface/UserInterface.h>
#include <Core/Thread/Runnable.h>

#include <Engine/Factory/Factory.h>

#include <sgi_stl_warnings_off.h>
#include <vector>
#include <string>
#include <sgi_stl_warnings_on.h>

namespace Manta {
  using namespace std;

  class MantaInterface;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////  
  // PromptUI is a userinterface which listens on STDIN for command line
  // directives and then changes manta's state. The directives match bin/manta
  // command line args.
  //
  // PromptUI is designed to be driven by a perl script for batch benchmarks.
  //
  // In general, manta outputs performance information on STDOUT and outputs
  // all other warnings and messages on STDERR. PromptUI will listen on STDIN
  // but by default won't output anything on STDOUT unless an error occurs.
  //
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////  
  class PromptUI: public UserInterface, public Runnable {
  protected:
    MantaInterface *manta_interface;
    Factory factory;

    // Channel that the ui is operating on.
    int current_channel;

    // Helper methods.
    void printHelp();
    void printList( ostream& out, const Factory::listType& list, int spaces=0 );

    void processCommand( const string &input_line );

    // Transaction callbacks
    void mantaCamera( string text );
    void mantaImageTraverser( string text );

    void automatorComplete();
    
  public:
    PromptUI(const vector<string>& args, MantaInterface *rtrt_int);
    virtual ~PromptUI();

    // UserInterface method.
    virtual void startup();

    // Runnable method.
    virtual void run();

    static UserInterface* create(const vector<string>& args,
                                 MantaInterface *manta_interface_ );
    
  private:
    PromptUI(const PromptUI&);
    PromptUI& operator=(const PromptUI&);
  };
}

#endif
