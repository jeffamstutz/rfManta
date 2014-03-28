#ifndef Manta_Core_Stat_h
#define Manta_Core_Stat_h

#include <UseStatsCollector.h>
#include <Core/Thread/Mutex.h>

#include <float.h>
#include <math.h>

namespace Manta
{
  class Stat
  {
#ifndef USE_STATS_COLLECTOR
  public:
    Stat(void) { /* no-op */ }

    inline void reset(void) { /* no-op */ }
    inline void increment(float /* inc_value */ = 1.0f) { /* no-op */ }
    inline void increment(int /* inc_value */) { /* no-op */ }
    inline float getTotal(void) { return -1.0f; }
    inline float getAverage(void) { return -1.0f; }
    inline float getVariance(void) { return -1.0f; }
    inline float getDeviation(void) { return -1.0f; }
    inline float getMinimum(void) { return -1.0f; }
    inline float getMaximum(void) { return -1.0f; }
#else
  public:
    Stat(void) : mutex("Stat mutex") { reset(); }

    inline void reset(void)
    {
      mutex.lock();

      sum_values=sum_squares=num_values=0.0f;
      min_value=FLT_MAX;
      max_value =-FLT_MAX;

      mutex.unlock();
    }

    inline void increment(float inc_value=1.0f)
    {
      mutex.lock();

      sum_values += inc_value;
      sum_squares += (inc_value*inc_value);
      num_values += 1.0f;
      if (inc_value<min_value)
        min_value=inc_value;
      if (inc_value>max_value)
        max_value=inc_value;

      mutex.unlock();
    }

    inline void increment(int inc_value)
    {
      increment(static_cast<float>(inc_value));
    }

    inline float getTotal(void)
    {
      return sum_values;
    }

    inline float getAverage(void)
    {
      return sum_values/num_values;
    }

    inline float getVariance(void)
    {
      float inv_n=1.f/num_values;
      float first=sum_squares*inv_n;
      float second=sum_values*inv_n;
      return first - second*second;
    }

    inline float getDeviation(void)
    {
      return sqrt(getVariance());
    }

    inline float getMinimum(void)
    {
      return min_value;
    }

    inline float getMaximum(void)
    {
      return max_value;
    }

    float sum_values;
    float sum_squares;
    float min_value;
    float max_value;
    float num_values;

  private:
    Mutex mutex;
#endif
  };
}

#endif // Manta_Core_Stat_h
