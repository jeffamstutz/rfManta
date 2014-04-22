#include "Embree.h"

#include "embree2/rtcore.h"
#include "embree2/rtcore_ray.h"

#include "Interface/RayPacket.h"
#include "Interface/Primitive.h"
#include "Interface/TexCoordMapper.h"
#include "Core/Color/Color.h"
#include "Core/Geometry/BBox.h"
#include "Model/Materials/Lambertian.h"
#include "Model/Groups/Mesh.h"
#include "Model/Groups/Group.h"

#include <stdio.h>

using namespace Manta;

typedef int   Triangle[3];
typedef float Vertex[4];

//XXX: this is local to this translation unit to keep the build system simple...
RTCScene scene;

Embree::Embree() :
  currMesh(0)
{
  /*no-op*/
}

Embree::~Embree()
{
  rtcDeleteScene(scene);
  delete scene;
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

void Embree::intersect(const RenderContext&/*context*/, RayPacket& rays) const
{
  //fprintf(stderr, "intersect(): rays %i-%i\n", rays.begin(), rays.end());
  RTCRay ray;

  if(rays.getFlag(RayPacket::ConstantOrigin))
  {
    Manta::Ray mray = rays.getRay(rays.begin());
    ray.org[0] = mray.origin()[0];
    ray.org[1] = mray.origin()[1];
    ray.org[2] = mray.origin()[2];
  }

  for(int i = rays.begin(); i < rays.end(); ++i)
  {
    Manta::Ray mray = rays.getRay(i);

    if(!rays.getFlag(RayPacket::ConstantOrigin))
    {
      ray.org[0] = mray.origin()[0];
      ray.org[1] = mray.origin()[1];
      ray.org[2] = mray.origin()[2];
    }

    ray.dir[0] = mray.direction()[0];
    ray.dir[1] = mray.direction()[1];
    ray.dir[2] = mray.direction()[2];

    ray.tnear = 0.0f;
    ray.tfar  = std::numeric_limits<float>::infinity();

    ray.geomID = -1;
    ray.primID = -1;

    ray.mask = 0xFFFFFFFF;
    ray.time = 0;

    // Trace ray
    rtcIntersect(scene, ray);

    if(ray.geomID != (int)RTC_INVALID_GEOMETRY_ID)
    {
      Material *material = 0;//currMesh->materials[rayData.matID];
      Primitive *primitive = 0;//(Primitive*)currMesh->get(rayData.triID);
      rays.hit(i, ray.tfar, material, primitive, this);
      Vector normal(ray.Ng[0], ray.Ng[1], ray.Ng[2]);
      rays.setNormal(i, normal);
    }
  }

  rays.setFlag(RayPacket::HaveNormals);
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
    uint ntris = mesh->face_material.size();
    uint nverts = mesh->vertices.size();

    float* vertices = new float[3*mesh->vertices.size()];
    for(uint i = 0; i < nverts; ++i)
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

    scene = rtcNewScene(RTC_SCENE_STATIC, RTC_INTERSECT8);
    uint mesh = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, ntris, nverts);

    Triangle* triangles = (Triangle*)rtcMapBuffer(scene,mesh,RTC_INDEX_BUFFER);
    Vertex*    verts    = (Vertex*)rtcMapBuffer(scene,mesh,RTC_VERTEX_BUFFER);

    for (uint i = 0; i < nverts; ++i)
    {
        verts[i][0] = vertices[3*i+0];
        verts[i][1] = vertices[3*i+1];
        verts[i][2] = vertices[3*i+2];
    }

    for (uint i = 0; i < ntris; ++i)
    {
        triangles[i][0] = indices[3*i+0];
        triangles[i][1] = indices[3*i+1];
        triangles[i][2] = indices[3*i+2];
    }

    rtcUnmapBuffer(scene, mesh, RTC_INDEX_BUFFER);
    rtcUnmapBuffer(scene, mesh, RTC_VERTEX_BUFFER);

    rtcCommit(scene);


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
  //TODO: add initialization code
  try
  {
      rtcInit();
  }
  catch(std::exception e)
  {
      fprintf(stderr, "catching init error: %s\n", e.what());
  }

}
