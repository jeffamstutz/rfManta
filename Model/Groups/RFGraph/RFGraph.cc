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

#define RF_ADDRBITS 32

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

//Uncomment for debug output during ray traversal
//#define DEBUG_OUTPUT

//Uncomment to force tracing with scalar ray traversal
//#define TRAVERSE_SINGLE_RAY


#define RF_PACKETOFFSET (0)
#define RF_PACKETWIDTH  (4)

using namespace Manta;

// Per-triangle data
typedef struct
{
  size_t triID;
  int    matID;
} rfTriangleData;


RFGraph::RFGraph() :
  graph(0),
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
  if(graph)
    free(graph);

  cleanup();
}

bool RFGraph::buildFromFile(const std::string &fileName)
{
  fprintf(stderr, "buildFromFile()\n");

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
  void *roots[4];
  float origin[3];

  bool cOrigin = rays.getFlag(RayPacket::ConstantOrigin);

  const int raybegin = rays.begin();
  const int rayend   = rays.end();

  if (cOrigin)
  {
    Manta::Ray ray = rays.getRay(raybegin);
    origin[0] = ray.origin()[0];
    origin[1] = ray.origin()[1];
    origin[2] = ray.origin()[2];
    roots[0] = roots[1] = roots[2] = roots[3] = resolve(graph, origin);
  }

#ifdef TRAVERSE_SINGLE_RAY
  for (int i = raybegin; i < rayend; ++i)
  {
    if (!cOrigin)
    {
      Manta::Ray ray = rays.getRay(i);
      origin[0] = ray.origin()[0];
      origin[1] = ray.origin()[1];
      origin[2] = ray.origin()[2];
      roots[0] = resolve(graph, origin);
    }

    intersectSingle(rays, i, roots[0]);
  }
#else
  // Do we have less than 4 rays to be traced?
  if (rayend - (raybegin-1) < 4)
  {
    // Yes, so trace them all as single rays
    for (int i = raybegin; i < rayend; ++i)
    {
      if (!cOrigin)
      {
        Manta::Ray ray = rays.getRay(i);
        origin[0] = ray.origin()[0];
        origin[1] = ray.origin()[1];
        origin[2] = ray.origin()[2];
        roots[0] = resolve(graph, origin);
      }

      intersectSingle(rays, i, roots[0]);
    }
  }
  else
  {
    // We have more than 4 rays, create sets of packets and trace them

    int i = raybegin;
    const int sse_begin = (i + 3) & (~3);

    // Trace any rays that are before the first SSE aligned ray
    while (i < sse_begin)
    {
      if (!cOrigin)
      {
        Manta::Ray ray = rays.getRay(i);
        origin[0] = ray.origin()[0];
        origin[1] = ray.origin()[1];
        origin[2] = ray.origin()[2];
        roots[0] = resolve(graph, origin);
      }

      intersectSingle(rays, i, roots[0]);
      ++i;
    }

    // Start with the first SSE aligned ray and trace packets
    for (; i+4 < rayend; i += 4)
    {
      if (!cOrigin)
      {
        for (int j = 0; j < 4; ++j)
        {
          Manta::Ray ray = rays.getRay(i);
          origin[0] = ray.origin()[0];
          origin[1] = ray.origin()[1];
          origin[2] = ray.origin()[2];
          roots[j] = resolve(graph, origin);
        }
      }

      RayPacket subpacket(rays, i, i+4);
      intersectSSE(subpacket, roots);
    }

    // Trace any lingering rays at the end of the parent packet
    while (i < rayend)
    {
      if (!cOrigin)
      {
        Manta::Ray ray = rays.getRay(i);
        origin[0] = ray.origin()[0];
        origin[1] = ray.origin()[1];
        origin[2] = ray.origin()[2];
        roots[0] = resolve(graph, origin);
      }

      intersectSingle(rays, i, roots[0]);
      ++i;
    }
  }
#endif // TRAVERSE_SINGLE_RAY

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
  if(mesh && !graph)
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

    graph = handle->objectgraph;
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

void RFGraph::intersectSingle(RayPacket &rays, int which, void *root) const
{
  int nredge;
  rfssize slink;
  rff vector[3];
  rff RF_ALIGN16 vectinv[4];
  int edgeindex[3];
  rff src[3], dst[3], dist, mindist;
  rff hitdist = 0.f;
  rfTri *trihit;
  int axisindex;

  float origin[3]; //original origin (for calculating hitdist at the end of
                   //                 triangle intersection)

  Manta::Ray ray = rays.getRay(which);

  vector[0] = ray.direction()[0];
  vector[1] = ray.direction()[1];
  vector[2] = ray.direction()[2];

  origin[0] = ray.origin()[0];
  origin[1] = ray.origin()[1];
  origin[2] = ray.origin()[2];

  root = resolve(graph, origin);

  // Offset src by T_EPSILON to avoid self-shading
  src[0] = origin[0] + vector[0] * T_EPSILON;
  src[1] = origin[1] + vector[1] * T_EPSILON;
  src[2] = origin[2] + vector[2] * T_EPSILON;

  RF_ELEM_PREPARE_AXIS(0);
  RF_ELEM_PREPARE_AXIS(1);
  RF_ELEM_PREPARE_AXIS(2);

  for( ; ; )
  {
#ifdef DEBUG_OUTPUT
      fprintf(stderr, "traversing sector\n");
#endif
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
    trihit = 0;
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

#ifdef DEBUG_OUTPUT
        fprintf(stderr, "hit triangle: %p\n", trihit);
#endif

      if(trihit) break;
    }
#ifdef DEBUG_OUTPUT
      else
          fprintf(stderr, "no triangles...\n");
#endif
    // [/info]

    // [info]
    // If the neiboring node is a sector, just go straight to it and move on
    /* Traverse through the sector's edge */
    if(RF_SECTOR(root)->flags &
       ((RF_LINK_SECTOR<<RF_SECTOR_LINKFLAGS_SHIFT) << nredge))
    {
#ifdef DEBUG_OUTPUT
        fprintf(stderr, "neighbor is a sector\n");
#endif
      /* Neighbor is sector */
      slink = (rfssize)RF_SECTOR(root)->link[nredge];
      if(!(slink))
      {
#ifdef DEBUG_OUTPUT
          fprintf(stderr, "goto tracevoid\n");
#endif
        break;//CHECKME:-->may be incorrect? was 'goto tracevoid;'
      }
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
#ifdef DEBUG_OUTPUT
        fprintf(stderr, "node traversal\n");
#endif

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
    rays.hit(which, hitdist - T_EPSILON, material, primitive, this);
    Vector normal(trihit->plane[0], trihit->plane[1], trihit->plane[2]);
    normal.normalize();
    rays.setNormal(which, normal);
  }
}

void RFGraph::intersectSSE(RayPacket& rays, void *roots[4]) const
{
  int32_t nredge;
  int32_t donemask;
  rfssize slink;
  int32_t raymask, raymaskinv, vecflag;
  __m128 vsrc[3], vdst[3];
  __m128 vunit;
  __m128 vector[3];
  __m128 vectinv[3];
  __m128 dstdist, srcdist, vsum;
  __m128 vray[3], utd, vtd;
  __m128 pl0, pl1, pl2, pl3;
#if RF_CPUSLOW_SHR
  static const int32_t maskcount[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
#endif
  int32_t edgeindex[4];
  int32_t edgevmask[3];
  void *root;
  int32_t axisindex;
  __m128 vhitdist;
 #define F 0x0
 #define T 0xffffffff
  static const uint32_t ortable[16*4] RF_ALIGN16 =
  {
    F,F,F,F,
    T,F,F,F,
    F,T,F,F,
    T,T,F,F,
    F,F,T,F,
    T,F,T,F,
    F,T,T,F,
    T,T,T,F,
    F,F,F,T,
    T,F,F,T,
    F,T,F,T,
    T,T,F,T,
    F,F,T,T,
    T,F,T,T,
    F,T,T,T,
    T,T,T,T,
  };
 #undef F
 #undef T

  rfResult4 result;
 #define RF_RESULT (&result)
  void *incoherentroot[4];

  const int ray0 = rays.rayBegin + 0;
  const int ray1 = rays.rayBegin + 1;
  const int ray2 = rays.rayBegin + 2;
  const int ray3 = rays.rayBegin + 3;

  raymask = 0x0;// = ( packet->raymask >> RF_PACKETOFFSET ) & 0xf;

  if (!rays.rayIsMasked(0))
    raymask |= 0x1;
  if (!rays.rayIsMasked(1))
    raymask |= 0x2;
  if (!rays.rayIsMasked(2))
    raymask |= 0x4;
  if (!rays.rayIsMasked(3))
    raymask |= 0x8;

  donemask = 0x0;

  vunit = _mm_set1_ps( 1.0 );

#if 1
  vsrc[0] = _mm_load_ps(&rays.getOrigin(rays.rayBegin, 0));
  vsrc[1] = _mm_load_ps(&rays.getOrigin(rays.rayBegin, 1));
  vsrc[2] = _mm_load_ps(&rays.getOrigin(rays.rayBegin, 2));

  vector[0] = _mm_load_ps(&rays.getDirection(rays.rayBegin, 0));
  vector[1] = _mm_load_ps(&rays.getDirection(rays.rayBegin, 1));
  vector[2] = _mm_load_ps(&rays.getDirection(rays.rayBegin, 2));

  __m128 vepsilon = _mm_set1_ps(T_EPSILON);

  vsrc[0] = _mm_add_ps(vsrc[0], _mm_mul_ps(vector[0], vepsilon));
  vsrc[1] = _mm_add_ps(vsrc[1], _mm_mul_ps(vector[1], vepsilon));
  vsrc[2] = _mm_add_ps(vsrc[2], _mm_mul_ps(vector[2], vepsilon));
#else
  // [line 107-109]
  vsrc[0] = _mm_load_ps( &packet->origin[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)] );
  vsrc[1] = _mm_load_ps( &packet->origin[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)] );
  vsrc[2] = _mm_load_ps( &packet->origin[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)] );
  // [/line]

  // [line 101-103]
  vector[0] = _mm_load_ps( &packet->vector[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)] );
  vector[1] = _mm_load_ps( &packet->vector[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)] );
  vector[2] = _mm_load_ps( &packet->vector[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)] );
  // [/line]
