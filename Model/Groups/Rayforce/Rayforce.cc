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
#include "Model/Materials/Lambertian.h"
#include "Core/Color/Color.h"
#include "Core/Geometry/BBox.h"

using namespace Manta;

extern rfPipeline rfRays;

Rayforce::Rayforce() :
  context(0),
  object(0),
  model(0),
  scene(0),
  device(0),
  traceFcn(0),
  inited(false),
  material(new Lambertian(Color::white()))
{
  /*no-op*/
}

Rayforce::~Rayforce()
{
  cleanup();

  delete material;
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

  inited = true;

  // Finished loading the graph cache, return success
  return true;
}

void Rayforce::intersect(const RenderContext& context, RayPacket& rays) const
{
  //fprintf(stderr, "intersect()\n");

  for(int i = rays.begin(); i < rays.end(); ++i)
  {
    rfRaySingle ray;

    ray.origin[0] = rays.getRay(i).origin().x();
    ray.origin[1] = rays.getRay(i).origin().y();
    ray.origin[2] = rays.getRay(i).origin().z();

    ray.vector[0] = rays.getRay(i).direction().x();
    ray.vector[1] = rays.getRay(i).direction().y();
    ray.vector[2] = rays.getRay(i).direction().z();

    ray.root = scene->resolve(ray.origin);

    rfRayData rayData;

    (*traceFcn)(ray, rayData);

    if(rayData.hit)
    {
      rays.hit(i, rayData.hit, material, 0, 0);
      //rays.overrideMinT(i, rayData.minT);
      //rays.setHitMaterial(i, material);
      //rays.setHitPosition(i, Vector(rayData.pos[0],
      //                              rayData.pos[1],
      //                              rayData.pos[2]));
    }
  }
}

void Rayforce::setGroup(Group* /*new_group*/)
{
  fprintf(stderr, "setGroup()\n");
  /*no op*/
}

void Rayforce::groupDirty()
{
  fprintf(stderr, "groupDirty()\n");
  /*no op*/
}

Group* Rayforce::getGroup() const
{
  fprintf(stderr, "getGroup()\n");
  return NULL;
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
  float box[6];
  rfGetBoundingBoxf(scene->getRawPtr(), box);
  Vector min(box[0], box[2], box[4]);
  Vector max(box[1], box[3], box[5]);
  bbox = BBox(min, max);
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

  inited = false;
}
