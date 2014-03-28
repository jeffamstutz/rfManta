
#ifndef Manta_Interface_CameraPath_h
#define Manta_Interface_CameraPath_h

#include <Core/Math/Spline.h>
#include <Interface/Camera.h>

#include <string>
#include <vector>

namespace Manta
{
  class BasicCameraData;
  class CallbackHandle;
  class MantaInterface;

  class CameraPath
  {
  public:
    typedef enum { ReadKnots, WriteKnots } IOMode;
    typedef enum { Count, CountAndQuit, Infinite } LoopMode;

    CameraPath(MantaInterface* interface, string const& fname,
               IOMode mode=WriteKnots, LoopMode loop=Infinite,
               int max_count=0, int offset=0);
    ~CameraPath(void);

    void interpolate(int, int, bool&);
    void animate(void);
    void reset(void) { frame=0; }

    void addKnot(unsigned int const frame, BasicCameraData const& data);
    void readKnots(void);
    void writeKnots(void) const;

  private:
    MantaInterface* interface;
    std::string fname;
    LoopMode loop;
    int max_count;
    int count;
    int offset;
    CallbackHandle* handle;
    bool anim;
    unsigned int frame;
    unsigned int max_frame;

    // You have to keep track of these separate from the splines,
    // since you can't get access to the knots in the spline directly.
    // Perhaps this should change....
    struct Knot
    {
      unsigned int frame;
      BasicCameraData data;
    };
    std::vector<Knot> knots;

    Spline<Vector> eyes;
    Spline<Vector> lookats;
    Spline<Vector> ups;
    Spline<Real> hfovs;
    Spline<Real> vfovs;
  };
}

#endif // Manta_Interface_CameraPath_h
