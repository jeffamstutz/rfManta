#ifndef Manta_Model_Groups_RFGraph_h
#define Manta_Model_Groups_RFGraph_h

#include <Interface/AccelerationStructure.h>

namespace Manta
{

  class RFGraph : public AccelerationStructure
  {

  public:

    RFGraph();

    ~RFGraph();

    // Overridden from AccelerationStructure //////////////////////////////////

    // Currently implemented //

    // Load pre-built Rayforce graph cache from a file
    // NOTE: currently the only way to use RFGraph
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

    // Private data members ///////////////////////////////////////////////////

    void *graph;
  };

}

#endif // Manta_Model_Groups_RFGraph_h
