
#ifndef Manta_Interface_Object_h
#define Manta_Interface_Object_h

#include <Interface/Interpolable.h>

namespace Manta {

  class BBox;
  class Object;
  class PreprocessContext;
  class RayPacket;
  class RenderContext;
  class UpdateContext;

  template<class KeyType> class UpdateGraph;
  template<class KeyType> class UpdateGraphNode;
  typedef UpdateGraph<Object*> ObjectUpdateGraph;
  typedef UpdateGraphNode<Object*> ObjectUpdateGraphNode;


  class Object : virtual public Interpolable {
  public:
    Object();
    virtual ~Object();

    // If you have a function that modifies the state of the class,
    // you need to call context.done().  This is to maintain
    // consistency for when you have multiple threads calling
    // preprocess.
    virtual void preprocess(const PreprocessContext& context) {};
    virtual void performUpdate(const UpdateContext& context);
    virtual void computeBounds(const PreprocessContext& context, BBox& bbox) const = 0;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const = 0;
    // If you'd like to be included in the update graph (so that you
    // can possibly be updated), simply add yourself in. By default,
    // nobody is inserted.
    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent) { }
  private:
    // Object(const Object&);
    // Object& operator=(const Object&);
  };
}

#endif
