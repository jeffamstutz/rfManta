
/*
 *  TileTexture.h
 *  Created by Leena Kora on 7/30/07.
 *  
 */

/*
 *  This Texture class is to apply tiles on the floor.
 *  It takes 5 arguments.
 *  First argument indicates the color of a tile and second indicates the color of the gap between the tiles.
 *  Third and fourth argument indicate orientation vectors and
 *  Fifth argument indicates tile gap.
 *
 */
 
#ifndef Manta_Model_TileTexture_h
#define Manta_Model_TileTexture_h

#include <Interface/Texture.h>
#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <Interface/RayPacket.h>
#include <MantaSSE.h>

namespace Manta {
  class RayPacket;
  class RenderContext;
  template<typename ValueType>
  class TileTexture : public Texture<ValueType> {
  public:
    TileTexture(const ValueType& value1, const ValueType& value2, const Vector& v1, const Vector& v2, const Real tile_gap);
    virtual ~TileTexture();

    virtual void mapValues(Packet<ValueType>&, const RenderContext& context, RayPacket& rays) const;
  private:
    TileTexture(const TileTexture&);
    TileTexture& operator=(const TileTexture&);

    ValueType values[2];
    Vector v1;
    Vector v2;
	bool need_w;
    Real tile_gap;
  };

  template<class ValueType>
  TileTexture<ValueType>::TileTexture(const ValueType& value1, const ValueType& value2, const Vector& v1, const Vector& v2, const Real tile_gap)
    : v1(v1), v2(v2), tile_gap(tile_gap)
  {
    values[0] = value1;
    values[1] = value2;
    if(v1.z() == 0 && v2.z() == 0)
      need_w = false;
    else
      need_w = true;
  }
  
  template<class ValueType>
  TileTexture<ValueType>::~TileTexture()
  {
  }
  
  template<class ValueType>
    void TileTexture<ValueType>::mapValues(Packet<ValueType>& results, const RenderContext& context, RayPacket& rays) const
  {
    if(need_w)
      rays.computeTextureCoordinates3(context);
    else
      rays.computeTextureCoordinates2(context);
    for(int i=rays.begin();i<rays.end();i++){
      Real vv1 = Dot(rays.getTexCoords(i), v1);
      Real vv2 = Dot(rays.getTexCoords(i), v2);
	  Real vv1_=vv1;
      Real vv2_=vv2;
	  int i1;
	  int i2;
      if(vv1>0){
        i1 =(int)vv1;
	  }
	  else
	  {
	    int i11=-(int)(-vv1);
		if(i11==vv1)
		 i1=i11;
		else
		 i1=i11-1;
	  }	 
	  if(vv2>0){
        i2 =(int)vv2;
	  }
	  else
	  {
	    int i22=-(int)(-vv2);
		if(i22==vv2)
		 i2=i22;
		else
		 i2=i22-1;
	  }	 	
	  Real ii1=vv1_-i1;
      Real ii2=vv2_-i2;
      int which = ((ii1<tile_gap)||(ii2<tile_gap));
      results.set(i, values[which]);
    }
  }

#ifdef MANTA_SSE
  template<>
    void TileTexture<Color>::mapValues(Packet<Color>& results, const RenderContext& context, RayPacket& rays) const;
  template<>
    void TileTexture<float>::mapValues(Packet<float>& results, const RenderContext& context, RayPacket& rays) const;
#endif

}


#endif
