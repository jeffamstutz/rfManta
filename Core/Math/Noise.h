#ifndef MANTA_CORE_MATH_NOISE_H
#define MANTA_CORE_MATH_NOISE_H

#include <MantaTypes.h>
#include <Core/Geometry/Vector.h>
#include <MantaSSE.h>

namespace Manta {

  /**
   * Computes the traditional signed scalar-valued noise function for a
   * given point.  This uses Perlin's original algorithm, but with his new,
   * higher-order quintic interpolant (which makes the second derivative
   * smooth so that displacement mapping looks nice.)  Note that this
   * repeats every 256 units.
   *
   * @param location  the point to evaluate the noise at
   * @return  a signed scalar noise value
   */
  Real ScalarNoise( Vector const& location );

#ifdef MANTA_SSE
  /**
   * This doesn't use any precomputed tables, but explitly computes
   * the random number index (P) and the gradient (G).
   *
   * The gradient table is the one from Perlin's improved noise paper.
   */
  __m128 ScalarNoiseSSE( const __m128& location_x,
                         const __m128& location_y,
                         const __m128& location_z);
#endif

  /**
   * Computes the vector-valued Perlin noise for a given point.  The
   * returned vector has components between -1 and 1.  Beware that this is
   * a very expensive noise computation.  Note that this repeats every 256
   * units.
   *
   * @param location  the point to evaluate the noise at
   * @return  a signed vector
   */
  Vector const VectorNoise( Vector const& location );

  /**
   * Computes a pseudo-random number that is constant between the integer
   * lattice points and discontinuous across integer cell boundaries.  The
   * return value is uniformly distributed between -1 and 1.
   *
   * @param location  the point to evaluate the noise at
   * @return  a scalar value between -1 and 1
   */
  Real ScalarCellNoise( Vector const& location );

  /**
   * Computes a pseudo-random vecotr that is constant between the integer
   * lattice points and discontinuous across integer cell boundaries.  The
   * returned vector has components uniformly distributed between -1 and
   * 1. (i.e. the vector is not a unit vector)
   *
   * @param location  the point to evaluate the noise at
   * @return  a vector with component values between -1 and 1
   */
  Vector const VectorCellNoise( Vector const& location );

  /**
   * Scalar-valued fractional brownian motion noise, a.k.a. fBm noise.
   * This is just the sum of several Perlin noise computations with
   * different scaling factors.  When gain = 1 / lacunarity, this produces
   * "1/f noise".  Lacunarity defines how the domain scales - increasing it
   * adds fine detail but largely leaves the basic structure the same.
   * Gain controls how the range is scaled - increasing it produces higher
   * frequencies.
   *
   * @param location  the location to evaluate the fBm at
   * @param octaves  the number of octaves to include
   * @param lacunarity  how the domain is scaled at each octave
   * @param gain  how the range is scaled at each octave.
   * @return  the scalar-value at that point
   */
  Real ScalarFBM( Vector const& location,
                  int           octaves,
                  Real          lacunarity,
                  Real          gain );

  /**
   * Vector-valued fractional brownian motion noise, a.k.a. fBm noise.
   * This is just the sum of several Perlin noise computations with
   * different scaling factors.  When gain = 1 / lacunarity, this produces
   * "1/f noise".  Lacunarity defines how the domain scales - increasing it
   * adds fine detail but largely leaves the basic structure the same.
   * Gain controls how the range is scaled - increasing it produces higher
   * frequencies.
   *
   * @param location  the location to evaluate the fBm at
   * @param octaves  the number of octaves to include
   * @param lacunarity  how the domain is scaled at each octave
   * @param gain  how the range is scaled at each octave.
   * @return  the vector-value at that Point
   */
  Vector const VectorFBM( Vector const& location,
                          int           octaves,
                          Real          lacunarity,
                          Real          gain );

  /**
   * Perlin's turbulence function.  This is very similiar to the fractional
   * Brownian motion, except that it sums the absolute value at each
   * octave.  This produces sharp creases around the zeros in the noise and
   * gives it a "billowy" appearance.
   *
   * @param location  the location to evaluate the fBm at
   * @param octaves  the number of octaves to include
   * @param lacunarity  how the domain is scaled at each octave
   * @param gain  how the range is scaled at each octave.
   * @return  the scalar-value at that point
   */
  Real Turbulence( Vector const& location,
                   int           octaves,
                   Real          lacunarity,
                   Real          gain );


}

#endif
