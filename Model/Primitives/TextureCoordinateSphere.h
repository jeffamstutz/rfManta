
#ifndef Manta_Model_TextureCoordinateSphere_h
#define Manta_Model_TextureCoordinateSphere_h

#include <Model/Primitives/Sphere.h>
#include <Core/Geometry/Vector.h>

namespace Manta {
  class ArchiveElement;
  using namespace std;

  class TextureCoordinateSphere : public Sphere {
    //This class exists to simplify texture mapping of spheres.
    //ComputeTexCoords returns the texture coordinates that it is given
    //in the constuctor. Otherwise it is just a sphere.
  public:
    TextureCoordinateSphere();
    TextureCoordinateSphere(Material* material, const Vector& center, Real radius, 
                Vector tc);
    virtual ~TextureCoordinateSphere();

    virtual void computeTexCoords2(const RenderContext& context,
                                   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
                                   RayPacket& rays) const;

  private:
    Vector TexCoords;
  };
}

#endif
