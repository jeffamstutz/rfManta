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
  inited(false),
  currMesh(0)
{
  /*no-op*/
}

Embree::~Embree()
{
  rtcDeleteScene(scene);
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

    ray.tnear = T_EPSILON;
    ray.tfar  = std::numeric_limits<float>::infinity();

    ray.geomID = -1;
    ray.primID = -1;

    ray.mask = 0xFFFFFFFF;
    ray.time = 0;

    // Trace ray
    rtcIntersect(scene, ray);

    if(ray.geomID != (int)RTC_INVALID_GEOMETRY_ID)
    {
      uint mid = currMesh->face_material[ray.primID];
      Material *material = currMesh->materials[mid];
      Primitive *primitive = (Primitive*)currMesh->get(ray.primID);
      rays.hit(i, ray.tfar, material, primitive, this);
      Vector normal(ray.Ng[0], ray.Ng[1], ray.Ng[2]);
      normal.normalize();
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
  if(mesh && !inited)
  {
    // Initialize Embree
    try
    {
      rtcInit();
    }
    catch(std::exception e)
    {
      fprintf(stderr, "catching init error: %s\n", e.what());
    }

    uint ntris = mesh->face_material.size();
    uint nverts = mesh->vertices.size();

    // Set the Embree model data using the extracted mesh data

    scene = rtcNewScene(RTC_SCENE_STATIC, RTC_INTERSECT1);
    uint ebMesh = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, ntris, nverts);

    Triangle* triangles =
            (Triangle*)rtcMapBuffer(scene, ebMesh, RTC_INDEX_BUFFER);
    Vertex*    verts    =
            (Vertex*)rtcMapBuffer(scene, ebMesh, RTC_VERTEX_BUFFER);

    for (uint i = 0; i < nverts; ++i)
    {
        verts[i][0] = mesh->vertices[i][0];
        verts[i][1] = mesh->vertices[i][1];
        verts[i][2] = mesh->vertices[i][2];
    }

    for (uint i = 0; i < ntris; ++i)
    {
        triangles[i][0] = mesh->vertex_indices[3*i+0];
        triangles[i][1] = mesh->vertex_indices[3*i+1];
        triangles[i][2] = mesh->vertex_indices[3*i+2];
    }

    rtcUnmapBuffer(scene, ebMesh, RTC_INDEX_BUFFER);
    rtcUnmapBuffer(scene, ebMesh, RTC_VERTEX_BUFFER);

    rtcCommit(scene);

    inited = true;
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
