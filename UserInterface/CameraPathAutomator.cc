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

#include <UserInterface/CameraPathAutomator.h>

#include <Core/Thread/Thread.h>
#include <Core/Thread/Time.h>
#include <Core/Exceptions/ErrnoException.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Math/MinMax.h>

#include <Interface/Camera.h>
#include <Interface/FrameState.h>
#include <Core/Util/Callback.h>

#include <Core/Math/CatmullRomInterpolator.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>

#include <stdio.h>
#include <errno.h>

#include <fstream>

using namespace Manta;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SETUP SETUP SETUP SETUP SETUP SETUP SETUP SETUP SETUP SETUP SETUP SETUP SETU
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CameraPathAutomator::CameraPathAutomator( MantaInterface *manta_interface_,
                                          int channel_, int warmup_frames,
                                          Vector *eye_, Vector *lookat_,
                                          Vector *up_, int total_points_,
                                          Real delta_t_, Real delta_time_ )
  :
  AutomatorUI( manta_interface_, warmup_frames ),
  total_points( total_points_ ),
  delta_t( delta_t_ ),
  delta_time( delta_time_ ),
  channel( channel_ ),
  synchronize_barrier( "sync barrier" ),
  last_sync_seconds( 0 ),
  last_sync_frame( 0 ),
  sync_frames( 0 ),
  loop_behavior( PATH_STOP ),
  interval_start( 1 ),
  interval_last ( total_points-2 ),
  average_fps( 0 )
{
  eye    = new Vector[total_points];
  lookat = new Vector[total_points];
  up     = new Vector[total_points];
  hfov   = new float[total_points];
  vfov   = new float[total_points];

  for(int i = 0; i < total_points; ++i) {
    eye[i]    = Vector(eye_[i]);
    lookat[i] = Vector(lookat_[i]);
    up[i]     = up_[i];
    hfov[i] = vfov[i] = 60; //some arbitrary default
  }
}

CameraPathAutomator::CameraPathAutomator( MantaInterface *manta_interface_, int channel_,
                                          int warmup_, CameraPathDataVector &camera_data,
                                          Real delta_t_, Real delta_time_ )
  :
  AutomatorUI( manta_interface_, warmup_ ),
  total_points( camera_data.size() ),
  delta_t( delta_t_ ),
  delta_time( delta_time_ ),
  channel( channel_ ),
  synchronize_barrier( "sync barrier" ),
  last_sync_seconds( 0 ),
  last_sync_frame( 0 ),
  sync_frames( 0 ),
  loop_behavior( PATH_STOP ),
  interval_start( 1 ),
  interval_last ( camera_data.size()-2 ),
  average_fps( 0 )

{
  eye    = new Vector[total_points];
  lookat = new Vector[total_points];
  up     = new Vector[total_points];
  hfov   = new float[total_points];
  vfov   = new float[total_points];

  for(int i = 0; i < total_points; ++i) {
    eye[i]    = camera_data[i].eye;
    lookat[i] = camera_data[i].lookat;
    up[i]     = camera_data[i].up;
    hfov[i]   = camera_data[i].hfov;
    vfov[i]   = camera_data[i].vfov;
  }

  // Reserve storage for the performance statistics results.
  performance.reserve( 1024 );
}




