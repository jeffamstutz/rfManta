
/*
 * This texture class takes three arguments.
 * First one indicates the color of the sky, which can be either blue or black)
 * Second one indicates coverage of cloud in the sky(0-1) and
 * Third one indicates the dimension in which clouds occur(0, 1 or 2) 
 * 0 indicates x-axis, 1 indicates y-axis and 2 z-axis.
 *
 */
     
#ifndef Manta_Model_CloudTexture_h
#define Manta_Model_CloudTexture_h

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
  class CloudTexture : public Texture< ValueType > {
  public:
    CloudTexture( ValueType const &Skycolor, Real const cloud_cover, int const cloud_coordinate );
    virtual ~CloudTexture();
    virtual void mapValues(Packet<Color>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
    Real cloud_coverage(Real value) const;
  private:
    CloudTexture(
                 CloudTexture const & );
    CloudTexture& operator=(
                            CloudTexture const & );
    
    ValueType skycolor;
    Real scale;
    Real fscale;
    Real tscale;
    int octaves;
    Real lacunarity;
    Real gain;
    Real cloud_cover;
    int cloud_coordinate;
  };

  template< class ValueType >
  CloudTexture< ValueType >::CloudTexture(ValueType const &Skycolor, Real const cloud_cover, int const cloud_coordinate)      
    : cloud_cover( cloud_cover ),
      cloud_coordinate(cloud_coordinate)
  {
    skycolor=Color::white()-Skycolor;
    scale=0.1;
    fscale=1;
    tscale=1;
    octaves=8;
    lacunarity=1.8;
    gain=0.48;
  }
  
  template< class ValueType >
  CloudTexture< ValueType >::~CloudTexture()
  {
  }
  
  template< class ValueType >
  void CloudTexture< ValueType >::mapValues(Packet<Color>& results,
                                            const RenderContext& context,
                                            RayPacket& rays) const
  {
    enum cloud_dimension {
      x_coordinate ,
      y_coordinate ,
      z_coordinate
    };

    rays.computeTextureCoordinates3( context );
    for( int i = rays.begin(); i < rays.end(); i++ ) {
     
      Vector T = rays.getTexCoords(i) * (scale * tscale);
      ColorComponent density;
      ColorComponent value;
     
      switch(cloud_coordinate) {
      case x_coordinate:
        density=cloud_coverage(rays.getTexCoords(i).x());
        value =(( rays.getTexCoords(i).x()) * fscale + (Real)Turbulence( T, octaves, lacunarity, gain ));
        break;
      case y_coordinate:
        density=cloud_coverage(rays.getTexCoords(i).y());
        value =(( rays.getTexCoords(i).y()) * fscale + (Real)Turbulence( T, octaves, lacunarity, gain ));
        break;
      case z_coordinate:
        density=cloud_coverage(rays.getTexCoords(i).z());
        value =(( rays.getTexCoords(i).z()) * fscale + (Real)Turbulence( T, octaves, lacunarity, gain ));
        break;
      }
          
      value=value*(Real)0.5+(Real)0.5;
      value=value*density;
      value=value*(Real)0.5+(Real)0.5;
      results.set(i, (Interpolate( skycolor, Color::white(), value))+Color::white());
    
    }
  }

  template< class ValueType >
  Real CloudTexture< ValueType >::cloud_coverage(Real value) const
  {
    Real total_cover=(cloud_cover*1.6215)+249.8;
    Real c_value = total_cover-value;
    if(c_value < 0)
      {
        c_value=0;
      }
    Real density_value = 255 - ((c_value));
    return density_value;
  }
}

#endif
