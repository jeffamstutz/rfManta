#include "Embree.h"

#include "Interface/RayPacket.h"
#include "Interface/Primitive.h"
#include "Interface/TexCoordMapper.h"
#include "Core/Color/Color.h"
#include "Core/Geometry/BBox.h"
#include "Model/Materials/Lambertian.h"
#include "Model/Groups/Mesh.h"
#include "Model/Groups/Group.h"

using namespace Manta;

Embree::Embree() :
  currMesh(0)
{
  /*no-op*/
}

Embree::~Embree()
{
  cleanup();
}

bool Embree::buildFromFile(const std::string &/*fileName*/)
{
  fprintf(stderr, "buildFromFile()\n");
  return false;
}

bool Embree::saveToFile(const string &/*fileName*/)
{
  fprintf(stderr, "saveToFile()\n");

  return false;
}

void Embree::intersect(const RenderContext& context, RayPacket& rays) const
{
  //fprintf(stderr, "intersect(): rays %i-%i\n", rays.begin(), rays.end());
}

void Embree::setGroup(Group* new_group)
{
  fprintf(stderr, "setGroup()\n");

  Mesh *mesh = dynamic_cast<Mesh*>(new_group);

  // Set the current mesh to the one we just got
  if(mesh)
    currMesh = mesh;
}

void Embree::groupDirty()
{
  fprintf(stderr, "groupDirty()\n");
  /*no op*/
}

Group* Embree::getGroup() const
{
  fprintf(stderr, "getGroup()\n");
  return currMesh;
}

void Embree::rebuild(int /*proc*/, int /*numProcs*/)
{
  fprintf(stderr, "rebuild()\n");
  /*no op*/
}

void Embree::addToUpdateGraph(ObjectUpdateGraph* /*graph*/,
                                ObjectUpdateGraphNode* /*parent*/)
{
  fprintf(stderr, "addToUpdateGraph()\n");
  /*no op*/
}

void Embree::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  fprintf(stderr, "computeBounds()\n");
  currMesh->computeBounds(context, bbox);
}

void Embree::preprocess(const PreprocessContext &context)
{
  fprintf(stderr, "preprocess()\n");

  currMesh->preprocess(context);

  Mesh *mesh = currMesh;

  // Check to see if we haven't loaded a graph cache
  if(mesh)
  {
    // Re-initialize rfut objects
    initialize();

    // Extract out triangle mesh data into temporary memory for rfut consumption
    //uint ntris = mesh->face_material.size();

    float* vertices = new float[3*mesh->vertices.size()];
    for(uint i = 0; i < mesh->vertices.size(); ++i)
    {
      vertices[3*i+0] = mesh->vertices[i][0];
      vertices[3*i+1] = mesh->vertices[i][1];
      vertices[3*i+2] = mesh->vertices[i][2];
    }

    uint* indices = new uint[mesh->vertex_indices.size()];
    for(uint i = 0; i < mesh->vertex_indices.size(); ++i)
      indices[i] = mesh->vertex_indices[i];

#if 0
    rfTriangleData* tridata = new rfTriangleData[ntris];
    for(uint i = 0; i < ntris; ++i)
    {
      tridata[i].triID = i;
      tridata[i].matID = mesh->face_material[i];
    }
#endif

    // Set the Embree model data using the extracted mesh data


    // Cleanup temporary memory
    delete vertices;
    delete indices;
    //delete tridata;
  }
}

void Embree::computeTexCoords2(const RenderContext &, RayPacket &) const
{
  fprintf(stderr, "computeTexCoords2()\n");
}

void Embree::computeTexCoords3(const RenderContext &, RayPacket &) const
{
  fprintf(stderr, "computeTexCoords3()\n");
}

void Embree::initialize()
{
  cleanup();
}

void Embree::cleanup()
{
}
