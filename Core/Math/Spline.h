
#ifndef Manta_DynLT_Spline_h
#define Manta_DynLT_Spline_h

#include <MantaTypes.h>

#include <map>

namespace Manta
{
  // Shamelessly borrowed from MacGuyver souce code (cs7960, Fall 2006)
  //
  // Cubic cardinal spline.  This is a subset of piece-wise cubic
  // Hermite splines where the tangent vectors are implicitly
  // calculated from the difference between the neighboring knots.
  // The tension value scales these tangents.  A tension of zero
  // causes the curve to go straight between each knot, while higher
  // values cause smoother interpolation.  A tension of 0.5 produces
  // the Catmull-Rom spline.  To smooth out the time differences, the
  // tangent adjustment described by Kochanek and Bartels is used.

  template<typename T>
  class Spline
  {
  public:
    inline Spline(Real const tension=0.5) : tension(tension) { }
    inline ~Spline(void) { /* no-op */ }

    inline void addKnot(Real const time, T const& value)
    {
      knots.insert(std::pair<Real, T>(time, value));
    }

    inline void reset(void)
    {
      knots.clear();
    }

    inline T const interpolate(Real const time) const
    {
      typename std::map<Real, T>::const_iterator m0, m1, m2, m3;

      // Pick the knots
      m2=knots.lower_bound(time);
      if (m2==knots.end())
        --m2;
      if ((m1=m2)==knots.begin())
        ++m2;
      else
        --m1;
      m3=m2;
      ++m3;
      if (m3==knots.end())
        --m3;
      if ((m0=m1) != knots.begin())
        --m0;

      // Compute the tangets
      T p0=m0->second;
      T p1=m1->second;
      T p2=m2->second;
      T p3=m3->second;
      Real time0=m0->first;
      Real time1=m1->first;
      Real time2=m2->first;
      Real time3=m3->first;
      T t1=((p2 - p0)*(2.*tension*(time1 - time0)/(time2 - time0)));
      T t2=((p3 - p1)*(2.*tension*(time3 - time2)/(time3 - time1)));

      // Scale time to [0, 1]
      Real scaled=(time - time1)/(time2 - time1);
      Real scaled2=scaled*scaled;

      // Compute the weights
      Real w1=(2*scaled - 3.)*scaled2 + 1.;
      Real w2=(-2*scaled + 3.)*scaled2;
      Real w3=(scaled - 2.)*scaled2 + scaled;
      Real w4=(scaled - 1.)*scaled2;

      return (w1*p1 + w2*p2 + w3*t1 + w4*t2);
    }

    inline T const operator()(Real const time) const
    {
      return this->interpolate(time);
    }

  private:
    std::map<Real, T> knots;
    Real tension;
  };
}

#endif // Manta_DynLT_Spline_h
