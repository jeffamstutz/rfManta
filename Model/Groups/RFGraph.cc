#include "RFGraph.h"

using namespace Manta;

RFGraph::RFGraph()
{
  //TODO: constructor
}

RFGraph::~RFGraph()
{
  //TODO: destructor
}

bool RFGraph::buildFromFile(const std::string &fileName)
{
  return true;
}

void RFGraph::intersect(const RenderContext& context, RayPacket& rays) const
{
  //TODO: intersect rays
}

void RFGraph::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  //TODO: compute bounds
}
