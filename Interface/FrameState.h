
#ifndef Manta_Interface_FrameState_h
#define Manta_Interface_FrameState_h

namespace Manta {
  struct FrameState {
    long frameSerialNumber; // Incremented every single frame
    long animationFrameNumber; // Only incremented when animation is running
    double frameTime;
    Real shutter_open;
    Real shutter_close;
  };
}

#endif
