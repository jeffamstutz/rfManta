#include <Core/Util/CPUTime.h>
#include <Core/Thread/ThreadError.h>

#if defined(__APPLE__)
# if defined(__POWERPC__)
# include <ppc_intrinsics.h>
# endif
#include <mach/mach.h>
#include <mach/mach_time.h>
#elif _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#ifdef __ia64__
# include <Core/Thread/Time.h>
#endif

#include <cstring>
#include <cstdlib>
#include <stdio.h>

using namespace Manta;

static bool initialized=false;
static double secondsPerTick_val = 0;

void CPUTime::initialize()
{
  initialized = true;
  resetScale();
}

void CPUTime::resetScale()
{
#if defined(__APPLE__)
  mach_timebase_info_data_t time_info;
  mach_timebase_info(&time_info);

  // Scales to nanoseconds without 1e-9f
  secondsPerTick_val = (1e-9*static_cast<double>(time_info.numer))/
    static_cast<double>(time_info.denom);
#elif defined(_WIN32)
  secondsPerTick_val = 1.0/static_cast<double>(CLOCKS_PER_SEC);
#elif defined(__ia64__)
  secondsPerTick_val = Time::secondsPerTick();
#else
  FILE *fp = fopen("/proc/cpuinfo","r");
  char input[255];
  if (!fp)
    throw ThreadError(std::string("CPUTime::resetScale failed: couldn't find /proc/cpuinfo. Is this not a linux machine?"));
  while (!feof(fp) && fgets(input, 255, fp)) {
    if (strstr(input,"cpu MHz")) {
      char *p = strchr(input,':');
      double MHz = 0.0;
      if (p)
        MHz = atof(p+1);
      secondsPerTick_val = 1000. / MHz * 1e-9;
      break;
    }
  }
  fclose(fp);
#endif
}

//////////
// Return the current CPU time, in terms of clock ticks.
// Time zero is at some arbitrary point in the past.
#if defined(__APPLE__)
/////////////////////////////
// Apple (PowerPC)
/////////////////////////////
CPUTime::SysClock CPUTime::currentTicks()
{
  if (!initialized) initialize();

  // NOTE(boulos): On recent Apple systems using assembly won't give
  // you the proper scaling factor, so we should be using
  // mach_absolute_time.  If this thing doesn't scale though, we
  // should try to find some other solution.
  return mach_absolute_time();
}
#elif defined (_WIN32)
/////////////////////////////
// Win32
/////////////////////////////
CPUTime::SysClock CPUTime::currentTicks()
{
  if (!initialized) initialize();
  return clock();
}

#elif defined(__ia64__)
CPUTime::SysClock CPUTime::currentTicks()
{
  return Time::currentTicks();
}
#else

/////////////////////////////
// Linux and Apple (x86)
/////////////////////////////
CPUTime::SysClock CPUTime::currentTicks()
{
  if (!initialized) initialize();

  unsigned int a, d;
  asm volatile("rdtsc" : "=a" (a), "=d" (d));
  return static_cast<unsigned long long>(a) |
        (static_cast<unsigned long long>(d) << 32);
}
#endif // defined(__APPLE__) && defined(__POWERPC__)

//////////
// Return the current CPU time, in terms of seconds.
// This is slower than currentTicks().  Time zero is at
// some arbitrary point in the past.
#ifdef __ia64__
double CPUTime::currentSeconds()
{
  return Time::currentSeconds();
}
#else
double CPUTime::currentSeconds()
{
  return currentTicks()*secondsPerTick_val;
}
#endif

//////////
// Return the conversion from seconds to ticks.
double CPUTime::ticksPerSecond()
{
  return 1.0/secondsPerTick_val;
}

//////////
// Return the conversion from ticks to seconds.
double CPUTime::secondsPerTick()
{
  return secondsPerTick_val;
}

