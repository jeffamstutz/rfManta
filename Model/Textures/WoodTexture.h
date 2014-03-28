
#ifndef Manta_Model_WoodTexture_h
#define Manta_Model_WoodTexture_h

#include <Interface/Texture.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/Vector.h>
#include <Core/Math/Noise.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>


namespace Manta {
  class RayPacket;
  class RenderContext;
  template< typename ValueType >
  class WoodTexture : public Texture< ValueType > {
    public:
    WoodTexture(ValueType const &value1,
                ValueType const &value2,
                Real const scale,
                Real const rscale,
                Real const tscale,
                Real const sharpness,
                int const octaves,
                Real const lacunarity,
                Real const gain );
    virtual ~WoodTexture();
    virtual void mapValues(Packet<Color>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
    private:
    WoodTexture( WoodTexture const & );
    WoodTexture& operator=( WoodTexture const & );
    
    ValueType value1;
    ValueType value2;
    Real scale;
    Real rscale;
    Real tscale;
    Real sharpness;
    int octaves;
    Real lacunarity;
    Real gain;
  };

  template< class ValueType >
  WoodTexture< ValueType >::WoodTexture(ValueType const &value1,
                                        ValueType const &value2,
                                        Real const scale,
                                        Real const rscale,
                                        Real const tscale,
                                        Real const sharpness,
                                        int const octaves,
                                        Real const lacunarity,
                                        Real const gain )
    : value1( value1 ),
      value2( value2 ),
      scale ( scale ),
      rscale( rscale ),
      tscale( tscale ),
      sharpness( sharpness ),
      octaves( octaves ),
      lacunarity( lacunarity ),
      gain( gain )
  {
  }
  
  template< class ValueType >
  WoodTexture< ValueType >::~WoodTexture()
  {
  }

  template< class ValueType >
  void WoodTexture< ValueType >::mapValues(Packet<Color>& results,
                                           const RenderContext& context,
                                           RayPacket& rays) const
  {
    rays.computeTextureCoordinates3( context );
    for( int i = rays.begin(); i < rays.end(); i++ ) {
      Vector T = rays.getTexCoords(i) * scale;
      Real distance = Sqrt( T.x() * T.x() + T.y() * T.y() ) * rscale;
      Real fbm = tscale * ScalarFBM( T, octaves, lacunarity, gain );
      Real value = (Real)0.5 * Cos( distance + fbm ) + (Real)0.5;
      results.set(i, Interpolate( value2, value1,
                                  Pow( value, sharpness ) ));
    }
  }

}

#endif
