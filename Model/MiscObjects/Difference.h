
#ifndef Manta_Model_Difference_h
#define Manta_Model_Difference_h

#include <Interface/Object.h>

namespace Manta {
  class Difference : public Object {
  public:
    Difference(Object* object1, Object* object2);
    virtual ~Difference();

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
