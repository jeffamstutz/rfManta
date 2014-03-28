


#ifndef Manta_Model_Phong_h
#define Manta_Model_Phong_h

#include <MantaTypes.h>
#include <Model/Materials/LitMaterial.h>
#include <Core/Color/Color.h>
#include <Interface/Texture.h>

namespace Manta{
  class LightSet;

  class Phong : public LitMaterial {
  public:
	
    // Note if refl == 0 the phong shader won't cast a reflected ray.
    Phong(const Color& diffuse, const Color& specular,
          int specpow, ColorComponent refl = 0);
    Phong(const Texture<Color>* diffuse, const Texture<Color>* specular,
          int specpow, const Texture<ColorComponent>* refl);
    virtual ~Phong();

    virtual void shade(const RenderContext& context, RayPacket& rays) const;
    void setDiffuse(Texture<Color>* tex)          { diffusetex = tex; }
    void setSpecular(Texture<Color>* tex, int sp) { speculartex = tex; specpow = sp; }
    void setReflective(Texture<ColorComponent>* tex, bool nonzero ) { refltex = tex; do_refl = nonzero; } 
    void setReflective(const Texture<ColorComponent>* tex );
  private:
    const Texture<Color>* diffusetex;
    const Texture<Color>* speculartex;
    const Texture<ColorComponent>* refltex;
    Real highlight_threshold;
    int specpow;
    bool do_refl;
  };
}

#endif
