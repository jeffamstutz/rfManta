#include <Core/Exceptions/InternalError.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Core/Persistent/stdRTTI.h>
#include <Core/Util/Preprocessor.h>
#include <Core/Util/UpdateGraph.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/Context.h>
#include <Model/Groups/Mesh.h>
#include <Model/Primitives/MeshTriangle.h>

using namespace Manta;

const unsigned int Mesh::kNoTextureIndex = static_cast<unsigned int>(-1);
const unsigned int Mesh::kNoBinormalIndex = static_cast<unsigned int>(-1);
const unsigned int Mesh::kNoTangentIndex = static_cast<unsigned int>(-1);

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
  for (unsigned int i=0; i < objs.size(); ++i) {
    delete objs[i];
  }
  // Should we delete the materials?  Maybe those are shared amongst other
  // meshes, so it might be best to make that a user's job.
}

Mesh* Mesh::clone(CloneDepth depth, Clonable* incoming)
{
  Mesh *copy;
  if (incoming)
    copy = dynamic_cast<Mesh*>(incoming);
  else
    copy = new Mesh();

  Group::clone(depth, copy);

  for (unsigned int i=0; i < objs.size(); ++i) {
    copy->get(i)->attachMesh(copy);
  }

  copy->vertices = vertices;
  copy->vertexNormals = vertexNormals;
  copy->vertexBinormals = vertexBinormals;
  copy->vertexTangents = vertexTangents;
  copy->texCoords = texCoords;
  copy->materials = materials;
  copy->vertex_indices = vertex_indices;
  copy->normal_indices = normal_indices;
  copy->binormal_indices = binormal_indices;
  copy->tangent_indices = tangent_indices;
  copy->texture_indices = texture_indices;
  copy->face_material = face_material;

  return copy;
}

Interpolable::InterpErr
Mesh::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  return parallelInterpolate(keyframes, 0, 1);
}

Interpolable::InterpErr
Mesh::parallelInterpolate(const std::vector<keyframe_t> &keyframes,
                           int proc, int numProcs)
{
  InterpErr worstError = success;

  Mesh **meshes = MANTA_STACK_ALLOC(Mesh*, keyframes.size());
  for (unsigned int frame=0; frame < keyframes.size(); ++frame) {
    Mesh *mesh = dynamic_cast<Mesh*>(keyframes[frame].keyframe);
    if (mesh == NULL)
      return notInterpolable;

    meshes[frame] = mesh;

    ASSERT(vertices.size() == mesh->vertices.size());
    ASSERT(vertexNormals.size() == mesh->vertexNormals.size());
    ASSERT(texCoords.size() == mesh->texCoords.size());
    ASSERT(materials.size() == mesh->materials.size());

    //These vectors should be identical down to the individual
    //elements. But we don't check that far down.
    ASSERT(vertex_indices.size() == mesh->vertex_indices.size());
    ASSERT(normal_indices.size() == mesh->normal_indices.size());
    ASSERT(texture_indices.size() == mesh->texture_indices.size());
    ASSERT(face_material.size() == mesh->face_material.size());
  }

  //vertices
  unsigned int size = vertices.size();
  unsigned int start = proc*size/numProcs;
  unsigned int end = (proc+1)*size/numProcs;
  for (unsigned int i=start; i < end; ++i) {
    vertices[i] = Vector(0,0,0);
    for(unsigned int frame=0; frame < keyframes.size(); ++frame) {
      vertices[i] += meshes[frame]->vertices[i] * keyframes[frame].t;
    }
  }

  //vertexNormals
  size = vertexNormals.size();
  start = proc*size/numProcs;
  end = (proc+1)*size/numProcs;
  for (unsigned int i=start; i < end; ++i) {
    vertexNormals[i] = Vector(0,0,0);
    for(unsigned int frame=0; frame < keyframes.size(); ++frame) {
      vertexNormals[i] += meshes[frame]->vertexNormals[i] * keyframes[frame].t;
    }
  }

  //vertexBinormals
  size = vertexBinormals.size();
  start = proc*size/numProcs;
  end = (proc+1)*size/numProcs;
  for (unsigned int i=start; i < end; ++i) {
    vertexBinormals[i] = Vector(0,0,0);
    for(unsigned int frame=0; frame < keyframes.size(); ++frame) {
      vertexBinormals[i] += meshes[frame]->vertexBinormals[i] * keyframes[frame].t;
    }
  }

  //vertexTangents
  size = vertexTangents.size();
  start = proc*size/numProcs;
  end = (proc+1)*size/numProcs;
  for (unsigned int i=start; i < end; ++i) {
    vertexTangents[i] = Vector(0,0,0);
    for(unsigned int frame=0; frame < keyframes.size(); ++frame) {
      vertexTangents[i] += meshes[frame]->vertexTangents[i] * keyframes[frame].t;
    }
  }

  //texCoords
  size = texCoords.size();
  start = proc*size/numProcs;
  end = (proc+1)*size/numProcs;
  for (unsigned int i=start; i < end; ++i) {
    texCoords[i] = Vector(0,0,0);
    for(unsigned int frame=0; frame < keyframes.size(); ++frame) {
      texCoords[i] += meshes[frame]->texCoords[i] * keyframes[frame].t;
    }
  }

  //materials
//   size = materials.size();
//   start = proc*size/numProcs;
//   end = (proc+1)*size/numProcs;
//   vector<keyframe_t> mat_keyframes(keyframes);
//   for (unsigned int i=start; i < end; ++i) {
//     for(unsigned int frame=0; frame < keyframes.size(); ++frame) {
//       mat_keyframes[frame].keyframe = meshes[frame]->materials[i];
//     }
//     InterpErr retcode = materials[i]->serialInterpolate(mat_keyframes);
//     if (retcode != success)
//       worstError = retcode;
//   }

  //there could still be some threads updating the vertices, which
  //get used when the triangles are updated below.
  barrier.wait(numProcs);

  Group::parallelInterpolate(keyframes, proc, numProcs);

  return worstError;
}


