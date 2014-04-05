#include "RFGraph.h"

#include <stdio.h>

using namespace Manta;

RFGraph::RFGraph()
{
  //TODO: constructor
}

RFGraph::~RFGraph()
{
  //TODO: destructor
}

bool RFGraph::buildFromFile(const std::string &/*fileName*/)
{
  fprintf(stderr, "buildFromFile()\n");
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
