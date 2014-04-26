#include "RFGraph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <float.h>
#include <unistd.h>

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

#include "rfdefs.h"
#include "rfgraph.h"
#include "rfmath.h"

// Macros from path.h /////////////////////////////////////////////////////////

#define RF_ADDRBITS 64

#if RF_ADDRBITS == 16
 #define RF_SECTOR(x) ((const rfSector16 * RF_RESTRICT)x)
 #define RF_NODE(x) ((const rfNode16 * RF_RESTRICT)x)
 #define RF_LINKSHIFT (RF_GRAPH_LINK16_SHIFT)
 #define RF_TRILIST RF_SECTOR16_TRILIST
 #define RF_TRILIST_TYPE int16_t
#elif RF_ADDRBITS == 32
 #define RF_SECTOR(x) ((const rfSector32 * RF_RESTRICT)x)
 #define RF_NODE(x) ((const rfNode32 * RF_RESTRICT)x)
 #define RF_LINKSHIFT (RF_GRAPH_LINK32_SHIFT)
 #define RF_TRILIST RF_SECTOR32_TRILIST
 #define RF_TRILIST_TYPE int32_t
#else
 #define RF_SECTOR(x) ((const rfSector64 * RF_RESTRICT)x)
 #define RF_NODE(x) ((const rfNode64 * RF_RESTRICT)x)
 #define RF_LINKSHIFT (RF_GRAPH_LINK64_SHIFT)
 #define RF_TRILIST RF_SECTOR64_TRILIST
 #define RF_TRILIST_TYPE int64_t
#endif

#define RF_ELEM_PREPARE_AXIS(axis) \
  edgeindex[axis] = ( axis << RF_EDGE_AXIS_SHIFT ) | RF_EDGE_MIN; \
  vectinv[axis] = -RFF_MAX; \
  if( rffabs( vector[axis] ) > (rff)0.0 ) \
  { \
    vectinv[axis] = (rff)1.0 / vector[axis]; \
    if( vector[axis] >= (rff)0.0 ) \
      edgeindex[axis] = ( axis << RF_EDGE_AXIS_SHIFT ) | RF_EDGE_MAX; \
  }

///////////////////////////////////////////////////////////////////////////////

#include "resolve.h"

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

  handle = scene->m_handle;
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

