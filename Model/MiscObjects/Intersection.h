
#ifndef Manta_Model_Intersection_h
#define Manta_Model_Intersection_h

#include <Interface/Object.h>

namespace Manta {
  class Intersection : public Object {
  public:
    Intersection(Object* object1, Object* object2);
    virtual ~Intersection();

    virtual void preprocess(const PreprocessContext&);
    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const;
  private:
    Object* object1;
    Object* object2;
  };
}

#endif
