#include "Rayforce.h"

#include <stdio.h>
#include <stdlib.h>

#include <rfut/CullMode.h>
#include <rfut/ModelType.h>
#include <rfut/Target.h>

#include <rfut/Buffer.t>
#include <rfut/Context.h>
#include <rfut/Device.t>
#include <rfut/Model.h>
#include <rfut/Object.h>
#include <rfut/Scene.t>
#include <rfut/TraceFcn.t>

#include "rfStruct.h"

#include "Interface/RayPacket.h"
#include "Interface/Primitive.h"
#include "Interface/TexCoordMapper.h"
#include "Core/Color/Color.h"
#include "Core/Geometry/BBox.h"
#include "Model/Materials/Lambertian.h"
#include "Model/Groups/Mesh.h"
#include "Model/Groups/Group.h"

using namespace Manta;

extern rfPipeline rfRays;

Rayforce::Rayforce() :
  context(0),
  object(0),
  model(0),
  scene(0),
  device(0),
  traceFcn(0),
  saveToFileName(""),
  currMesh(0)
{
  /*no-op*/
}

Rayforce::~Rayforce()
{
  cleanup();
}

bool Rayforce::buildFromFile(const std::string &fileName)
{
  fprintf(stderr, "buildFromFile()\n");

  initialize();

  model = new rfut::Model(*scene, ModelType::Triangles, 255, 255);
  model->setData(fileName);

  object->attach(*model);
  scene->acquire();

  traceFcn = new rfut::TraceFcn<Target::System>(*scene, rfRays);

  // Finished loading the graph cache, return success
  return true;
}

bool Rayforce::saveToFile(const string &fileName)
{
  fprintf(stderr, "saveToFile()\n");

  saveToFileName = fileName;

  return true;
}

void Rayforce::intersect(const RenderContext& context, RayPacket& rays) const
{
  //fprintf(stderr, "intersect(): rays %i-%i\n", rays.begin(), rays.end());

  rfRaySingle ray;
  rfRaySingleInit(&ray);

  //NOTE: ray.skipdist is not implemented to be used by Rayforce
  //ray.skipdist = T_EPSILON;
  //ray.clipdist = std::numeric_limits<float>::infinity();

  if(rays.getFlag(RayPacket::ConstantOrigin))
  {
    Manta::Ray mray = rays.getRay(rays.begin());
    ray.origin[0] = mray.origin()[0];
    ray.origin[1] = mray.origin()[1];
    ray.origin[2] = mray.origin()[2];
    ray.root = scene->resolve(ray.origin);
  }

  for(int i = rays.begin(); i < rays.end(); ++i)
  {
    Manta::Ray mray = rays.getRay(i);

    if(!rays.getFlag(RayPacket::ConstantOrigin))
    {
      ray.origin[0] = mray.origin()[0];
      ray.origin[1] = mray.origin()[1];
      ray.origin[2] = mray.origin()[2];
      ray.root = scene->resolve(ray.origin);
    }

    ray.vector[0] = mray.direction()[0];
    ray.vector[1] = mray.direction()[1];
    ray.vector[2] = mray.direction()[2];

    rfRayData rayData;
    rayData.skipdist = T_EPSILON;
    rayData.hit      = 0;

    (*traceFcn)(ray, rayData);

    if(rayData.hit)
    {
      Material *material = currMesh->materials[rayData.matID];
      Primitive *primitive = (Primitive*)currMesh->get(rayData.triID);
      rays.hit(i, rayData.minT, material, primitive, this);
      Vector normal(rayData.normal[0], rayData.normal[1], rayData.normal[2]);
      normal.normalize();
      rays.setNormal(i, normal);
    }
  }

  rays.setFlag(RayPacket::HaveNormals);
}

void Rayforce::setGroup(Group* new_group)
{
  fprintf(stderr, "setGroup()\n");

  Mesh *mesh = dynamic_cast<Mesh*>(new_group);

  // Set the current mesh to the one we just got
  if(mesh)
    currMesh = mesh;
}

void Rayforce::groupDirty()
{
  fprintf(stderr, "groupDirty()\n");
  /*no op*/
}

Group* Rayforce::getGroup() const
{
  fprintf(stderr, "getGroup()\n");
  return currMesh;
}

void Rayforce::rebuild(int /*proc*/, int /*numProcs*/)
{
  fprintf(stderr, "rebuild()\n");
  /*no op*/
}

void Rayforce::addToUpdateGraph(ObjectUpdateGraph* /*graph*/,
                                ObjectUpdateGraphNode* /*parent*/)
{
  fprintf(stderr, "addToUpdateGraph()\n");
  /*no op*/
}

void Rayforce::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  fprintf(stderr, "computeBounds()\n");
  currMesh->computeBounds(context, bbox);
}

void Rayforce::preprocess(const PreprocessContext &context)
{
  fprintf(stderr, "preprocess()\n");

  currMesh->preprocess(context);

  Mesh *mesh = currMesh;

  // Check to see if we haven't loaded a graph cache
  if(mesh && !model)
  {
    // Re-initialize rfut objects
    initialize();

    // Extract out triangle mesh data into temporary memory for rfut consumption
    uint ntris = mesh->face_material.size();

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

    rfTriangleData* tridata = new rfTriangleData[ntris];
    for(uint i = 0; i < ntris; ++i)
    {
      tridata[i].triID = i;
      tridata[i].matID = mesh->face_material[i];
    }

    // Set the rfut::Model data using the extracted mesh data
    model = new rfut::Model(*scene, ModelType::Triangles, 255, 255);
    model->setData(ntris,
                   vertices,
                   indices,
                   sizeof(rfTriangleData),
                   sizeof(rfTriangleData),
                   tridata);

    object->attach(*model);
    scene->acquire();

    // Cleanup temporary memory
    delete vertices;
    delete indices;
    delete tridata;

    // Allocate the trace function
    traceFcn = new rfut::TraceFcn<Target::System>(*scene, rfRays);

    // Save the cache file out if we got a filename from saveToFile()
    if(!saveToFileName.empty())
      model->saveCacheFile(saveToFileName);
  }
}

void Rayforce::computeTexCoords2(const RenderContext &, RayPacket &) const
{
  fprintf(stderr, "computeTexCoords2()\n");
}

void Rayforce::computeTexCoords3(const RenderContext &, RayPacket &) const
{
  fprintf(stderr, "computeTexCoords3()\n");
}

void Rayforce::initialize()
{
  cleanup();

  context = new rfut::Context;
  device  = new rfut::Device<Target::System>(*context, 0);
  scene   = new rfut::Scene<Target::System>(*context, *device);
  object  = new rfut::Object(*scene, CullMode::None);
}

void Rayforce::cleanup()
{
  delete traceFcn;
  delete device;

  // NOTE(cpg) - the order of object destruction matters to Rayforce, at
  //             least in so far as rfObjects are concerned; from Alexis:
  //
  //   Did you already free the scene that contains the object? That would
  //   destroy the object too. Or if you destroyed the context that contains
  //   the scene.
  //
  //             we'll have to think carefully about how to ensure rfut
  //             behaves properly if client-side code doesn't observe
  //             the proper sequence...
  //
  //             to fix the issue here, release the instance of rfut::Scene
  //             after releasing the instance of rfut::Object

  delete model;
  delete object;
  delete scene;
  delete context;
}
