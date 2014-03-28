
#ifndef Manta_Model_OakTexture_h
#define Manta_Model_OakTexture_h

#include <Interface/Texture.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/Vector.h>
#include <Core/Math/Noise.h>
#include <Core/Math/MiscMath.h>

namespace Manta {
  class RayPacket;
  class RenderContext;
  template< typename ValueType >
  class OakTexture : public Texture< ValueType > {
    public:
    OakTexture(ValueType const &value1,
               ValueType const &value2,
               Real const ringfreq,
               Real const ringunevenness,
               Real const grainfreq,
               Real const ringnoise,
               Real const ringnoisefreq,
               Real const trunkwobble,
               Real const trunkwobblefreq,
               Real const angularwobble,
               Real const angularwobblefreq,
               Real const ringy,
               Real const grainy );
    virtual ~OakTexture();
    virtual void mapValues(Packet<Color>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
    private:
    OakTexture(
      OakTexture const & );
    OakTexture& operator=(
      OakTexture const & );

    ValueType value1;
    ValueType value2;
    Real const ringfreq;
    Real const ringunevenness;
    Real const grainfreq;
    Real const ringnoise;
    Real const ringnoisefreq;
    Real const trunkwobble;
    Real const trunkwobblefreq;
    Real const angularwobble;
    Real const angularwobblefreq;
    Real const ringy;
    Real const grainy;
  };


  template< class ValueType >
  OakTexture< ValueType >::OakTexture(ValueType const &value1,
                                      ValueType const &value2,
                                      Real const ringfreq,
                                      Real const ringunevenness,
                                      Real const grainfreq,
                                      Real const ringnoise,
                                      Real const ringnoisefreq,
                                      Real const trunkwobble,
                                      Real const trunkwobblefreq,
                                      Real const angularwobble,
                                      Real const angularwobblefreq,
                                      Real const ringy,
                                      Real const grainy )
    : value1( value1 ),
      value2( value2 ),
      ringfreq( ringfreq ),
      ringunevenness( ringunevenness ),
      grainfreq( grainfreq ),
      ringnoise( ringnoise ),
      ringnoisefreq( ringnoisefreq ),
      trunkwobble( trunkwobble ),
      trunkwobblefreq( trunkwobblefreq ),
      angularwobble( angularwobble ),
      angularwobblefreq( angularwobblefreq ),
      ringy( ringy ),
      grainy( grainy )
  {
  }

  template< class ValueType >
  OakTexture< ValueType >::~OakTexture()
  {
  }

  template< class ValueType >
  void OakTexture< ValueType >::mapValues(Packet<Color>& results,
                                          const RenderContext& context,
                                          RayPacket& rays) const
  {
    rays.computeTextureCoordinates3( context );
    for( int i = rays.begin(); i < rays.end(); i++ ) {
      Vector tc = rays.getTexCoords(i);
      Vector offset = VectorFBM( tc * ringnoisefreq, 2, 4, (Real)0.5 );
      Vector Pring = tc + ringnoise * offset;
      Vector vsnoise = VectorNoise( Vector( (Real)0.5,
                                            (Real)0.5,
                                            tc.z()*trunkwobblefreq) );
      Pring += Vector( trunkwobble * vsnoise.x(),
                       trunkwobble * vsnoise.y(),
                       0 );
      Real r = Sqrt( Pring.x() * Pring.x() + Pring.y() * Pring.y() ) * ringfreq;
      r += angularwobble * SmoothStep( r, 0, 5 ) * ScalarNoise( Vector( angularwobble * Pring.x(),
                                                                        angularwobble * Pring.y(),
                                                                        angularwobble * Pring.z() * 0.1 ) );
      r += (Real)0.5 * ScalarNoise( Vector( (Real)0.5, (Real)0.5, r ) );
      Real rfrac = r - Floor( r );
      Real inring = (SmoothStep( rfrac, (Real)0.1, (Real)0.55 ) -
                     SmoothStep( rfrac, (Real)0.7, (Real)0.95 ));
      Vector Pgrain( tc * Vector(grainfreq, grainfreq, grainfreq*(Real)0.05));
      Real grain = 0;
      Real amp = 1;
      for ( int it = 0; it < 2; ++it )
      {
          Real g = (Real)0.8 * ScalarNoise( Pgrain );
          g *= ( (Real)0.3 + (Real)0.7 * inring );
          g = Clamp( (Real)0.8 - g, (Real)0, (Real)1 );
          g = grainy * SmoothStep( g * g, (Real)0.5, 1 );
          if ( it == 0 )
              inring *= (Real)0.7;
          grain = Max( grain, g );
          Pgrain *= 2;
          amp *= (Real)0.5;
      }
      Real value = Interpolate( (Real)1, grain, inring * ringy );
      results.set(i, Interpolate( value2, value1, value ) );
    }
  }
}

#endif
