/*
 *  BumpPrimitive.h
 *  
 *
 *  Created by Leena Kora on 6/5/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

/*
This primitive class takes an object and a material with k1, k2, lacunarity, gain, octaves values and bump maps that object with the material given as an argument
*/

#ifndef Manta_Model_BumpPrimitive_h
#define Manta_Model_BumpPrimitive_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>

namespace Manta
{

	class BumpPrimitive : public PrimitiveCommon 
	{
  
		public:
			BumpPrimitive();
			BumpPrimitive(PrimitiveCommon* obj_, Material* mat_, Real const k1_, Real const k2_, int const octaves_, Real const lacunarity_, Real const gain_ );
			virtual ~BumpPrimitive();
    
			virtual void computeBounds(const PreprocessContext& context, BBox& bbox) const;
			virtual void intersect(const RenderContext& context, RayPacket& rays) const;
			virtual void computeNormal(const RenderContext& context, RayPacket& rays) const;  
	
			Real getk1(void) const;
			Real getk2(void) const;
			int getOctaves(void) const;
			Real getLacunarity(void) const;
			Real getGain(void) const;
			PrimitiveCommon* getObject(void) const;
     
		private:
			PrimitiveCommon* bumpobject;
			Real k1;
			Real k2;
			int octaves;
			Real lacunarity;
			Real gain;
		
	};
}

#endif
