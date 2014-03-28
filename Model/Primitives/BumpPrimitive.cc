#include <Model/Primitives/BumpPrimitive.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>
#include <Core/Math/Noise.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>

using namespace Manta;
using namespace std;

BumpPrimitive::BumpPrimitive()
{

}

BumpPrimitive::BumpPrimitive(PrimitiveCommon* obj_, Material* mat, Real const k1_, Real const k2_, int const octaves_, Real const lacunarity_, Real const gain_ )
  : PrimitiveCommon(mat),
    bumpobject(obj_),
    k1(k1_),
    k2(k2_),
    octaves( octaves_ ),
    lacunarity( lacunarity_ ),
    gain( gain_ )
{
  
}

BumpPrimitive::~BumpPrimitive()
{

}


void BumpPrimitive::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
	bumpobject->computeBounds(context, bbox);
}

void BumpPrimitive::intersect(const RenderContext& context, RayPacket& rays) const
{
 	 
	RayPacketData raydata;
  	RayPacket object_rays(raydata, RayPacket::UnknownShape, rays.begin(), rays.end(), rays.getDepth(), rays.getAllFlags());

  	for(int i = rays.begin();i<rays.end();i++){
    	object_rays.setRay(i, rays.getRay(i));
    	object_rays.resetHit(i, rays.getMinT(i));
  	}
  	bumpobject->intersect(context, object_rays);
	
	for(int i=rays.begin(); i<rays.end(); i++) 
	{
		
		if(object_rays.wasHit(i))
		{
			
			rays.hit(i, object_rays.getMinT(i), object_rays.getHitMaterial(i), object_rays.getHitPrimitive(i), object_rays.getHitTexCoordMapper(i));
			rays.setHitPrimitive(i, const_cast<BumpPrimitive *>(this));
			rays.setHitMaterial(i, this->getMaterial());
			
		}
		
	}
}

void BumpPrimitive::computeNormal(const RenderContext& context, RayPacket& rays) const
{
	bumpobject->computeNormal(context, rays);

	for(int i=rays.begin(); i<rays.end(); i++)
	{
		Vector nm = rays.getNormal(i);
		Vector T= k1*rays.getHitPosition(i);
		nm=nm+(k2*VectorFBM( T, octaves, lacunarity, gain ));
		nm=nm.normal();
		rays.setNormal(i, nm);
	}

}

Real BumpPrimitive::getk1(void) const
{
	return k1;
}

Real BumpPrimitive::getk2(void) const
{
	return k2;
}
    
int BumpPrimitive::getOctaves(void) const
{
	return octaves;
}

Real BumpPrimitive::getLacunarity(void) const
{
	return lacunarity;
}

Real BumpPrimitive::getGain(void) const
{
	return gain;
}

PrimitiveCommon * BumpPrimitive::getObject(void) const
{
	return bumpobject;
}
