#ifndef Manta_Interface_Interpolable_h
#define Manta_Interface_Interpolable_h

#include <Interface/Clonable.h>
#include <vector>

namespace Manta {
  class Interpolable; // forward declaration for the keframe class.
  
  struct Interpolable_keyframe_t {
    Interpolable* keyframe;
    float t;
  };

  class Interpolable : public Clonable {
  public:
    virtual ~Interpolable();
    enum InterpErr{success, 
                   notInterpolable}; //cannot interpolate. No interpolation performed.

    // Swig doesn't like nested classes, so I pulled it out.  I'm
    // defining a typedef here, so that I don't have to change all the
    // places where it's used for now.
    typedef Interpolable_keyframe_t keyframe_t;
    
    //Assuming the following operators existed, this function is
    //supposed to do something like the following: 
    //       *this = keyframe[0]*t[0] + ... + keyframe[n]*t[n];
    //Make sure to check types of keyframes match
    virtual InterpErr serialInterpolate(const std::vector<keyframe_t> &keyframes)
    {
      return notInterpolable;
    }

    virtual InterpErr parallelInterpolate(const std::vector<keyframe_t> &keyframes,
                                          int proc, int numProc)
    {
      //don't call serialInterpolate by default, since we don't want
      //to hide from user that they are doing something stupid.
      //either throw an error or do nothing.
      return notInterpolable;
    }

    virtual bool isParallel() const { return false; }

  };
}
#endif //Manta_Interface_Interpolable_h
