
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

#include <Engine/Display/FileDisplay.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Util/Args.h>
#include <Image/NullImage.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>
#include <Image/TGAFile.h>
#include <Image/NRRDFile.h>
#include <Interface/Context.h>
#include <Core/Thread/Time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace Manta;
using namespace std;

ImageDisplay *FileDisplay::create(
  const vector< string > &args )
{
  return new FileDisplay(args);
}

FileDisplay::FileDisplay(const vector<string> &args ) :
  display_fps( false ),
  current_frame( 0 ),
  file_number( 0 ),
  skip_frames( 0 ),
  prefix( "manta_frame" ),
  doFrameCount( true ),
  type_extension( "png" ),
  init_time( Time::currentSeconds() ),
  use_timestamp( true )
{

  /////////////////////////////////////////////////////////////////////////////
  // Parse args.
  for (size_t i=0;i<args.size();++i) {
    if (args[i] == "-prefix") {
      if (!getArg( i, args, prefix )) {
        throw IllegalArgument( "FileDisplay", i, args );
      }
    }
    else if(args[i] == "-noframecount") {
      doFrameCount = false;
    }
    else if (args[i] == "-type") {
      if (!getStringArg( i, args, type_extension )) {
        throw IllegalArgument( "FileDisplay", i, args );
      }
    }
    else if (args[i] == "-offset") {
      if (!getArg( i, args, file_number )) {
        throw IllegalArgument( "FileDisplay", i, args );
      }
    }
    else if (args[i] == "-skip") {
      if (!getArg( i, args, skip_frames )) {
        throw IllegalArgument( "FileDisplay", i, args );
      }
    }
    else if (args[i] == "-fps") {
      display_fps = true;
    }
    else if (args[i] == "-notimestamp") {
      use_timestamp = false;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Determine which writer to use.
  if (type_extension == "tga") {
    writer = TGA_WRITER;
  }
  else {
    writer = NRRD_WRITER;
  }
}

FileDisplay::FileDisplay(const string &prefix_, const string &type_, int offset_, int skip_, bool use_timestamp_, bool fps_ ) :
  display_fps( fps_ ),
  current_frame( 0 ),
  file_number( offset_ ),
  skip_frames( skip_ ),
  prefix( prefix_ ),
  doFrameCount( true ),
  type_extension( type_ ),
  init_time( Time::currentSeconds() ),
  use_timestamp( use_timestamp_ )

{

  // Determine which writer to use.
  if (type_extension == "tga") {
    writer = TGA_WRITER;
  }
  else {
    writer = NRRD_WRITER;
  }
}

FileDisplay::~FileDisplay() {
}

void FileDisplay::setupDisplayChannel( SetupContext & ) {
}

void FileDisplay::displayImage(
  const DisplayContext &context,
  const Image *image )
{

  // Make sure processor 0 writes.
  if(context.proc != 0)
    return;

  // Check to see if we should skip the frame.
  if (!skip_frames || ((current_frame % skip_frames) == 0)) {

    // Record the timestamp of this file.
    unsigned number;
    unsigned width = 10;
    if (use_timestamp) {
      const float delta_time = (Time::currentSeconds() - init_time);
      number = (unsigned)(delta_time*1000.0f);
    }
    else {
      number = file_number;
    }

    // Record time to output file.
    double start_time = 0;
    if (display_fps) start_time = Time::currentSeconds();

    // Determine resolution.
    bool stereo;
    int xres, yres;
    image->getResolution( stereo, xres, yres );

    // Check for stereo.
    if (stereo) {

      stringstream lss, rss;

      // Output left and right images.
      if(doFrameCount){
        lss << prefix << "_left_" << setfill( '0' ) << setw( width ) << number << "." << type_extension;
        rss << prefix << "_right_" << setfill( '0' ) << setw( width ) << number << "." << type_extension;
      }
      else{
        lss << prefix << "_left_." << type_extension;
        lss << prefix << "_right_." << type_extension;
      }

      // Send the image to the appropriate writer.
      if (writer == TGA_WRITER) {
        writeTGA( image, lss.str(), 0 );
        writeTGA( image, rss.str(), 1 );
      }
      else {
        writeNRRD( image, lss.str(), 0 );
        writeNRRD( image, rss.str(), 1 );
      }
    } else {

      // Otherwise output a single image.
      stringstream ss;
      if(doFrameCount){
        ss << prefix << "_"
           << setfill( '0' )
           << setw( width )
           << number
           << "." << type_extension;
      }
      else{
        ss << prefix << "." << type_extension;
      }

      if (writer == TGA_WRITER) {
        writeTGA( image, ss.str(), 0 );
      }
      else {
        writeNRRD( image, ss.str(), 0 );
      }

      // Increment output file counter.
      ++file_number;
    }

    if (display_fps) {
      double end_time = Time::currentSeconds();
      std::cerr << "FileDisplay fps: " << 1.0/(end_time-start_time) << "\n";
    }
  }

  // Increment frame counter.
  current_frame++;
}