#endif

  RF_RESULT->hitmask = 0;
  _mm_store_ps( &RF_RESULT->hitdist[0], vunit );

  // [info]
  // Computing the ray inverse to accelerate ray-box intersection
  //
  // Also compute the sign of the ray and invert it to test if rays
  // are coherent on every axis
  //
  // edgevmask = [1 if positive on the axis, 0 otherwise]
  raymaskinv = raymask ^ 0xF;
  vecflag = 0;
  // x axis
  vectinv[0] = _mm_div_ps( vunit, vector[0] );
  edgevmask[0] = raymaskinv | ( _mm_movemask_ps( vector[0] ) ^ 0xF );
  if( (uint32_t)(edgevmask[0]-1) < 0xE )
    vecflag = 1;
  // y axis
  vectinv[1] = _mm_div_ps( vunit, vector[1] );
  edgevmask[1] = raymaskinv | ( _mm_movemask_ps( vector[1] ) ^ 0xF );
  if( (uint32_t)(edgevmask[1]-1) < 0xE )
    vecflag = 1;
  // z axis
  vectinv[2] = _mm_div_ps( vunit, vector[2] );
  edgevmask[2] = raymaskinv | ( _mm_movemask_ps( vector[2] ) ^ 0xF );
  if( (uint32_t)(edgevmask[2]-1) < 0xE )
    vecflag = 1;
  // [/info]

  /* Storage for trace1 */
  _mm_store_ps( &RF_RESULT->vectinv[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)], vectinv[0] );
  _mm_store_ps( &RF_RESULT->vectinv[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)], vectinv[1] );
  _mm_store_ps( &RF_RESULT->vectinv[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)], vectinv[2] );

  // [info]
  // One of the rays is incoherent on one of the axes, go ahead and trace
  // as single rays
  if(vecflag)
  {
    if( raymask & 1 ) intersectSingle(rays, ray0, roots[0]);
    if( raymask & 2 ) intersectSingle(rays, ray1, roots[1]);
    if( raymask & 4 ) intersectSingle(rays, ray2, roots[2]);
    if( raymask & 8 ) intersectSingle(rays, ray3, roots[3]);
    return;
  }
  // [/info]

  ////////////////////////////////////////////////////////////////
  // REMOVED POSSIBLE SSE ORIGIN RESOLUTION CODE
  ////////////////////////////////////////////////////////////////

  // Load pre-resolved root
  root = roots[0];//RF_ADDRESS( handle->objectgraph, roots[0] );

  edgeindex[RF_AXIS_X] = ( RF_AXIS_X << RF_EDGE_AXIS_SHIFT ) | ( ( edgevmask[RF_AXIS_X] >> 0 ) & 1 );
  edgeindex[RF_AXIS_Y] = ( RF_AXIS_Y << RF_EDGE_AXIS_SHIFT ) | ( ( edgevmask[RF_AXIS_Y] >> 0 ) & 1 );
  edgeindex[RF_AXIS_Z] = ( RF_AXIS_Z << RF_EDGE_AXIS_SHIFT ) | ( ( edgevmask[RF_AXIS_Z] >> 0 ) & 1 );

  for( ; ; )
  {
    /* Sector traversal */
    int32_t vmask0, vmask1, vmask2, submask;
    __m128 vxdist, vydist, vmindist;

    // [info] [line 140-153]
    // Ray box intersection to determine endpoint of the line segment to
    // intersect with triangles. (faster than raw ray intersection, we think)
    vxdist = _mm_mul_ps( _mm_sub_ps( _mm_set1_ps( RF_SECTOR(root)->edge[ edgeindex[0] ] ), vsrc[0] ), vectinv[0] );
    vydist = _mm_mul_ps( _mm_sub_ps( _mm_set1_ps( RF_SECTOR(root)->edge[ edgeindex[1] ] ), vsrc[1] ), vectinv[1] );
    vmindist = _mm_mul_ps( _mm_sub_ps( _mm_set1_ps( RF_SECTOR(root)->edge[ edgeindex[2] ] ), vsrc[2] ), vectinv[2] );
    vmindist = _mm_min_ps( vmindist, _mm_min_ps( vxdist, vydist ) );

    submask = 0x0;
    // Sort out if all rays agree if the ray endpoint should be found in x, y, or z...start with z
    //
    // Use a "majority vote" mechanism (using bitmasks) to figure out what axis to use for the
    // majority of rays
    vmask0 = _mm_movemask_ps( _mm_cmpeq_ps( vmindist, vxdist ) ) & raymask;
    nredge = edgeindex[2];
    if( vmask0 == raymask )// all active rays less than x?
      nredge = edgeindex[0];// x is the min dist
    else if( ( vmask1 = ( _mm_movemask_ps( _mm_cmpeq_ps( vmindist, vydist ) ) & raymask ) ) == raymask )// check y?
      nredge = edgeindex[1];// y is the min dist
    else if( vmask0 | vmask1 )// some are x, some are y?
    {
      submask = raymask;
#if RF_CPUSLOW_SHR
      if( maskcount[vmask0] >= 2 )
#else
      if( 0xfec8 & ( 1 << vmask0 ) )
#endif
      {
        submask -= vmask0;
        nredge = edgeindex[0];
      }
#if RF_CPUSLOW_SHR
      else if( maskcount[vmask1] >= 2 )
#else
      else if( 0xfec8 & ( 1 << vmask1 ) )
#endif
      {
        submask -= vmask1;
        nredge = edgeindex[1];
      }
#if RF_CPUSLOW_SHR
      else if( maskcount[ vmask2 = ( ( vmask0 | vmask1 ) ^ raymask ) ] >= 2 )
#else
      else if( 0xfec8 & ( 1 << ( vmask2 = ( ( vmask0 | vmask1 ) ^ raymask ) ) ) )
#endif
        submask -= vmask2;
    }
    // [/info]

    if( !( RF_SECTOR_GET_PRIMCOUNT( RF_SECTOR(root) ) ) )
    {
      if( submask )// are there any rays left over that don't agree with the majority from above?
      {
        void *rayroot;
        int32_t raynredge;

        vsrc[0] = _mm_add_ps( vsrc[0], _mm_mul_ps( vmindist, vector[0] ) );
        vsrc[1] = _mm_add_ps( vsrc[1], _mm_mul_ps( vmindist, vector[1] ) );
        vsrc[2] = _mm_add_ps( vsrc[2], _mm_mul_ps( vmindist, vector[2] ) );

        _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)], vsrc[0] );
        _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)], vsrc[1] );
        _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)], vsrc[2] );

        // figure out which ray(s) are in the minority
        if( submask & 0x1 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x1 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x1 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray0, rayroot);
        }
        if( submask & 0x2 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x2 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x2 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray1, rayroot);
        }
        if( submask & 0x4 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x4 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x4 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray2, rayroot);
        }
        if( submask & 0x8 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x8 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x8 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray3, rayroot);
        }
        raymask -= submask;// we have traced the rays via the single ray api, thus "deactivate" them from the overall ray mask
        if( !( raymask ) )// check to see if we have traced every ray in the mask?
          goto done;

        // (We think) some rays need to see if they traverse to another sector?
        if( RF_SECTOR(root)->flags & ( (RF_LINK_SECTOR<<RF_SECTOR_LINKFLAGS_SHIFT) << nredge ) )
        {
          slink = (rfssize)RF_SECTOR(root)->link[ nredge ];
          if( !( slink ) )
            goto done;
          root = RF_ADDRESS( root, slink << RF_LINKSHIFT );
          continue;
        }
        // Don't prep for node traversal because we won't be doing SSE node traversal???
      }
      else
      {
        // All rays agree about the edge (face) we are traversing
        //
        // Does the link point to a valid sector?
        if( RF_SECTOR(root)->flags & ( (RF_LINK_SECTOR<<RF_SECTOR_LINKFLAGS_SHIFT) << nredge ) )
        {
          slink = (rfssize)RF_SECTOR(root)->link[ nredge ];
          if( !( slink ) )// have we hit the edge of the graph?
            goto done;
          root = RF_ADDRESS( root, slink << RF_LINKSHIFT );
          continue;
        }

        // We are NOT going straight to another sector, prep for node traversal
        vsrc[0] = _mm_add_ps( vsrc[0], _mm_mul_ps( vmindist, vector[0] ) );
        vsrc[1] = _mm_add_ps( vsrc[1], _mm_mul_ps( vmindist, vector[1] ) );
        vsrc[2] = _mm_add_ps( vsrc[2], _mm_mul_ps( vmindist, vector[2] ) );
      }
    }
    else
    {
      // Triangle intersection!!!
      int32_t a;
      __m128 vtridst[3];
      RF_TRILIST_TYPE *trilist;

      vtridst[0] = vdst[0] = _mm_add_ps( _mm_mul_ps( vmindist, vector[0] ), vsrc[0] );
      vtridst[1] = vdst[1] = _mm_add_ps( _mm_mul_ps( vmindist, vector[1] ), vsrc[1] );
      vtridst[2] = vdst[2] = _mm_add_ps( _mm_mul_ps( vmindist, vector[2] ), vsrc[2] );

      trilist = RF_TRILIST( RF_SECTOR(root) );
      for( a = RF_SECTOR(root)->primcount ; a ; a-- )
      {
        int32_t tflags, hitmask;
        rfTri *tri;
        int32_t signmask;

        tri = (rfTri *)RF_ADDRESS( root, (rfssize)(*trilist++) << RF_LINKSHIFT );
        pl0 = _mm_set1_ps( tri->plane[0] );
        pl1 = _mm_set1_ps( tri->plane[1] );
        pl2 = _mm_set1_ps( tri->plane[2] );
        pl3 = _mm_set1_ps( tri->plane[3] );

        // Intersect the triangle with no front/back face culling
        dstdist = _mm_add_ps( _mm_add_ps( _mm_mul_ps( pl0, vtridst[0] ), _mm_mul_ps( pl1, vtridst[1] ) ), _mm_add_ps( _mm_mul_ps( pl2, vtridst[2] ), pl3 ) );
        srcdist = _mm_add_ps( _mm_add_ps( _mm_mul_ps( pl0, vsrc[0] ), _mm_mul_ps( pl1, vsrc[1] ) ), _mm_add_ps( _mm_mul_ps( pl2, vsrc[2] ), pl3 ) );
        tflags = ( _mm_movemask_ps( _mm_mul_ps( srcdist, dstdist ) ) ^ 0xF ) & raymask;
        if( tflags == raymask )
          continue;
        vsum = _mm_sub_ps( dstdist, srcdist );
        vray[0] = _mm_sub_ps( _mm_mul_ps( vsrc[0], dstdist ), _mm_mul_ps( vtridst[0], srcdist ) );
        vray[1] = _mm_sub_ps( _mm_mul_ps( vsrc[1], dstdist ), _mm_mul_ps( vtridst[1], srcdist ) );
        vray[2] = _mm_sub_ps( _mm_mul_ps( vsrc[2], dstdist ), _mm_mul_ps( vtridst[2], srcdist ) );
        signmask = _mm_movemask_ps( vsum );
        utd = _mm_add_ps( _mm_add_ps( _mm_add_ps( _mm_mul_ps( _mm_set1_ps( tri->edpu[0] ), vray[0] ), _mm_mul_ps( _mm_set1_ps( tri->edpu[1] ), vray[1] ) ), _mm_mul_ps( _mm_set1_ps( tri->edpu[2] ), vray[2] ) ), _mm_mul_ps( _mm_set1_ps( tri->edpu[3] ), vsum ) );
        tflags |= ( _mm_movemask_ps( utd ) ^ signmask ) & raymask;
        if( tflags == raymask )
          continue;
        vtd = _mm_add_ps( _mm_add_ps( _mm_add_ps( _mm_mul_ps( _mm_set1_ps( tri->edpv[0] ), vray[0] ), _mm_mul_ps( _mm_set1_ps( tri->edpv[1] ), vray[1] ) ), _mm_mul_ps( _mm_set1_ps( tri->edpv[2] ), vray[2] ) ), _mm_mul_ps( _mm_set1_ps( tri->edpv[3] ), vsum ) );
        tflags |= ( _mm_movemask_ps( vtd ) ^ signmask ) & raymask;
        if( tflags == raymask )
          continue;
        tflags |= ( _mm_movemask_ps( _mm_cmple_ps( vsum, _mm_add_ps( utd, vtd ) ) ) ^ signmask ) & raymask;
        if( tflags == raymask )
          continue;

        vsum = _mm_div_ps( vunit, vsum );
        hitmask = tflags ^ raymask;

        donemask |= hitmask;// mark rays that are both "active" and "hit/done"
        // tflags = rays that did hit the plane of the triangle
        if( !( tflags ) )
        {
          // All of the rays hit the triangle, compute the distances for all of the rays
          vtridst[0] = _mm_mul_ps( vray[0], vsum );
          vtridst[1] = _mm_mul_ps( vray[1], vsum );
          vtridst[2] = _mm_mul_ps( vray[2], vsum );
        }
        else
        {
          // ...sort out (efficiently) the rays that did hit this triangle
          __m128 vblendmask;
          vblendmask = _mm_load_ps( (float *)&ortable[ hitmask << 2 ] );
#if RFTRACE__SSE4_1__ && !RF_CPUSLOW_BLENDVPS
          vtridst[0] = _mm_blendv_ps( vtridst[0], _mm_mul_ps( vray[0], vsum ), vblendmask );
          vtridst[1] = _mm_blendv_ps( vtridst[1], _mm_mul_ps( vray[1], vsum ), vblendmask );
          vtridst[2] = _mm_blendv_ps( vtridst[2], _mm_mul_ps( vray[2], vsum ), vblendmask );
#else
          vtridst[0] = _mm_or_ps( _mm_and_ps( _mm_mul_ps( vray[0], vsum ), vblendmask ), _mm_andnot_ps( vblendmask, vtridst[0] ) );
          vtridst[1] = _mm_or_ps( _mm_and_ps( _mm_mul_ps( vray[1], vsum ), vblendmask ), _mm_andnot_ps( vblendmask, vtridst[1] ) );
          vtridst[2] = _mm_or_ps( _mm_and_ps( _mm_mul_ps( vray[2], vsum ), vblendmask ), _mm_andnot_ps( vblendmask, vtridst[2] ) );
#endif
        }

        /* TODO: Optimize me */
        axisindex = nredge >> 1;
        // [line 236-238?]
        vhitdist = _mm_mul_ps( _mm_sub_ps( vtridst[axisindex], *(__m128*)&rays.getOrigin(rays.rayBegin, axisindex) ), vectinv[axisindex] );
        // [info]
        // Figure out where to store the results of the ray hits
        if( hitmask == 0xF )
        {
          // All the rays hit the same thing
          RF_RESULT->hittri[RF_PACKETOFFSET+0] = tri;
          RF_RESULT->hittri[RF_PACKETOFFSET+1] = tri;
          RF_RESULT->hittri[RF_PACKETOFFSET+2] = tri;
          RF_RESULT->hittri[RF_PACKETOFFSET+3] = tri;
          _mm_store_ps( &RF_RESULT->hitdist[RF_PACKETOFFSET], vhitdist );
        }
        else
        {
          // Store the data one ray at a time
          __m128 vblendmask;
          if( hitmask & 1 )
            RF_RESULT->hittri[RF_PACKETOFFSET+0] = tri;
          if( hitmask & 2 )
            RF_RESULT->hittri[RF_PACKETOFFSET+1] = tri;
          if( hitmask & 4 )
            RF_RESULT->hittri[RF_PACKETOFFSET+2] = tri;
          if( hitmask & 8 )
            RF_RESULT->hittri[RF_PACKETOFFSET+3] = tri;

          vblendmask = _mm_load_ps( (float *)&ortable[ hitmask << 2 ] );
 #if RFTRACE__SSE4_1__ && !RF_CPUSLOW_BLENDVPS
          _mm_store_ps( &RF_RESULT->hitdist[RF_PACKETOFFSET], _mm_blendv_ps( _mm_load_ps( &RF_RESULT->hitdist[RF_PACKETOFFSET] ), vhitdist, vblendmask ) );
 #else
          _mm_store_ps( &RF_RESULT->hitdist[RF_PACKETOFFSET], _mm_or_ps( _mm_and_ps( vhitdist, vblendmask ), _mm_andnot_ps( vblendmask, _mm_load_ps( &RF_RESULT->hitdist[RF_PACKETOFFSET] ) ) ) );
 #endif
        }

        // [/info]
      }

      // Deactivate rays that agreed with major edge traversal and reset submask to the other rays that need intersected
      submask &= ~donemask;

      // Do we have rays to still trace?
      if( submask )
      {
        void *rayroot;
        int32_t raynredge;

        _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)], vdst[0] );
        _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)], vdst[1] );
        _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)], vdst[2] );

        if( submask & 0x1 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x1 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x1 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray0, rayroot);
        }
        if( submask & 0x2 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x2 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x2 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray1, rayroot);
        }
        if( submask & 0x4 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x4 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x4 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray2, rayroot);
        }
        if( submask & 0x8 )
        {
          raynredge = edgeindex[2];
          if( vmask0 & 0x8 )
            raynredge = edgeindex[0];
          else if( vmask1 & 0x8 )
            raynredge = edgeindex[1];
          rayroot = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ raynredge ] << RF_LINKSHIFT );
          if( rayroot )
            intersectSingle(rays, ray3, rayroot);
        }
      }

      // Update ray mask to what we have traced thus far
      raymask &= ~( submask | donemask );
      if( !( raymask ) )
        goto done;
      if( RF_SECTOR(root)->flags & ( (RF_LINK_SECTOR<<RF_SECTOR_LINKFLAGS_SHIFT) << nredge ) )
      {
        slink = (rfssize)RF_SECTOR(root)->link[ nredge ];
        if( !( slink ) )
          goto done;
        root = RF_ADDRESS( root, slink << RF_LINKSHIFT );
        continue;
      }

      // Prep to push into adjacent nodes
      vsrc[0] = vdst[0];
      vsrc[1] = vdst[1];
      vsrc[2] = vdst[2];
    }

    // ALL OF THIS SECTOR'S TRAVERSAL IS DONE, begin adjacent node traversal (because some rays are still active)

    root = RF_ADDRESS( root, (rfssize)RF_SECTOR(root)->link[ nredge ] << RF_LINKSHIFT );

    _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)], vsrc[0] );
    _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)], vsrc[1] );
    _mm_store_ps( &RF_RESULT->raysrc[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)], vsrc[2] );

    /* Node traversal */
    for( ; ; )
    {
      int32_t linkflags, nbits;
      __m128 nodesrc;

      linkflags = RF_NODE(root)->flags;

      /* TODO: What about branches? Merges? Something? */
      nodesrc = _mm_load_ps( &RF_RESULT->raysrc[ RF_PACKETOFFSET + ( RF_NODE_GET_AXIS(linkflags) * RF_PACKETWIDTH ) ] );

      nbits = _mm_movemask_ps( _mm_cmplt_ps( nodesrc, _mm_set1_ps( RF_NODE(root)->plane ) ) ) & raymask;
      if( !( nbits ) )
      {
        root = RF_ADDRESS( root, (rfssize)RF_NODE(root)->link[RF_NODE_MORE] << RF_LINKSHIFT );
        if( linkflags & ( (RF_LINK_SECTOR<<RF_NODE_LINKFLAGS_SHIFT) << RF_NODE_MORE ) )
          break;
      }
      else if( nbits == raymask )
      {
        root = RF_ADDRESS( root, (rfssize)RF_NODE(root)->link[RF_NODE_LESS] << RF_LINKSHIFT );
        if( linkflags & ( (RF_LINK_SECTOR<<RF_NODE_LINKFLAGS_SHIFT) << RF_NODE_LESS ) )
          break;
      }
      else
      {
        int32_t nbitsl, snmask, rayflag, raypath, nbitscount;
        void *rayroot;
        nbitsl = nbits ^ raymask;
        snmask = nbits & raymask;
#if RF_CPUSLOW_SHR
        nbitscount = maskcount[ nbits ];
        if( nbitscount > maskcount[ nbitsl ] )
#else
        nbitscount = ( ( 0xfee9e994 >> (nbits+nbits) ) & 0x3 );
        if( nbitscount > ( ( 0xfee9e994 >> (nbitsl+nbitsl) ) & 0x3 ) )
#endif
        {
          snmask ^= raymask;
          if( nbitscount < 2 )
            snmask = raymask;
        }

        if( snmask & 0x1 )
        {
          rayflag = ( nbitsl >> 0 ) & 0x1;
          rayroot = RF_ADDRESS( root, (rfssize)RF_NODE(root)->link[ rayflag ] << RF_LINKSHIFT );
          raypath = ( linkflags >> ( RF_NODE_LINKFLAGS_SHIFT + rayflag ) ) & 0x1;
          intersectSingle(rays, ray0, rayroot);
        }
        if( snmask & 0x2 )
        {
          rayflag = ( nbitsl >> 1 ) & 0x1;
          rayroot = RF_ADDRESS( root, (rfssize)RF_NODE(root)->link[ rayflag ] << RF_LINKSHIFT );
          raypath = ( linkflags >> ( RF_NODE_LINKFLAGS_SHIFT + rayflag ) ) & 0x1;
          intersectSingle(rays, ray1, rayroot);
        }
        if( snmask & 0x4 )
        {
          rayflag = ( nbitsl >> 2 ) & 0x1;
          rayroot = RF_ADDRESS( root, (rfssize)RF_NODE(root)->link[ rayflag ] << RF_LINKSHIFT );
          raypath = ( linkflags >> ( RF_NODE_LINKFLAGS_SHIFT + rayflag ) ) & 0x1;
          intersectSingle(rays, ray2, rayroot);
        }
        if( snmask & 0x8 )
        {
          rayflag = ( nbitsl >> 3 ) & 0x1;
          rayroot = RF_ADDRESS( root, (rfssize)RF_NODE(root)->link[ rayflag ] << RF_LINKSHIFT );
          raypath = ( linkflags >> ( RF_NODE_LINKFLAGS_SHIFT + rayflag ) ) & 0x1;
          intersectSingle(rays, ray3, rayroot);
        }
        raymask -= snmask;
        if( !( raymask ) )
          goto done;
      }
    }
  }

  done:

    rfTri *tri0, *tri1, *tri2, *tri3;
    donemask |= ( RF_RESULT->hitmask >> RF_PACKETOFFSET ) & 0xf;
    tri0 = &((rfTri*)RF_RESULT->hittri)[RF_PACKETOFFSET+0];
    tri1 = &((rfTri*)RF_RESULT->hittri)[RF_PACKETOFFSET+1];
    tri2 = &((rfTri*)RF_RESULT->hittri)[RF_PACKETOFFSET+2];
    tri3 = &((rfTri*)RF_RESULT->hittri)[RF_PACKETOFFSET+3];
    if( donemask & 0x1 )
      pl0 = _mm_load_ps( tri0->plane );
    else
      pl0 = _mm_set1_ps( 0.0 );
    if( donemask & 0x2 )
      pl1 = _mm_load_ps( tri1->plane );
    else
      pl1 = _mm_set1_ps( 0.0 );
    if( donemask & 0x4 )
      pl2 = _mm_load_ps( tri2->plane );
    else
      pl2 = _mm_set1_ps( 0.0 );
    if( donemask & 0x8 )
      pl3 = _mm_load_ps( tri3->plane );
    else
      pl3 = _mm_set1_ps( 0.0 );