void RFGraph::intersect(const RenderContext& /*context*/, RayPacket& rays) const
{
  //fprintf(stderr, "intersect()\n");

  for(int i = rays.begin(); i < rays.end(); ++i)
  {
    Manta::Ray ray = rays.getRay(rays.begin());

    float origin[3]; //original origin (for calculating hitdist at the end of
                     //                 triangle intersection)

    // Begin code imported from path.h //

    int nredge;
    rfssize slink;
    rff vector[3];
    rff RF_ALIGN16 vectinv[4];
    int edgeindex[3];
    rff src[3], dst[3], dist, mindist;
    rff hitdist;
    void *root;
    rfTri *trihit;
    int axisindex;

    vector[0] = ray.direction()[0];
    vector[1] = ray.direction()[1];
    vector[2] = ray.direction()[2];

    origin[0] = src[0] = ray.origin()[0];
    origin[1] = src[1] = ray.origin()[1];
    origin[2] = src[2] = ray.origin()[2];

    root = resolve(handle->objectgraph, origin);

    RF_ELEM_PREPARE_AXIS(0);
    RF_ELEM_PREPARE_AXIS(1);
    RF_ELEM_PREPARE_AXIS(2);

    for( ; ; )
    {
      /* Sector traversal */
      int tricount;

      // [info]
      // Ray box intersection to determine endpoint of the line segment to
      // intersect with triangles. (faster than raw ray intersection, we think)
      nredge = edgeindex[0];
      mindist = (RF_SECTOR(root)->edge[edgeindex[0]] - src[0]) * vectinv[0];
      dist = (RF_SECTOR(root)->edge[edgeindex[1]] - src[1]) * vectinv[1];
      if( dist < mindist )
      {
        nredge = edgeindex[1];
        mindist = dist;
      }
      dist = (RF_SECTOR(root)->edge[edgeindex[2]] - src[2]) * vectinv[2];
      if( dist < mindist )
      {
        nredge = edgeindex[2];
        mindist = dist;
      }
      // [/info]

      tricount = RF_SECTOR_GET_PRIMCOUNT(RF_SECTOR(root));
      if( tricount )
      {
        RF_TRILIST_TYPE *trilist;// addressing type

        // [info]
        // Calculating the endpoint using the entry (resolved origin)
        // and ray info
        dst[0] = src[0] + ( mindist * vector[0] );
        dst[1] = src[1] + ( mindist * vector[1] );
        dst[2] = src[2] + ( mindist * vector[2] );
        // [/info]

        // [info]
        // Triangle intersection!
        trihit = 0;
        trilist = RF_TRILIST( RF_SECTOR(root) );
        do
        {
          rff dstdist, srcdist, uv[2], f, vray[3];
          rfTri *tri;

          tri = (rfTri *)RF_ADDRESS(root,
                                    (rfssize)(*trilist++) << RF_LINKSHIFT);

          // Intersect the triangle, with no back/front face culling
          dstdist = rfMathPlanePoint( tri->plane, dst );
          srcdist = rfMathPlanePoint( tri->plane, src );
          if( dstdist * srcdist > (rff)0.0 )
            continue;
          f = srcdist / ( srcdist - dstdist );
          vray[0] = src[0] + f * ( dst[0] - src[0] );
          vray[1] = src[1] + f * ( dst[1] - src[1] );
          vray[2] = src[2] + f * ( dst[2] - src[2] );
          uv[0] = ( rfMathVectorDotProduct( &tri->edpu[0], vray ) + tri->edpu[3] );
          if( !( uv[0] >= (rff)0.0 ) || ( uv[0] > (rff)1.0 ) )
            continue;
          uv[1] = ( rfMathVectorDotProduct( &tri->edpv[0], vray ) + tri->edpv[3] );
          if( ( uv[1] < (rff)0.0 ) || ( ( uv[0] + uv[1] ) > (rff)1.0 ) )
            continue;
          dst[0] = vray[0];
          dst[1] = vray[1];
          dst[2] = vray[2];

          trihit = tri;
        } while( --tricount );

        axisindex = nredge >> 1;
        hitdist = (dst[axisindex] - origin[axisindex]) * vectinv[axisindex];

        if( trihit && hitdist > T_EPSILON )
          break;
      }
      // [/info]

      // [info]
      // If the neiboring node is a sector, just go straight to it and move on
      /* Traverse through the sector's edge */
      if(RF_SECTOR(root)->flags &
         ((RF_LINK_SECTOR<<RF_SECTOR_LINKFLAGS_SHIFT) << nredge))
      {
        /* Neighbor is sector */
        slink = (rfssize)RF_SECTOR(root)->link[nredge];
        if(!(slink))
          break;//CHECKME:-->may be incorrect? was 'goto tracevoid;'
        root = RF_ADDRESS(root, slink << RF_LINKSHIFT);
        continue;
      }
      // [/info]

      // [info]
      // We have to do some kind of disambiguation of the neighbors to figure
      // out what sector we need to traverse to next.
      /* Neighbor is node */
      src[0] += mindist * vector[0];
      src[1] += mindist * vector[1];
      src[2] += mindist * vector[2];
      root = RF_ADDRESS(root,
                        (rfssize)RF_SECTOR(root)->link[nredge] << RF_LINKSHIFT);
      // [/info]

      /* Node traversal */
      for( ; ; )
      {

        int linkflags;
        linkflags = RF_NODE(root)->flags;//--> pull out the current traversal
                                         //    node
                                         //...links to other nodes or a sector
        // [info]
        // Figure out which half-space we are traversing through the
        // diambiguation plane.
        if( src[RF_NODE_GET_AXIS(linkflags)] < RF_NODE(root)->plane )
        {
          root = RF_ADDRESS(root,
                            (rfssize)RF_NODE(root)
                            ->link[RF_NODE_LESS] << RF_LINKSHIFT);
          if( linkflags & ((RF_LINK_SECTOR<<RF_NODE_LINKFLAGS_SHIFT)
                           << RF_NODE_LESS))
            break;
        }
        else
        {
          root = RF_ADDRESS(root,
                            (rfssize)RF_NODE(root)->link[RF_NODE_MORE]
                            << RF_LINKSHIFT);
          if( linkflags & ((RF_LINK_SECTOR<<RF_NODE_LINKFLAGS_SHIFT)
                           << RF_NODE_MORE))
            break;
        }
        // [/info]
      }
    }

    // We have a triangle intersection, go ahead and populate the ray in the
    // RayPacket accordingly
    if(trihit)
    {
      rfTriangleData* data =
              (rfTriangleData*)(RF_ADDRESS(trihit, sizeof(rfTri)));

      Material *material   = currMesh->materials[data->matID];
      Primitive *primitive = (Primitive*)currMesh->get(data->triID);
      rays.hit(i, hitdist, material, primitive, this);
      Vector normal(trihit->plane[0], trihit->plane[1], trihit->plane[2]);
      normal.normalize();
      rays.setNormal(i, normal);
    }
  }

  rays.setFlag(RayPacket::HaveNormals);
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

    handle = scene->m_handle;

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