void Mesh::add(Object*)
{
  throw InternalError(string("Illegal call to ") + MANTA_FUNC);
}

void Mesh::remove(Object*, bool)
{
  throw InternalError(string("Illegal call to ") + MANTA_FUNC);
}

void Mesh::set( int, Object *)
{
  throw InternalError(string("Illegal call to ") + MANTA_FUNC);
}

void Mesh::addTriangle(MeshTriangle *meshTri)
{
  meshTri->attachMesh(this, objs.size());
  objs.push_back(meshTri);

}

void Mesh::computeBounds(const PreprocessContext& context,
                         int proc, int numProcs) const
{
  if (proc == 0) {
    this->bbox.reset();
  }

  BBox myBBox;

  //Compute Bounding boxes in parallel
  size_t size = vertices.size();
  size_t start = proc*size/numProcs;
  size_t end = (proc+1)*size/numProcs;
  for (size_t i=start; i < end; ++i) {
    myBBox.extendByPoint(vertices[i]);
  }

  //this barrier enforces that bbox has been initialized before
  //threads start writing to it.
  barrier.wait(numProcs);

  mutex.lock();
  this->bbox.extendByBox(myBBox);
  mutex.unlock();

  if (proc == 0) {
    dirtybbox = false;
  }

  //Need to wait for other threads to finish computing bbox
  barrier.wait(numProcs);
}

BBox Mesh::getBBox(unsigned int which)
{
  const Vector &p1 = getVertex(which, 0);
  const Vector &p2 = getVertex(which, 1);
  const Vector &p3 = getVertex(which, 2);
  BBox bbox(p1, p1);
  bbox.extendByPoint(p2);
  bbox.extendByPoint(p3);
  return bbox;
}

void Mesh::preprocess(const PreprocessContext& context)
{
  size_t start = context.proc*materials.size()/context.numProcs;
  size_t end = (context.proc+1)*materials.size()/context.numProcs;
  PreprocessContext serialContext = context;
  serialContext.proc = 0;
  serialContext.numProcs = 1;
  for (size_t i=start; i < end; ++i) {
    materials[i]->preprocess(serialContext);
  }

  if (context.proc == 0) {
    parallelSplit = objs.end(); // all triangles are serial
    setDirty();
  }

  start = context.proc*objs.size()/context.numProcs;
  end = (context.proc+1)*objs.size()/context.numProcs;
  for (size_t i=start; i < end; ++i) {
    objs[i]->preprocess(serialContext);
  }

  context.done();
}

