#ifndef Manta_Model_Groups_Rayforce_h
#define Manta_Model_Groups_Rayforce_h

#include <Interface/AccelerationStructure.h>

#include <rfut/Target.h>

namespace rfut
{
  class Context;
  class Object;
  class Model;
  template<typename T> class Scene;
  template<typename T> class Device;
  template<typename T> class TraceFcn;
}

namespace Manta
{
  class Lambertian;

  class Rayforce : public AccelerationStructure
  {

  public:

    Rayforce();

    ~Rayforce();

    // Overridden from AccelerationStructure //////////////////////////////////

    // Currently implemented //

    // Load pre-built Rayforce graph cache from a file
    // NOTE: currently the only way to use Rayforce
    bool buildFromFile(const std::string &fileName);

    // Intersect a packet of rays against the rfgraph
    void intersect(const RenderContext& context, RayPacket& rays) const;

    // Not yet implemented //

    void setGroup(Group* new_group);
    void groupDirty();
    Group* getGroup() const;
    void rebuild(int proc=0, int numProcs=1);
    void addToUpdateGraph(ObjectUpdateGraph* graph,
                          ObjectUpdateGraphNode* parent);

    // Overridden from Object (through AccelerationStructure) /////////////////

    void computeBounds(const PreprocessContext& context, BBox& bbox) const;

  private:

    // Helper functions ///////////////////////////////////////////////////////

    void initialize();

    void cleanup();

    // Private data members ///////////////////////////////////////////////////

    // Rayforce related data
    rfut::Context*                  context;
    rfut::Object*                   object;
    rfut::Model*                    model;
    rfut::Scene<Target::System>*    scene;
    rfut::Device<Target::System>*   device;
    rfut::TraceFcn<Target::System>* traceFcn;

    bool inited;

    // Fake material data
    Lambertian *material;

  };

}

#endif // Manta_Model_Groups_Rayforce_h