CameraPathAutomator::CameraPathAutomator( MantaInterface *manta_interface_, int channel_, 
                                          int warmup_frames, const string &file_name,
                                          Real delta_t_, Real delta_time_ ) :
  AutomatorUI( manta_interface_, warmup_frames ),
  delta_t( delta_t_ ),
  delta_time( delta_time_ ),
  channel( channel_ ),
  synchronize_barrier( "sync barrier" ),
  last_sync_seconds( 0 ),
  last_sync_frame( 0 ),
  loop_behavior( PATH_STOP ), 
  interval_start( 1 ),
  average_fps( 0 )
{

  total_points = 0;
  
  vector<string> args;
  string name;
  
  /////////////////////////////////////////////////////////////////////////////
  // Load the input file.
  std::string line;

  std::ifstream file(file_name.c_str());
  if(!file){
    throw  ErrnoException( "Cannot open camera path file: " + file_name, 
                          errno, __FILE__, __LINE__ );
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Count input.
  while (true){
    // Read in one line at a time. 
    getline(file, line);
    if(file.eof())
      break;
    
    // Parse a manta specification.
    parseSpec( line, name, args );
    
    // Count the number of control points.
    if (name == "control") {
      total_points++;
    }
  }

  // Check to make sure we got enough control points
  if (total_points < 4) {
    throw  InternalError( "Didn't find at least 4 control points in the file: " + file_name );
  }

  // Clear the EOF bit and rewind the input file.
  file.clear();
  file.seekg(0);

  // Allocate storage for points.
  eye    = new Vector[total_points];
  lookat = new Vector[total_points];
  up     = new Vector[total_points];
  hfov   = new float[total_points];
  vfov   = new float[total_points];

  /////////////////////////////////////////////////////////////////////////////
  // Copy out input.
  int line_num = 0;
  int control_point = 0;
  char error_message[64];
  
  while(true){
    // Parse a new set of args.
    args.clear();

    getline(file, line);
    if(file.eof())
      break;

    parseSpec( line, name, args );
    
    ///////////////////////////////////////////////////////////////////////////
    // Parse the args.
    
    // Line should be in the form "delta_t( 0.25 )"
    if (name == "delta_t") {
      if (args.size() == 0 || !parseValue(args[0],delta_t)) {
        sprintf(error_message, "CameraPathAutomator input line: %d", line_num );
        throw  IllegalArgument(error_message, 0, args);
      }
    }
    else if (name == "delta_time") {
      if (args.size() == 0 || !parseValue(args[0],delta_time)) {
        sprintf(error_message, "CameraPathAutomator input line: %d", line_num );
        throw  IllegalArgument(error_message, 0, args);
      }
    }
    else if (name == "control") {
    
      /////////////////////////////////////////////////////////////////////////
      // Parse eye, lookat and up from the args.
      Vector eye_, lookat_, up_;
      Real hfov_ = 60;
      Real vfov_ = 60;
      
      bool got_eye = false;
      bool got_lookat = false; 
      bool got_up = false;
      
      for (size_t i=0;i<args.size();++i) {
        if (args[i] == "-eye") {
          if (!getVectorArg(i,args,eye_)) {
            sprintf(error_message, "CameraPathAutomator -eye input line: %d", line_num );
            throw  IllegalArgument(error_message, i, args);
          }
          got_eye = true;
        }
        else if (args[i] == "-lookat") {
          if (!getVectorArg(i,args,lookat_)) {
            sprintf(error_message, "CameraPathAutomator -lookat input line: %d", line_num );
            throw  IllegalArgument(error_message, i, args);
          }
          got_lookat = true;
        }
        else if (args[i] == "-up") {
          if (!getVectorArg(i,args,up_)) {
            sprintf(error_message, "CameraPathAutomator -up input line: %d", line_num );
            throw  IllegalArgument(error_message, i, args);
          }
          got_up = true;
        }
        else if (args[i] == "-hfov") {
          if (!getArg(i,args,hfov_)) {
            sprintf(error_message, "CameraPathAutomator -hfov input line: %d", line_num );
            throw  IllegalArgument(error_message, i, args);
          }
        }
        else if (args[i] == "-vfov") {
          if (!getArg(i,args,vfov_)) {
            sprintf(error_message, "CameraPathAutomator -vfov input line: %d", line_num );
            throw  IllegalArgument(error_message, i, args);
          }
        }
      }
      
      /////////////////////////////////////////////////////////////////////////
      // Add the control point.
      if (got_eye && got_lookat && got_up) {
        eye   [control_point] = eye_;
        lookat[control_point] = lookat_;
        up    [control_point] = up_;
        hfov  [control_point] = hfov_;
        vfov  [control_point] = vfov_;
        ++control_point;
      }
      else {
        sprintf(error_message, "CameraPathAutomator incomplete spec input line: %d", line_num );
        throw  IllegalArgument(error_message, line_num, args);
      }
    }
    
    ++line_num;
  }

  // Set the end point
  interval_last = total_points-2;
  
  /////////////////////////////////////////////////////////////////////////////

  std::cerr << "CameraPathAutomator: " << total_points
            << " control points from " << file_name << std::endl;
  std::cerr << " Points in path: " << (total_points-3) << std::endl;
  std::cerr << " delta_t: " << delta_t 
            << " delta_time: " << delta_time << std::endl;
  std::cerr << " Total time for issue: " << ((1.0/delta_t)*(total_points-3.0)*delta_time) << " seconds" << std::endl;
}

CameraPathAutomator::~CameraPathAutomator() {

  delete [] eye;
  delete [] lookat;
  delete [] up;
  delete [] hfov;
  delete [] vfov;
}

UserInterface *CameraPathAutomator::create( const vector<string> &args, MantaInterface *manta_interface_ ) {
  /////////////////////////////////////////////////////////////////////////////
  // Parse args.
  string file_name;
  int warmup_frames = 2;
  int channel = 0;
  int sync_frames = 0;
  int behavior = PATH_STOP;
  int start = 0, last = 0;
  bool sync_quiet = false;
  
  Real delta_t    = 0;
  Real delta_time = 0;

  for (size_t i=0; i < args.size(); ++i) {
    if (args[i] == "-file") {
      file_name = args[++i];
    }
    else if (args[i] == "-warmup") {
      getArg(i, args, warmup_frames );
    }
    else if (args[i] == "-channel") {
      getArg(i, args, channel );
    }
    else if (args[i] == "-sync") {
      getArg(i, args, sync_frames );
    }
    else if (args[i] == "-delta_t") {
      getArg(i, args, delta_t );
    }
    else if (args[i] == "-delta_time") {
      getArg(i, args, delta_time );
    }
    else if (args[i] == "-behavior") {
      ++i;
      if (args[i] == "exit") {
        behavior = PATH_EXIT;
      }
      else if (args[i] == "loop") {
        behavior = PATH_LOOP;
      }
    }
    else if (args[i] == "-interval") {
      getArg(i, args, start );
      getArg(i, args, last  );
    }
    else if (args[i] == "-quiet") {
      sync_quiet = true;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Create the Automator.
  CameraPathAutomator *automator = new CameraPathAutomator( manta_interface_, channel, warmup_frames, file_name );

  automator->set_sync_frames  ( sync_frames );
  automator->set_loop_behavior( behavior );
  automator->set_sync_quiet   ( sync_quiet );
  
  // Check to see if either delta was specified on the command line.
  if (delta_t != 0) {
    automator->set_delta_t( delta_t );
  }
  if (delta_time != 0) {
    automator->set_delta_time( delta_time );
  }

  // Check to see if an interval was specified.
  if (start != 0 || last != 0) {
    automator->set_interval( start, last );
  }

  return automator;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  RUN  R
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Implementation of the interpolator.
void CameraPathAutomator::run_automator() {

  Vector current_eye, current_lookat, current_up;
  float current_hfov, current_vfov;

  int current_point = 0;

  int last_point  = Min(total_points-2,interval_last);
  int first_point = Max(1,Min(interval_start,last_point));
  
  int start_frame = getMantaInterface()->getFrameState().frameSerialNumber;
  start_seconds = Time::currentSeconds();
  
  std::cerr << "Beginning camera path " << total_points 
            << " control points. Using interval " << first_point << ":" << last_point << std::endl;

  // Clear performance statistics.
  performance.clear();
  
  int transaction_number = 0;
  
  do {
    
    //////////////////////////////////////////////////////////////////////////
    // Main Automator loop.
    for (current_point=first_point;
         (current_point<last_point) && (loop_behavior!=PATH_ABORT);
         ++current_point) {

      ////////////////////////////////////////////////////////////////////////
      // Sample by delta_t between the points.
      for (Real t=0; t<=(1.0-delta_t) && loop_behavior!=PATH_ABORT; t+=delta_t) {

        // Evaluate the spline.
        // NOTE: operator & is overloaded by Vector to return (Real *)

        catmull_rom_interpolate( &eye[current_point],    t, current_eye);
        catmull_rom_interpolate( &lookat[current_point], t, current_lookat);
        catmull_rom_interpolate( &up[current_point],     t, current_up );

        //We really can't have negative fov, and it's possible to get
        //it with catmull rom (it can overshoot), so to be safe we
        //linearly interpolate
        current_hfov = (1-t)*hfov[current_point] + t*hfov[current_point+1];
        current_vfov = (1-t)*vfov[current_point] + t*vfov[current_point+1];

        // Record the time of this transaction before a potential sync
        double start_time = Time::currentSeconds();
        
        // Send the transaction to manta.
        getMantaInterface()->addTransaction
          ("CameraPathAutomator",
           Callback::create(this, &CameraPathAutomator::mantaSetCamera,
                            current_eye, current_lookat, current_up,
                            current_hfov, current_vfov) );


        // Check to see if this is a synchronization point.
        if (sync_frames && ((transaction_number % sync_frames) == 0)) {

          // Add a synchronization transaction.
          getMantaInterface()->addTransaction
            ("CameraPathAutomator-Sync",
             Callback::create(this, &CameraPathAutomator::mantaSynchronize,
                              transaction_number ), TransactionBase::CONTINUE );

          // Wait for the render thread.
          synchronize_barrier.wait( 2 );
        }

        transaction_number++;

        //////////////////////////////////////////////////////////////////////
        // Wait for delta_time seconds.
        while ((Time::currentSeconds()-start_time) < delta_time);
      }
    }

    // Compute the average fps for this run.
    int total_frames = getMantaInterface()->getFrameState().frameSerialNumber - start_frame;
    average_fps = (Real)total_frames/(Time::currentSeconds()-start_seconds);

    std::cerr << "Path complete. "    << total_frames
              << " frames. Avg fps: " << average_fps << std::endl;
  }

  // Check to see if we should continue looping.
  while (loop_behavior == PATH_LOOP);

  // Check to see if we should exit the renderer.
  if (loop_behavior == PATH_EXIT) {
    Thread::exitAll( 0 );
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// TRANSACTION CALLBACK  TRANSACTION CALLBACK  TRANSACTION CALLBACK  TRANSACTIO
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CameraPathAutomator::mantaSetCamera( const Vector eye_,
                                          const Vector lookat_,
                                          const Vector up_,
                                          const float hfov_,
                                          const float vfov_) {

  // Reset the current camera.
  getMantaInterface()->getCamera( channel )->reset( eye_, up_, lookat_, hfov_, vfov_);
}



void CameraPathAutomator::mantaSynchronize( int issue_transaction ) {

  // Wait for either the renderer thread or the camera path thread.
  synchronize_barrier.wait( 2 );

  double current_sync_seconds = Time::currentSeconds();
  int  current_sync_frame   = getMantaInterface()->getFrameState().frameSerialNumber;

  double total_elapse_seconds = current_sync_seconds - start_seconds;
  
  double elapse_seconds = current_sync_seconds - last_sync_seconds;
  int  elapse_frame   = current_sync_frame   - last_sync_frame;

  last_sync_seconds = current_sync_seconds;
  last_sync_frame   = current_sync_frame;

  if (sync_quiet) { 

    // Store the statistics.
    performance.push_back( elapse_frame/elapse_seconds );
  }
  else {
    // Output performance since last sync.
    std::cout << current_sync_frame << " "
              << issue_transaction << " "
              << elapse_frame << " "
              << elapse_seconds << " "
              << elapse_frame/elapse_seconds << " "
              << total_elapse_seconds
              << std::endl;
    // stdout flush is necessary so that output occurs before program terminates.
  }
  
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// WRITE PATH  WRITE PATH  WRITE PATH  WRITE PATH  WRITE PATH  WRITE PATH  WRIT
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CameraPathAutomator::write_path( const string &file_name ) {

  FILE *file = fopen( file_name.c_str(), "w" );
  if (file == 0) {
    throw  ErrnoException( "Cannot open camera path file: " + file_name, 
                          errno, __FILE__, __LINE__ );
  }

  // Output the delta t and delta time.
  fprintf( file, "delta_t(%f)\n", delta_t );
  fprintf( file, "delta_time(%f)\n", delta_time );

  // Output the control points.
  for (int i=0; i<total_points; ++i) {
    fprintf( file, "control( -eye %f %f %f ", eye[i][0],    eye[i][1],    eye[i][2] );
    fprintf( file,       "-lookat %f %f %f ", lookat[i][0], lookat[i][1], lookat[i][2] );
    fprintf( file,           "-up %f %f %f ", up[i][0],     up[i][1],     up[i][2] );
    fprintf(file, "-hfov %f -vfov %f", hfov[i], vfov[i]);
    fprintf( file, ")\n");
  }

  // Terminate with newline.
  fprintf( file, "\n" );

  fclose( file );
}
