
#ifndef Manta_Model_TextureCoordinateCylinder_h
#define Manta_Model_TextureCoordinateCylinder_h

#include <Model/Primitives/Cylinder.h>
#include <Core/Geometry/Vector.h>

namespace Manta
{
  class TextureCoordinateCylinder : public Cylinder {
    //This class exists to simplify texture mapping of spheres.
    //ComputeTexCoords returns the texture coordinates that it is given
    //in the constuctor. Otherwise it is just a sphere.
  public:
    TextureCoordinateCylinder(Material* mat, const Vector& bottom, const Vector& top,
                              Real radius, Vector tc1, Vector tc2);
    virtual ~TextureCoordinateCylinder();
    
    virtual void computeTexCoords2(const RenderContext& context,
				   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
				   RayPacket& rays) const;
    
  private:
    Vector TexCoord1, TexCoord2;
  };
}

#endif


