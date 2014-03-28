
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

#include <Core/Util/Args.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/OutputError.h>
#include <Core/Persistent/Archive.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Engine/Factory/Factory.h>
#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/Scene.h>
#include <Core/Thread/Thread.h>
#include <Core/Util/Assert.h>
#include <Model/Textures/CheckerTexture.h>

#include <Interface/Background.h>
#include <Interface/LightSet.h>

#include <iostream>
#include <string>

using namespace std;
using namespace Manta;

void usage()
{
  cerr << "usage: savescene scenefile outfile\n";
  exit(1);
}

void rtrtPreprocess(MantaInterface* rtrt,
                    Scene* scene) {
  LightSet* lights = scene->getLights();

  PreprocessContext context(rtrt, 0, 1, lights);
  lights->preprocess(context);
  scene->getBackground()->preprocess(context);

  //cerr << "About to preprocess the scene (proc " << proc << ")...\n";
  scene->getObject()->preprocess(context);
}

int
main(int argc, char* argv[])
{
  // Copy args into a vector<string>
  vector<string> args;
  for(int i=1;i<argc;i++)
    args.push_back(argv[i]);

  try {

    ///////////////////////////////////////////////////////////////////////////
    // Create Manta.
    MantaInterface* rtrt = createManta();

    // Create a Manta Factory.
    Factory* factory = new Factory( rtrt );

    // Set the scene search path based on an env variable.
    if(getenv("MANTA_SCENEPATH"))
      factory->setScenePath(getenv("MANTA_SCENEPATH"));
    else
      factory->setScenePath("");
    if(args.size() != 2)
      usage();
    size_t i=0;
    i--;
    string scenename;
    if(!getStringArg(i, args, scenename))
      usage();
    if(!factory->readScene(scenename)){
      cerr << "Error reading scene: " << scenename << '\n';
      throw IllegalArgument( "-scene", i, args );
    }
    string outfile;
    if(!getStringArg(i, args, outfile))
      usage();

    Archive* archive = Archive::openForWriting(outfile);
    if(!archive)
      throw OutputError("Could not open Archive for writing: " + outfile);
    Scene* scene = rtrt->getScene();
    rtrtPreprocess(rtrt, scene);
    ArchiveElement* root = archive->getRoot();
    root->readwrite("scene", scene);
    delete archive;
    delete rtrt;

  } catch (Exception& e) {
    cerr << "savescene.cc (top level): Caught exception: " << e.message() << '\n';
    Thread::exitAll( 1 );

  } catch (std::exception e){
    cerr << "savescene.cc (top level): Caught std exception: "
         << e.what()
         << '\n';
    Thread::exitAll(1);

  } catch(...){
    cerr << "savescene.cc (top level): Caught unknown exception\n";
    Thread::exitAll(1);
  }

  cerr << "Temporary measure to force instantiation of checker texture\n";
  new CheckerTexture<Color>();

  Thread::exitAll(0);
}
