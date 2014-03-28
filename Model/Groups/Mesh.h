#ifndef Manta_Model_Groups_Mesh_h
#define Manta_Model_Groups_Mesh_h

#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>
#include <Interface/Material.h>
#include <Interface/RayPacket.h>
#include <Interface/TexCoordMapper.h>
#include <Model/Groups/Group.h>

#include <vector>

namespace Manta
{
  class MeshTriangle;
  using namespace std;
  class Mesh : public Group {
  public:

    // If a triangle has a vertex without texture coordinates, this
    // will be its index into the texture_indices array. Similarly
    // for binormals and tangents.
    static const unsigned int kNoTextureIndex;
    static const unsigned int kNoBinormalIndex;
    static const unsigned int kNoTangentIndex;

    vector<Vector> vertices;
    vector<Vector> vertexNormals;
    vector<Vector> vertexBinormals;
    vector<Vector> vertexTangents;
    vector<Vector> texCoords;
//     vector<Vector> faceNormals; //not handled for now
    vector<Material*> materials;

    //Note: We might want to templatize the indices in case we end up
    //having lots of meshes with small (less than a short) numbers of
    //vertices. Most likely though is that we have just one or two
    //meshes total, so the space savings is minimal and we can just
    //avoid template ugliness.

    // Per vertex data.  size() == 3*numTriangles;
    vector<unsigned int> vertex_indices;
    vector<unsigned int> normal_indices;
    vector<unsigned int> binormal_indices;
    vector<unsigned int> tangent_indices;
    vector<unsigned int> texture_indices;

    // Per face data.  size() == numTriangles;
    //vector<unsigned int> face_normals; //not handled for now.
    vector<unsigned int> face_material;

    // Should we support having both face_normals and vertex_normals?

    Mesh();
    virtual ~Mesh();

    virtual Mesh* clone(CloneDepth depth, Clonable* incoming=NULL);

    virtual InterpErr serialInterpolate(const std::vector<keyframe_t> &keyframes);
    virtual InterpErr parallelInterpolate(const std::vector<keyframe_t> &keyframes,
                                           int proc, int numProc);
    virtual bool isParallel() const { return true; }

    bool hasVertexNormals() const { return !normal_indices.empty(); }
    void discardVertexNormals() {
      normal_indices.clear();
      vertexNormals.clear();
    }
    void interpolateNormals();

    // These methods should not be used, instead use addTriangle.
    virtual void add(Object*);
    virtual void set( int, Object *);
    virtual void remove(Object*, bool);

    inline MeshTriangle* get( size_t i ) {
      ASSERT( i < objs.size() );
      //For some reason static_cast doesn't work when I inline this
      return reinterpret_cast<MeshTriangle*>(objs[i]);
    }
    inline const MeshTriangle* get( size_t i ) const {
      ASSERT( i < objs.size() );
      //For some reason static_cast doesn't work when I inline this
      return reinterpret_cast<MeshTriangle* const>(objs[i]);
    }

    virtual inline Vector getVertex( size_t tri_id, size_t which_vert, Real t = 0 ) const {
      return vertices[vertex_indices[3*tri_id + which_vert]];
    }

    //this only adds the triangle to the group. You still need to add
    //the vertices, materials, etc. to the mesh.
    void addTriangle(MeshTriangle *tri);

    virtual void computeBounds(const PreprocessContext& context, BBox& bbox) const {
      Group::computeBounds(context, bbox);
    }
    virtual void computeBounds(const PreprocessContext& context, int proc, int numProcs) const;

    // You can't get access to the actual primitive, because it may not
    // actually exist.  You can get the bounds of it, though.
    virtual BBox getBBox(unsigned int which);

    virtual void preprocess(const PreprocessContext& context);

    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent);

    //removes degenerate triangles from mesh.
    void removeDegenerateTriangles();

    //Resize the mesh geometry so the diagonal is scalingFactor longer.
    void scaleMesh(Real scalingFactor);

    void readwrite(ArchiveElement* archive);

    Mesh& operator+=(const Mesh& otherMesh);
  };

  MANTA_DECLARE_RTTI_DERIVEDCLASS(Mesh, Group, ConcreteClass, readwriteMethod);
}

#endif //Manta_Model_Group_Mesh_h
