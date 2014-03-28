#ifndef Manta_Model_Primitives_ValuePrimitive_h
#define Manta_Model_Primitives_ValuePrimitive_h

#include <Interface/Primitive.h>

namespace Manta {
  template<typename T>
  class ValuePrimitive : public Primitive {
  public:
    typedef T ValueType;

  public:
    ValuePrimitive(Primitive *prim) : prim(prim) {}
    ValuePrimitive(Primitive *prim, const T& val) : prim(prim), val(val) {}

    // Object interface.
    void computeBounds(const PreprocessContext& context, BBox& bbox) const {
      prim->computeBounds(context, bbox);
    }

    // Primitive interface.
    void preprocess(const PreprocessContext& context){
      prim->preprocess(context);
    }

    void intersect(const RenderContext& context,
                   RayPacket& rays) const {
      prim->intersect(context, rays);
      for(int i=rays.begin(); i<rays.end(); i++){
        if(rays.getHitPrimitive(i) == this->prim)
          rays.setHitPrimitive(i, this);
      }
    }

    void computeNormal(const RenderContext& context,
                       RayPacket& rays) const {
      prim->computeNormal(context, rays);
    }

    void computeSurfaceDerivatives(const RenderContext& context,
                                   RayPacket& rays) const {
      prim->computeSurfaceDerivatives(context, rays);
    }

    void setTexCoordMapper(const TexCoordMapper* new_tex){
      prim->setTexCoordMapper(new_tex);
    }

    void getRandomPoints(Packet<Vector>& points,
                         Packet<Vector>& normals,
                         Packet<Real>& pdfs,
                         const RenderContext& context,
                         RayPacket& rays) const {
      prim->getRandomPoints(points, normals, pdfs, context, rays);
    }

    // ValuePrimitive interface.
    T getValue() const { return val; }

    void setValue(const T& newval) {
      val = newval;
    }

    const Primitive *getPrimitive() const { return prim; }
    Primitive *getPrimitive() { return prim; }

  private:
    Primitive *prim;
    T val;
  };
}

#endif
