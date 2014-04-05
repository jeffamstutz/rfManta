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

using namespace Manta;

RFGraph::RFGraph() : graph(0)
{
  /*no-op*/
}

RFGraph::~RFGraph()
{
  // Free the graph if it exists
  if(graph)
    free(graph);
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

void RFGraph::intersect(const RenderContext& /*context*/, RayPacket& /*rays*/) const
{
  fprintf(stderr, "intersect()\n");
  //TODO: intersect rays
}

void RFGraph::setGroup(Group* /*new_group*/)
{
  fprintf(stderr, "setGroup()\n");
  /*no op*/
}

void RFGraph::groupDirty()
{
  fprintf(stderr, "groupDirty()\n");
  /*no op*/
}

Group* RFGraph::getGroup() const
{
  fprintf(stderr, "getGroup()\n");
  return NULL;
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
  //TODO: compute bounds
}
