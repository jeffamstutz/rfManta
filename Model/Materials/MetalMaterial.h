
#ifndef Manta_Model_MetalMaterial_h
#define Manta_Model_MetalMaterial_h

#include <Model/Materials/LitMaterial.h>
#include <Core/Color/Color.h>
#include <Interface/Texture.h>

namespace Manta{
  class Archive;
  class LightSet;

  class MetalMaterial : public LitMaterial {
  public:
    MetalMaterial();
    MetalMaterial(const Color& specular_reflectance, int phong_exponent = 100);
    MetalMaterial(const Texture<Color>* specular_reflectance,
		  int phong_exponent = 100);
    virtual ~MetalMaterial();
  
    virtual void shade(const RenderContext& context, RayPacket& rays) const;
    void readwrite(ArchiveElement* archive);
  private:
    const Texture<Color>* specular_reflectance;
    int phong_exponent;
  };
}


#endif
