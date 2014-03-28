#ifndef Manta_Interface_MeshTriangle_h
#define Manta_Interface_MeshTriangle_h

#include <Core/Persistent/MantaRTTI.h>
#include <Core/Util/Assert.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/Primitive.h>
#include <Model/Groups/Mesh.h>

namespace Manta {

  class MeshTriangle : public Primitive {
  public:

    virtual ~MeshTriangle() { };

    virtual void attachMesh(Mesh *mesh) { this->mesh = mesh; }

    virtual void attachMesh(Mesh *mesh, unsigned int id)
    {
      this->mesh = mesh;
      myID = id;
    }

    //which should be 0, 1, or 2.
    virtual Vector getVertex(unsigned int which) const
    {
      ASSERT(which < 3);
      return mesh->getVertex(myID, which);
    }

    Material *getMaterial() const {
      return mesh->materials[mesh->face_material[myID]];
    }

    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const
    {
      bbox.extendByBox(mesh->getBBox(myID));
    }

    virtual Real computeArea() const {
      const Vector v0 = getVertex(0);
      return static_cast<Real>(0.5) * Cross(getVertex(2) - v0,
                                            getVertex(1) - v0).length();
    }

    virtual void computeSurfaceDerivatives(const RenderContext& context,
                                           RayPacket& rays) const;

    void readwrite(ArchiveElement*);
    //List of the triangle classes that implement MeshTriangle.
    enum TriangleType {
      WALD_TRI,
      KENSLER_SHIRLEY_TRI,
      MOVING_KS_TRI,
    };

  protected:
    MeshTriangle(): myID(0), mesh(NULL) { }
    MeshTriangle(Mesh *mesh, unsigned int id): myID(id), mesh(mesh) { }

    unsigned int myID;
    Mesh *mesh;
  };

  MANTA_DECLARE_RTTI_DERIVEDCLASS(MeshTriangle, Primitive, AbstractClass, readwriteMethod);
}
#endif //Manta_Interface_MeshTriangle_h
