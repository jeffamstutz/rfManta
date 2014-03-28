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

#ifndef __CAMERAPATHAUTOMATOR_H__
#define __CAMERAPATHAUTOMATOR_H__

#include <Interface/Camera.h>
#include <UserInterface/AutomatorUI.h>

#include <MantaTypes.h>
#include <Core/Geometry/Vector.h>

#include <Core/Thread/Barrier.h>

#include <string>
#include <vector>

#ifdef SWIG
%template (vector_float) std::vector<float>;
%template (vector_BasicCameraData) std::vector<Manta::BasicCameraData>;
#endif

namespace Manta {

  using std::string;
  using std::vector;

  // std::vector wrapper class for swig.
  class CameraPathDataVector : public std::vector<BasicCameraData> {
    typedef std::vector<BasicCameraData> Parent;
  public:    
    CameraPathDataVector( ) { }
    CameraPathDataVector( unsigned size_ ) : Parent( size_ ) { };

    void PushBack( BasicCameraData data ) { push_back( data ); }
    void Clear() { clear(); }
  };



  class CameraPathPerformanceVector : public std::vector<float> {
    typedef std::vector<float> Parent;
  public:    
    CameraPathPerformanceVector( ) { }
    CameraPathPerformanceVector( unsigned size_ ) : Parent( size_ ) { };

    int __len__ () {
      return size();
    }
  };
  
  class CameraPathAutomator : public AutomatorUI {
  private:
    ///////////////////////////////////////////////////////////////////////////
    // Control points.
    Vector *eye;
    Vector *lookat;
    Vector *up;
    float *hfov, *vfov;
    int total_points;

    CameraPathPerformanceVector performance;
    
    ///////////////////////////////////////////////////////////////////////////
    // Parameter t interval between control points.
    Real delta_t;

    // Minimum time to wait between camera movements.
    Real delta_time;

    // Channel to apply camera changes to.
    int channel;

    ///////////////////////////////////////////////////////////////////////////
    // Synchronization barrier.
    Barrier synchronize_barrier;

    double last_sync_seconds;
    int  last_sync_frame;
    double start_seconds;

    // Number of frames between synchronization points.
    int sync_frames;

    // Should output occur during sync.
    bool sync_quiet;

    ///////////////////////////////////////////////////////////////////////////
    // Behavior
    int loop_behavior;
    int interval_start, interval_last;

    ///////////////////////////////////////////////////////////////////////////
    // Performance counters.

    // fps for the last complete run of this automator.
    Real average_fps;

  public:
    // Loop Behaviors
    enum { PATH_STOP, PATH_EXIT, PATH_LOOP, PATH_ABORT };
    

    CameraPathAutomator( MantaInterface *manta_interface_, int channel_,
                         int warmup_, const string &file_name,
                         Real delta_t_ = 0.2, Real delta_time_ = 0.2 );

    CameraPathAutomator( MantaInterface *manta_interface_, int channel_,
                         int warmup_, Vector *eye_, Vector *lookat_,
                         Vector *up_, int total_points_,
                         Real delta_t_ = 0.2, Real delta_time_ = 0.2 );
    CameraPathAutomator( MantaInterface *manta_interface_, int channel_,
                         int warmup_, CameraPathDataVector &camera_data,
                         Real delta_t_, Real delta_time_ );
    
    ~CameraPathAutomator();

    // Create method called by RTRT_register.
    static UserInterface *create( const vector<string> &args,
                                  MantaInterface *manta_interface_ );

    // Implementation of the interpolator.
    virtual void run_automator();

    // This method is called by the manta rendering thread to update
    // the camera.
    void mantaSetCamera( Vector eye_, Vector lookat_,
                         Vector up_, float hfov_, float vfov_ );

    // This method is called by the manta rendering thread to synchronize.
    void mantaSynchronize( int issue_transaction );

    // This may be called to write the camera path to a specified file
    // in the correct format.
    void write_path( const string &file_name );

    // Accessors.
    inline int get_total_points() { return total_points; };

    inline Real get_delta_t()                { return delta_t; };
    inline void set_delta_t( Real delta_t_ ) { delta_t = delta_t_; };

    inline Real get_delta_time()                   { return delta_time; };
    inline void set_delta_time( Real delta_time_ ) { delta_time = delta_time_; };

    inline int  get_sync_frames()                   { return sync_frames; };
    inline void set_sync_frames( int sync_frames_ ) { sync_frames = sync_frames_; };

    inline void set_loop_behavior( int behavior_ )    { loop_behavior = behavior_; };
    inline void set_interval( int start, int last )   { interval_start = start; interval_last = last; };
    inline void get_interval( int &start, int &last ) { start = interval_start; last = interval_last; };

    inline void set_sync_quiet( bool sync_quiet_ ) { sync_quiet = sync_quiet_; }
    
    // Get the fps for the last complete run.
    inline Real get_average_fps() { return average_fps; };

    inline CameraPathPerformanceVector get_performance() { return performance; }

    // Access path control points.
    inline BasicCameraData GetControlPoint( const unsigned index ) {
      return BasicCameraData( eye[index], lookat[index], up[index],
                              hfov[index], vfov[index] );
    }

    
  };
};

#endif
