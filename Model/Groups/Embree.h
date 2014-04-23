#ifndef Manta_Model_Groups_Embree_h
#define Manta_Model_Groups_Embree_h

#include "Interface/AccelerationStructure.h"
#include "Interface/TexCoordMapper.h"

namespace Manta
{
  class Mesh;

  class Embree : public AccelerationStructure, public TexCoordMapper
  {

  public:

    Embree();

    ~Embree();

    // Overridden from AccelerationStructure //////////////////////////////////

    // Currently implemented //

    // Intersect a packet of rays against the rfgraph
    void intersect(const RenderContext& context, RayPacket& rays) const;

    // Set the mesh to be traced
    void setGroup(Group* new_group);

    // Get the current mesh
    Group* getGroup() const;

    // Not yet implemented //

    bool buildFromFile(const std::string &fileName);
    bool saveToFile(const std::string &fileName);
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

    // Private data members ///////////////////////////////////////////////////

    bool inited;

    Mesh* currMesh;
  };

}

#endif // Manta_Model_Groups_Embree_h
