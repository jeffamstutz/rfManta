#ifndef MANTA_UTIL_CPUTIME_H__
#define MANTA_UTIL_CPUTIME_H__

namespace Manta {

  // This uses the cycle counter of the processor.  Different
  // processors in the system will have different values for this.  If
  // you process moves across processors, then the delta time you
  // measure will likely be incorrect.  This is mostly for fine
  // grained measurements where the process is likely to be on the
  // same processor.  For more global things you should use the
  // Time interface.

  // Also note that if you processors' speeds change (i.e. processors
  // scaling) or if you are in a heterogenous environment, you will
  // likely get spurious results.
  class CPUTime {
  public:
    typedef unsigned long long SysClock;
	    
    //////////
    // Return the current CPU time, in terms of clock ticks.
    // Time zero is at some arbitrary point in the past.
    static SysClock currentTicks();
	    
    //////////
    // Return the current CPU time, in terms of seconds.
    // This is slower than currentTicks().  Time zero is at
    // some arbitrary point in the past.
    static double currentSeconds();
	    
    //////////
    // Return the conversion from seconds to ticks.
    static double ticksPerSecond();
	    
    //////////
    // Return the conversion from ticks to seconds.
    static double secondsPerTick();

    //////////
    // Requeries the processor speed to recompute the ticksPerSecond.
    static void resetScale();
  private:
    CPUTime();
    static void initialize();
  };
  
} // end namespace Manta

#endif // #ifndef MANTA_UTIL_CPUTIME_H__
