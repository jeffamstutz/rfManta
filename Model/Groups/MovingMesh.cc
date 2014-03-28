#include <Model/Groups/MovingMesh.h>
#include <Core/Util/Preprocessor.h>
#include <Core/Util/UpdateGraph.h>
#include <Core/Exceptions/InternalError.h>
#include <Model/Primitives/MeshTriangle.h>

using namespace Manta;

MovingMesh::MovingMesh(Mesh* frame0, Mesh* frame1) :
  Mesh(),
  frame0(frame0),
  frame1(frame1) {
  vertices = frame0->vertices;
  vertexNormals = frame0->vertexNormals;
  texCoords = frame0->texCoords;
  materials = frame0->materials;
  vertex_indices = frame0->vertex_indices;
  normal_indices = frame0->normal_indices;
  texture_indices = frame0->texture_indices;
  face_material = frame0->face_material;

  objs = frame0->getVectorOfObjects();
  for (size_t i = 0; i < objs.size(); i++) {
    ((MeshTriangle*)objs[i])->attachMesh(this);
  }
}

void MovingMesh::computeBounds(const PreprocessContext& context,
                               int proc, int numProcs) const
{
  if (proc == 0) {
    this->bbox.reset();
  }

  frame0->computeBounds(context, proc, numProcs);
  frame1->computeBounds(context, proc, numProcs);

  if (proc == 0) {
    dirtybbox = false;
    ((Group*)frame0)->computeBounds(context, bbox);
    ((Group*)frame1)->computeBounds(context, bbox);
  }

  //Need to wait for other threads to finish computing bbox
  barrier.wait(numProcs);
}