void Mesh::addToUpdateGraph(ObjectUpdateGraph* graph,
                            ObjectUpdateGraphNode* parent) {
  // Insert myself underneath my parent
  /*ObjectUpdateGraphNode* node = */graph->insert(this, parent);
  // NOTE(boulos): Not currently adding the children (for
  // WaldTriangle this is incorrect).  Worse yet, without having
  // information even for KenslerShirley about which Primitives
  // have changed the BVH update won't be able to update only the
  // sections that changed.
}

void Mesh::interpolateNormals()
{
  //I think this should do the correct thing, but I might have messed
  //something with where and when I normalize vectors...

  //set all vertex normals to 0 without doing too much extra work.
  size_t oldSize = vertexNormals.size();
  vertexNormals.resize(vertices.size(), Vector(0,0,0));
  oldSize = min(oldSize, vertexNormals.size());
  for (size_t i = 0; i < oldSize; ++i) {
    vertexNormals[i] = Vector(0,0,0);
  }

  normal_indices.resize(vertex_indices.size());

  for (size_t i = 0; i < vertex_indices.size(); i+=3) {
    const unsigned int index0 = vertex_indices[i+0];
    const unsigned int index1 = vertex_indices[i+1];
    const unsigned int index2 = vertex_indices[i+2];

    normal_indices[i+0] = index0;
    normal_indices[i+1] = index1;
    normal_indices[i+2] = index2;

    const Vector& a = vertices[index0];
    const Vector& b = vertices[index1];
    const Vector& c = vertices[index2];
    const Vector n = Cross(b-a, c-a).normal();

    vertexNormals[index0] += n;
    vertexNormals[index1] += n;
    vertexNormals[index2] += n;
  }

  for (size_t i = 0; i < vertexNormals.size(); ++i) {
    vertexNormals[i] = vertexNormals[i].normal();
  }
}


void Mesh::removeDegenerateTriangles()
{
  for (size_t i=0; i < vertex_indices.size(); i+=3) {
    const unsigned int index0 = vertex_indices[i+0];
    const unsigned int index1 = vertex_indices[i+1];
    const unsigned int index2 = vertex_indices[i+2];

    const Vector& a = vertices[index0];
    const Vector& b = vertices[index1];
    const Vector& c = vertices[index2];

    //degenerate if triangle is collapsed onto a line or point.
    if ( Cross(a-b, c-b) == Vector::zero() ) {

      //this triangle is degenerate. Let's remove it.
      vertex_indices[i+0] = vertex_indices[vertex_indices.size()-3];
      vertex_indices[i+1] = vertex_indices[vertex_indices.size()-2];
      vertex_indices[i+2] = vertex_indices[vertex_indices.size()-1];
      vertex_indices.resize(vertex_indices.size()-3);

      if (!normal_indices.empty()) {
        normal_indices[i+0] = normal_indices[normal_indices.size()-3];
        normal_indices[i+1] = normal_indices[normal_indices.size()-2];
        normal_indices[i+2] = normal_indices[normal_indices.size()-1];
        normal_indices.resize(normal_indices.size()-3);
      }

      if (!texture_indices.empty()) {
        texture_indices[i+0] = texture_indices[texture_indices.size()-3];
        texture_indices[i+1] = texture_indices[texture_indices.size()-2];
        texture_indices[i+2] = texture_indices[texture_indices.size()-1];
        texture_indices.resize(texture_indices.size()-3);
      }

      const size_t tri_index = i/3;
      face_material[tri_index] = face_material.back();
      face_material.pop_back();

      MeshTriangle* tri = get(tri_index);
      tri->attachMesh(this, tri_index);
      shrinkTo(size()-1, false); //note this might cause a memory leak.

      i-=3;
    }
  }
}

void Mesh::scaleMesh(Real scalingFactor)
{
  if (dirtybbox) {
    PreprocessContext dummyContext;
    computeBounds(dummyContext, 0, 1);
  }

  Vector center = bbox.center();

  for (size_t i=0; i < vertices.size(); ++i)
    vertices[i] = scalingFactor*(vertices[i] - center);

  bbox[0] = scalingFactor*(bbox[0] - center);
  bbox[1] = scalingFactor*(bbox[1] - center);
}

