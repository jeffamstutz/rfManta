#include "RFGraph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __unix__
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <sys/mman.h>
 #include <unistd.h>
#endif

#include <rfut/CullMode.h>
#include <rfut/ModelType.h>
#include <rfut/Target.h>

#include <rfut/Buffer.t>
#include <rfut/Context.h>
#include <rfut/Device.t>
#include <rfut/Model.h>
#include <rfut/Object.h>
#include <rfut/Scene.t>

#include "Interface/RayPacket.h"
#include "Interface/Primitive.h"
#include "Interface/TexCoordMapper.h"
#include "Core/Color/Color.h"
#include "Core/Geometry/BBox.h"
#include "Model/Materials/Lambertian.h"
#include "Model/Groups/Mesh.h"
#include "Model/Groups/Group.h"

using namespace Manta;

// Per-triangle data
typedef struct
{
  size_t triID;
  int    matID;
} rfTriangleData;


RFGraph::RFGraph() :
  //graph(0)
  context(0),
  object(0),
  model(0),
  scene(0),
  device(0),
  saveToFileName(""),
  currMesh(0)

{
  /*no-op*/
}

RFGraph::~RFGraph()
{
  // Free the graph if it exists
#if 0
  if(graph)
    free(graph);
#else
  cleanup();
#endif
}

bool RFGraph::buildFromFile(const std::string &fileName)
{
  fprintf(stderr, "buildFromFile()\n");
#if 0
  /////////////////////////////////////////////////////////////////////////////
  // Code imported from Rayforce //////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  // Load the Rayforce graph cache file ///////////////////////////////////////

  void *cache;
  size_t cachesize;
#ifdef __unix__
  int fd;
  struct stat filestat;
#else
  FILE *file;
#endif

#ifdef __unix__
  if((fd = open(fileName.c_str(), O_RDONLY)) == -1)
    return false;
  if(fstat(fd, &filestat))
  {
    close( fd );
    return false;
  }
  cachesize = filestat.st_size;
  if((cache = mmap( 0, cachesize, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
  {
    close( fd );
    return false;
  }
#else
  if( !( file = fopen( filename, "r" ) ) )
    return 0;
  fseek( file, 0, SEEK_END );
  cachesize = ftell( file );
  fseek( file, 0, SEEK_SET );
  if( !( cache = malloc( cachesize ) ) )
  {
    fclose( file );
    return 0;
  }
  cachesize = fread( cache, 1, cachesize, file );
#endif

  // Allocate the graph and copy from the loaded cache file ///////////////////

  this->graph = malloc(cachesize);
  memcpy(this->graph, cache, cachesize);

#ifdef __unix__
  munmap( cache, cachesize );
  close( fd );
#else
  free( cache );
  fclose( file );
#endif
#else
  initialize();

  model = new rfut::Model(*scene, ModelType::Triangles, 255, 255);
  model->setData(fileName);

  object->attach(*model);
  scene->acquire();
#endif

  // Finished loading the graph cache, return success
  return true;
}

bool RFGraph::saveToFile(const string &fileName)
{
  fprintf(stderr, "saveToFile()\n");

  saveToFileName = fileName;

  return true;
}

void RFGraph::intersect(const RenderContext& /*context*/, RayPacket& /*rays*/) const
{
  fprintf(stderr, "intersect()\n");
  //TODO: intersect rays
}

void RFGraph::setGroup(Group* new_group)
{
  fprintf(stderr, "setGroup()\n");

  Mesh *mesh = dynamic_cast<Mesh*>(new_group);

  // Set the current mesh to the one we just got
  if(mesh)
    currMesh = mesh;
}

void RFGraph::groupDirty()
{
  fprintf(stderr, "groupDirty()\n");
  /*no op*/
}

Group* RFGraph::getGroup() const
{
  fprintf(stderr, "getGroup()\n");
  return currMesh;
}

void RFGraph::rebuild(int /*proc*/, int /*numProcs*/)
{
  fprintf(stderr, "rebuild()\n");
  /*no op*/
}

void RFGraph::addToUpdateGraph(ObjectUpdateGraph* /*graph*/,
                                ObjectUpdateGraphNode* /*parent*/)
{
  fprintf(stderr, "addToUpdateGraph()\n");
  /*no op*/
}

void RFGraph::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  fprintf(stderr, "computeBounds()\n");
  currMesh->computeBounds(context, bbox);
}

void RFGraph::preprocess(const PreprocessContext &context)
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

    // Save the cache file out if we got a filename from saveToFile()
    if(!saveToFileName.empty())
      model->saveCacheFile(saveToFileName);
  }
}

void RFGraph::computeTexCoords2(const RenderContext &, RayPacket &) const
{
  fprintf(stderr, "computeTexCoords2()\n");
}

void RFGraph::computeTexCoords3(const RenderContext &, RayPacket &) const
{
  fprintf(stderr, "computeTexCoords3()\n");
}

void RFGraph::initialize()
{
  cleanup();

  context = new rfut::Context;
  device  = new rfut::Device<Target::System>(*context, 0);
  scene   = new rfut::Scene<Target::System>(*context, *device);
  object  = new rfut::Object(*scene, CullMode::None);
}

void RFGraph::cleanup()
{
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
