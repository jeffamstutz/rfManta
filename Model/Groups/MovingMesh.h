#ifndef Manta_Model_Groups_MovingMesh_h
#define Manta_Model_Groups_MovingMesh_h

#include <Core/Geometry/Vector.h>
#include <Core/Thread/Barrier.h>
#include <Core/Thread/Mutex.h>
#include <Core/Geometry/BBox.h>
#include <Interface/Material.h>
#include <Interface/RayPacket.h>
#include <Interface/TexCoordMapper.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <vector>

namespace Manta
{
  class MeshTriangle;
  using namespace std;
  class MovingMesh : public Mesh {
  public:
    MovingMesh(Mesh* frame0, Mesh* frame1);
    virtual ~MovingMesh(){}

    virtual inline Vector getVertex( size_t tri_id, size_t which_vert, Real t ) const {
      return ((1-t)*frame0->getVertex(tri_id, which_vert) +
                 t *frame1->getVertex(tri_id, which_vert));
    }

    virtual void computeBounds(const PreprocessContext& context, int proc, int numProcs) const;

    virtual BBox getBBox(unsigned int which) {
      BBox bbox;
      bbox.extendByBox(frame0->getBBox(which));
      bbox.extendByBox(frame1->getBBox(which));
      return bbox;
    }

    Mesh* frame0;
    Mesh* frame1;
  };
}

#endif //Manta_Model_Group_Mesh_h
