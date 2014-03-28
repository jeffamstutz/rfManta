
#ifndef Manta_Model_MarbleTexture_h
#define Manta_Model_MarbleTexture_h

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
  class MarbleTexture : public Texture< ValueType > {
    public:
    MarbleTexture(
      ValueType const &value1,
      ValueType const &value2,
      Real const scale,
      Real const fscale,
      Real const tscale,
      int const octaves,
      Real const lacunarity,
      Real const gain );
    virtual ~MarbleTexture();
    virtual void mapValues(Packet<ValueType>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
    private:
    MarbleTexture(
      MarbleTexture const & );
    MarbleTexture& operator=(
      MarbleTexture const & );
    
    ValueType value1;
    ValueType value2;
    Real scale;
    Real fscale;
    Real tscale;
    int octaves;
    Real lacunarity;
    Real gain;
  };

  template< class ValueType >
  MarbleTexture< ValueType >::MarbleTexture(
      ValueType const &value1,
      ValueType const &value2,
      Real const scale,
      Real const fscale,
      Real const tscale,
      int const octaves,
      Real const lacunarity,
      Real const gain )
    : value1( value1 ),
      value2( value2 ),
      scale ( scale ),
      fscale( fscale ),
      tscale( tscale ),
      octaves( octaves ),
      lacunarity( lacunarity ),
      gain( gain )
  {
  }
  
  template< class ValueType >
  MarbleTexture< ValueType >::~MarbleTexture()
  {
  }
  
  template< class ValueType >
  void MarbleTexture< ValueType >::mapValues(Packet<ValueType>& results,
                                             const RenderContext& context,
                                             RayPacket& rays) const
  {
    rays.computeTextureCoordinates3( context );
    for( int i = rays.begin(); i < rays.end(); i++ ) {
      Vector T = rays.getTexCoords(i) * (scale * fscale);
      ColorComponent value = (Real)0.25 *
        Cos( rays.getTexCoords(i).x() * scale + tscale *
             (Real)Turbulence( T, octaves, lacunarity, gain ) );
      results.set(i, Interpolate( value1, value2, value ));
    }
  }
}

#endif
