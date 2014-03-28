
#ifndef Manta_Interface_MPT_h
#define Manta_Interface_MPT_h

/*
 * If this ever gets used outside of Instances, it should be moved to a
 * more general spot
 */
#include <MantaTypes.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/VectorT.h>

namespace Manta {
  class Material;
  class Primitive;
  class TexCoordMapper;

  // Convenience struct to hold a material, primitive and texcoordmapper
  struct MPT {
    const Material* material;
    const Primitive* primitive;
    const TexCoordMapper* tex;
    MPT(const Material* material, const Primitive* primitive,
                                const TexCoordMapper* tex)
      : material(material), primitive(primitive), tex(tex)
    {
    }
  };

  // Convenience struct to hold a material, primitive and texcoordmapper
  // and a scale/inverse scale
  struct MPTscale {
    const Material* material;
    const Primitive* primitive;
    const TexCoordMapper* tex;
    Real scale;
    Real inv_scale;
    MPTscale(const Material* material, const Primitive* primitive,
                                                 const TexCoordMapper* tex, Real scale, Real inv_scale)
      : material(material), primitive(primitive), tex(tex),
                        scale(scale), inv_scale(inv_scale)
    {
    }
  };

  // Convenience struct to hold normal, texcoord, etc to avoid
  // stomping useful data in the scratchpad.
  struct InstanceShadingData {
    Vector normal;
    Vector geometricNormal;
    VectorT<Real, 2> texcoord2;
    // NOTE(boulos): When we update texture coordinates to be
    // separated add this one too.
    //
    //Vector texcoord3;
    Vector surfDerivU;
    Vector surfDerivV;

    InstanceShadingData(const Vector& normal,
                        const Vector& geometricNormal,
                        const VectorT<Real, 2>& tex2,
                        const Vector& dpdu,
                        const Vector& dpdv
                        )
      : normal(normal), geometricNormal(geometricNormal),
        texcoord2(tex2),
        surfDerivU(dpdu), surfDerivV(dpdv)
    {
    }
  };
}

#endif