#define TRACEHITPLANEPACKED 0

 #if !TRACEHITPLANEPACKED
    __m128 t0, t1, t2, t3;
    t0 = _mm_unpacklo_ps( pl0, pl1 );
    t1 = _mm_unpacklo_ps( pl2, pl3 );
    t2 = _mm_unpackhi_ps( pl0, pl1 );
    t3 = _mm_unpackhi_ps( pl2, pl3 );
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)], _mm_movelh_ps( t0, t1 ) );
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)], _mm_movehl_ps( t1, t0 ) );
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)], _mm_movelh_ps( t2, t3 ) );
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(3*RF_PACKETWIDTH)], _mm_movehl_ps( t3, t2 ) );
 #else
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(0*RF_PACKETWIDTH)], pl0 );
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(1*RF_PACKETWIDTH)], pl1 );
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(2*RF_PACKETWIDTH)], pl2 );
    _mm_store_ps( &RF_RESULT->hitplane[RF_PACKETOFFSET+(3*RF_PACKETWIDTH)], pl3 );
 #endif

  RF_RESULT->hitmask |= donemask << RF_PACKETOFFSET;

  RF_RESULT->hittri[0] = RF_ADDRESS( RF_RESULT->hittri[0], sizeof(rfTri) );
  RF_RESULT->hittri[1] = RF_ADDRESS( RF_RESULT->hittri[1], sizeof(rfTri) );
  RF_RESULT->hittri[2] = RF_ADDRESS( RF_RESULT->hittri[2], sizeof(rfTri) );
  RF_RESULT->hittri[3] = RF_ADDRESS( RF_RESULT->hittri[3], sizeof(rfTri) );

  //[info]
  // Set the ray hit information for the ray packet
  for (int i = 0; i < 4; ++i)
  {
    if(RF_RESULT->hitmask & (0x1 << i))
    {
      rfTriangleData* data = (rfTriangleData*)(RF_RESULT->hittri[i]);

      Material *material   = currMesh->materials[data->matID];
      Primitive *primitive = (Primitive*)currMesh->get(data->triID);

      float hitdist = RF_RESULT->hitdist[i];
      rays.hit(rays.rayBegin+i, hitdist - T_EPSILON, material, primitive, this);

      #if !TRACEHITPLANEPACKED
      Vector normal(RF_RESULT->hitplane[0+i],
                    RF_RESULT->hitplane[4+i],
                    RF_RESULT->hitplane[8+i]);
      #else
      Vector normal(RF_RESULT->hitplane[4*i+0],
                    RF_RESULT->hitplane[4*i+1],
                    RF_RESULT->hitplane[4*i+2]);
      #endif
      normal.normalize();
      rays.setNormal(rays.rayBegin+i, normal);
    }
  }
  //[/info]
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
