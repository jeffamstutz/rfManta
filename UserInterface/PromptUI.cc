
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

#include <UserInterface/PromptUI.h>
#include <UserInterface/AutomatorUI.h>
#include <Interface/MantaInterface.h>
#include <Interface/Camera.h>
#include <Interface/ShadowAlgorithm.h>
#include <Core/Util/Args.h>
#include <Core/Exceptions/Exception.h>
#include <Core/Thread/Runnable.h>
#include <Core/Thread/Thread.h>

#include <Engine/Factory/RegisterKnownComponents.h>

#include <vector>
#include <string>
#include <iostream>

using namespace Manta;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SETUP  SETUP  SETUP  SETUP  SETUP  SETUP  SETUP  SETUP  SETUP  SETUP  SETUP
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UserInterface* PromptUI::create(const vector<string>& args,
				MantaInterface *manta_interface_) {

  return new PromptUI( args, manta_interface_ );
}

PromptUI::PromptUI(const vector<string>& args, MantaInterface *manta_interface_) :
  manta_interface( manta_interface_ ),
  factory( manta_interface_ ),
  current_channel( 0 )
{
  registerKnownComponents( &factory );
}

PromptUI::~PromptUI() {
}

void PromptUI::startup() {
  (new Thread(this, "Manta User Interface"))->detach();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  R
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PromptUI::run() {

  char buffer[256];
  
  while (1) {

    // Read in a line from stdin.
    cin.getline( buffer, 255 );

    try {
      // Process the command.
      processCommand( buffer );
    }
    catch (Exception &e) {
      cerr << e.message() << std::endl;
    }
    catch (Exception *e) {
      cerr << e->message() << std::endl;
    }
  }
}

void PromptUI::processCommand( const string &input_line ) {

  // Each line of input should contain:
  // -option value"( -args ... )"
  // Everything between the value( and ) will be passed to the manta_interface
  // as a string. Note the quotes are unnecessary

  // cerr << "Input line: " << input_line << endl;

  /////////////////////////////////////////////////////////////////////////////
  // Find the option string.
  string::const_iterator iter = input_line.begin();

  // Skip leading whitespace.
  while ((iter != input_line.end()) &&
         (((*iter) == ' ') ||
          ((*iter) == '\t')))
    ++iter;

  // Make sure the first character is a '-'
  if (*iter != '-') {
    return;
  }

  string::const_iterator begin = iter;  
  
  // Advance to white space.
  while ((iter != input_line.end()) &&
         !(((*iter) == ' ') ||
           ((*iter) == '\t')))
    ++iter;

  // Found the option string.
  string option_string( begin, iter );

  /////////////////////////////////////////////////////////////////////////////
  // Find the value string.

  // Skip white space
  while ((iter != input_line.end()) &&
         (((*iter) == ' ') ||
          ((*iter) == '\t')))
    ++iter;

  begin = iter;

  // Find either a space or the end of the line.
  while ((iter != input_line.end()) &&
         !(((*iter) == ' ') ||
           ((*iter) == '(')))
    ++iter;

  // Check to see if args are specified.
  if (*iter == '(') {

    while ((iter != input_line.end()) &&
         !((*iter) == ')'))
    ++iter;

    // Advance one past the closing ')'
    if (iter != input_line.end())
      ++iter;
  }

  string value_string( begin, iter );

  // cerr << "Parsed directive: option: " << option_string << " value: " << value_string << std::endl;
  
  /////////////////////////////////////////////////////////////////////////////
  // Match the option string to a known option.

  try {
  
    if (option_string == "-help") {
      printHelp();
    }
    else if (option_string == "-bench") {
    }
    else if (option_string == "-camera") {
      manta_interface->addTransaction( "select camera",
                                       Callback::create( this, &PromptUI::mantaCamera, value_string ));
    }
    else if (option_string == "-idlemode") {
    }
    else if (option_string == "-imagetraverser") {
      manta_interface->addTransaction( "select imagetraverser",
                                       Callback::create( this, &PromptUI::mantaImageTraverser, value_string ));
    }
    else if (option_string == "-loadbalancer") {
    }
    else if (option_string == "-np") {
    }  
    else if (option_string == "-pixelsampler") {
    }  
    else if (option_string == "-renderer") {
    }
    else if (option_string == "-scene") {
    }  
    else if (option_string == "-shadows") {
    }  
    else if (option_string == "-ui") {
      UserInterface *ui = factory.createUserInterface( value_string );

      // Make sure a ui was created.
      if (ui == 0) {
        return;
      }
    
      // Check to see if the ui is a Automator.
      AutomatorUI *automator = dynamic_cast<AutomatorUI *>( ui );
      if (automator != 0) {

        // Set the termination callback so we know when the automator finishes.
        automator->set_terminate_callback( Callback::create( this, &PromptUI::automatorComplete ));
      }

      // Start the ui.
      ui->startup();
    }
    else if (option_string == "-quit") {

      Thread::exitAll( 0 );
    }

  } catch (Exception *e) {
    cerr << "Caught exception: " << e->message() << std::endl << std::endl;
  } catch (Exception& e) {
    cerr << "Caught exception: " << e.message() << '\n';
    if(e.stackTrace())
      cerr << "Stack trace: " << e.stackTrace() << '\n';
  } catch (std::exception e){
    cerr << "Caught std exception: " << e.what() << '\n';
		
  } catch(...){
    cerr << "Caught unknown exception\n";
  }
  
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// TRANSACTION CALLBACKS  TRANSACTION CALLBACKS  TRANSACTION CALLBACKS  TRANSAC
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PromptUI::mantaCamera( string text ) {

  // Create the camera.
 	Camera *new_camera = factory.createCamera( text );
	if (new_camera == 0) {
		std::cerr << "Could not select camera " << text << std::endl;
		return;
	}

  // Get the old camera.
  Camera *old_camera = manta_interface->getCamera( current_channel );
	
	// Replace the camera.
	manta_interface->setCamera( current_channel, new_camera );
	
	delete old_camera;  
}

void PromptUI::mantaImageTraverser( string text ) {

  factory.selectImageTraverser( text );
}

void PromptUI::automatorComplete() {

  std::cout << "PROMPT_UI_AUTOMATOR_COMPLETE" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// HELP  HELP  HELP  HELP  HELP  HELP  HELP  HELP  HELP  HELP  HELP  HELP  HELP
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PromptUI::printHelp() {

  cerr << "Usage: manta [options]\n";
  cerr << "Valid options are:\n";
  cerr << " -help           - Print this message and exit\n";
  cerr << " -bench [N [M]]  - Time N frames after an M frame warmup period and print out the framerate,\n";
  cerr << "                   default N=100, M=10\n";
  cerr << " -np N           - Use N processors\n";
  cerr << " -res NxM        - Use N by M pixels for rendering (needs the x).\n";
  cerr << " -imagedisplay S - Use image display mode named S, valid modes are:\n";
  printList(cerr, factory.listImageDisplays(), 4);
  cerr << " -imagetype S    - Use image display mode named S, valid modes are:\n";
  printList(cerr, factory.listImageTypes(), 4);
  cerr << " -ui S           - Use the user interface S, valid options are:\n";
  printList(cerr, factory.listUserInterfaces(), 4);
  cerr << " -shadows S      - Use S mode for rendering shadows, valid modes are:\n";
  printList(cerr, factory.listShadowAlgorithms(), 4);
  cerr << " -imagetraverser S - Use S method for image traversing, valid modes are:\n";
  printList(cerr, factory.listImageTraversers(), 4);
  cerr << " -pixelsampler S - Use S method for pixel sampling, valid modes are:\n";
  printList(cerr, factory.listPixelSamplers(), 4);
  cerr << " -camera S       - User camera model S, valid cameras are:\n";
  printList(cerr, factory.listCameras(), 4);
  cerr << " -renderer S     - Use renderer S, valid renderers are:\n";
  printList(cerr, factory.listRenderers(), 2);
  cerr << " -scene S        - Render Scene S\n";
}

// This method prints a list of options.
void PromptUI::printList( ostream& out, const Factory::listType& list, int spaces ) {
  
  for(int i=0;i<spaces;i++)
    out << ' ';
  
  for(Factory::listType::const_iterator iter = list.begin();
      iter != list.end(); ++iter){
    if(iter != list.begin())
      out << ", ";
    out << *iter;
  }
  out << "\n";
}
