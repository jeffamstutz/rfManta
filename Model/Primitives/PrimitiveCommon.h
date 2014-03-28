
#ifndef Manta_Model_PrimitiveCommon_h
#define Manta_Model_PrimitiveCommon_h

#include <Interface/Interpolable.h>
#include <Interface/Primitive.h>
#include <Core/Persistent/MantaRTTI.h>

namespace Manta {
  class ArchiveElement;
  class Material;
  class PrimitiveCommon : public Primitive {
  public:
    PrimitiveCommon(Material* material, const TexCoordMapper* tex = 0);
    // Empty default constructor (used for an array of some primitive)
    PrimitiveCommon() {  };
    virtual ~PrimitiveCommon();

    // Note that this preprocess method sets up the activeLights for the associated
    // material (not sure what happens for shared materials)
    virtual void preprocess(const PreprocessContext&);
    virtual void intersect(const RenderContext& context,
                           RayPacket& rays) const = 0;

    // Accessors.
    void setTexCoordMapper(const TexCoordMapper* new_tex);
    const TexCoordMapper *getTexCoordMapper() const { return tex; }

    void setMaterial( Material *material_ ) { material = material_; }
    Material *getMaterial() const { return material; }

#ifndef SWIG
    virtual PrimitiveCommon* clone(CloneDepth depth, Clonable* incoming=NULL);
    virtual InterpErr interpolate(const std::vector<keyframe_t> &keyframes);
#endif

    void readwrite(ArchiveElement* archive);

  private:
    Material* material;
    const TexCoordMapper* tex;

    PrimitiveCommon( const PrimitiveCommon & );
    PrimitiveCommon &operator = ( const PrimitiveCommon & );
  };

  MANTA_DECLARE_RTTI_DERIVEDCLASS(PrimitiveCommon, Primitive, AbstractClass, readwriteMethod);

}

#endif
