#ifndef Manta_Model_Groups_RFGraph_h
#define Manta_Model_Groups_RFGraph_h

#include <Interface/AccelerationStructure.h>

#include <rfut/Target.h>
#include "Interface/TexCoordMapper.h"

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
  class Mesh;

  class RFGraph : public AccelerationStructure,
                  public TexCoordMapper
  {

  public:

    RFGraph();

    ~RFGraph();

    // Overridden from AccelerationStructure //////////////////////////////////

    // Currently implemented //

    // Load pre-built Rayforce graph cache from a file
    bool buildFromFile(const std::string &fileName);

    // Save out the Rayforce graph cache
    bool saveToFile(const std::string &fileName);

    // Intersect a packet of rays against the rfgraph
    void intersect(const RenderContext& context, RayPacket& rays) const;

    // Set the mesh to be traced
    void setGroup(Group* new_group);

    // Get the current mesh
    Group* getGroup() const;

    // Not yet implemented //

    void groupDirty();
    void rebuild(int proc=0, int numProcs=1);
    void addToUpdateGraph(ObjectUpdateGraph* graph,
                          ObjectUpdateGraphNode* parent);

    // Overridden from Object (through AccelerationStructure) /////////////////

    void computeBounds(const PreprocessContext& context, BBox& bbox) const;

    void preprocess(const PreprocessContext &context);

    // Overridden from TexCoordMapper /////////////////////////////////////////

    void computeTexCoords2(const RenderContext&, RayPacket&) const;

    void computeTexCoords3(const RenderContext&, RayPacket&) const;

  private:

    // Helper functions ///////////////////////////////////////////////////////

    void initialize();

    void cleanup();

    // Private data members ///////////////////////////////////////////////////

    //void *graph;

    // Rayforce related data
    rfut::Context*                  context;
    rfut::Object*                   object;
    rfut::Model*                    model;
    rfut::Scene<Target::System>*    scene;
    rfut::Device<Target::System>*   device;

    std::string saveToFileName;

    Mesh* currMesh;

  };

}

#endif // Manta_Model_Groups_RFGraph_h