namespace Manta {
  MANTA_REGISTER_CLASS(Mesh);
}

void Mesh::readwrite(ArchiveElement* archive) {
  MantaRTTI<Group>::readwrite(archive, *this);
  // First the data
  archive->readwrite("vertices", vertices);
  archive->readwrite("normals", vertexNormals);
  archive->readwrite("binormals", vertexBinormals);
  archive->readwrite("tangents", vertexTangents);
  archive->readwrite("texcoords", texCoords);
  archive->readwrite("materials", materials);
  // Now the indices
  archive->readwrite("vertex_indices", vertex_indices);
  archive->readwrite("normal_indices", normal_indices);
  archive->readwrite("binormal_indices", binormal_indices);
  archive->readwrite("tangent_indices", tangent_indices);
  archive->readwrite("texture_indices", texture_indices);
  archive->readwrite("face_material", face_material);
}

Mesh& Mesh::operator+=(const Mesh& otherMesh)
{
  // It doesn't make any sense to add a mesh to itself since all the triangles
  // will overlap.
  if (&otherMesh == this) return *this;

  size_t endIndex;

  endIndex = vertices.size();
  vertices.insert(vertices.end(),
                  otherMesh.vertices.begin(), otherMesh.vertices.end());
  vertex_indices.reserve(vertex_indices.size() + otherMesh.vertex_indices.size());
  for (size_t i=0; i < otherMesh.vertex_indices.size(); ++i)
    vertex_indices.push_back(otherMesh.vertex_indices[i]+endIndex);

  endIndex = vertexNormals.size();
  vertexNormals.insert(vertexNormals.end(),
                       otherMesh.vertexNormals.begin(), otherMesh.vertexNormals.end());
  normal_indices.reserve(normal_indices.size() + otherMesh.normal_indices.size());
  for (size_t i=0; i < otherMesh.normal_indices.size(); ++i)
    normal_indices.push_back(otherMesh.normal_indices[i]+endIndex);

  endIndex = vertexBinormals.size();
  vertexBinormals.insert(vertexBinormals.end(),
                         otherMesh.vertexBinormals.begin(), otherMesh.vertexBinormals.end());
  binormal_indices.reserve(binormal_indices.size() + otherMesh.binormal_indices.size());
  for (size_t i=0; i < otherMesh.binormal_indices.size(); ++i)
    binormal_indices.push_back(otherMesh.binormal_indices[i]+endIndex);

  endIndex = vertexTangents.size();
  vertexTangents.insert(vertexTangents.end(),
                        otherMesh.vertexTangents.begin(), otherMesh.vertexTangents.end());
  tangent_indices.reserve(tangent_indices.size() + otherMesh.tangent_indices.size());
  for (size_t i=0; i < otherMesh.tangent_indices.size(); ++i)
    tangent_indices.push_back(otherMesh.tangent_indices[i]+endIndex);

  endIndex = texCoords.size();
  texCoords.insert(texCoords.end(),
                   otherMesh.texCoords.begin(), otherMesh.texCoords.end());
  texture_indices.reserve(texture_indices.size() + otherMesh.texture_indices.size());
  for (size_t i=0; i < otherMesh.texture_indices.size(); ++i)
    texture_indices.push_back(otherMesh.texture_indices[i]+endIndex);

  endIndex = materials.size();
  // TODO: We should probably do a clone of the materials in case the otherMesh
  // deletes its materials.
  materials.insert(materials.end(),
                   otherMesh.materials.begin(), otherMesh.materials.end());
  face_material.reserve(face_material.size() + otherMesh.face_material.size());
  for (size_t i=0; i < otherMesh.face_material.size(); ++i)
    face_material.push_back(otherMesh.face_material[i]+endIndex);

  for (size_t i=0; i < otherMesh.size(); ++i) {
    // Make a copy of the triangles since we don't want to modify the source
    // triangles.  The cloning really does not modify the source triangle, but
    // since clone() is not a const method we need to do this
    // const_cast. Ugh. Ideally clone would be rewritten to be const.
    Clonable* copy = const_cast<MeshTriangle*>(otherMesh.get(i))->clone(shallow);
    addTriangle(dynamic_cast<MeshTriangle*>(copy));
  }

  setDirty();

  return *this;
}
