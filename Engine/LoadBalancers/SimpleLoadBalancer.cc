
#include <Engine/LoadBalancers/SimpleLoadBalancer.h>
#include <Interface/Context.h>
#include <Core/Util/NotFinished.h>

using namespace Manta;

LoadBalancer* SimpleLoadBalancer::create(const vector<string>& args)
{
  return new SimpleLoadBalancer(args);
}

SimpleLoadBalancer::SimpleLoadBalancer(const vector<string>& /* args */)
{
}

SimpleLoadBalancer::~SimpleLoadBalancer()
{
}

SimpleLoadBalancer::ChannelInfo::ChannelInfo(int numProcs)
{
  processorInfo.resize(numProcs);
}

SimpleLoadBalancer::ChannelInfo::~ChannelInfo()
{
}

void SimpleLoadBalancer::setupBegin(const SetupContext& context, int numChannels)
{
  int oldchannels = static_cast<int>(channelInfo.size());
  for(int i=numChannels;i<oldchannels;i++)
    delete channelInfo[i];
  channelInfo.resize(numChannels);
  for(int i=oldchannels;i<numChannels;i++)
    channelInfo[i]=new ChannelInfo(context.numProcs);
}

void SimpleLoadBalancer::setupDisplayChannel(SetupContext& context,
					     int numAssignments)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ci->numAssignments = numAssignments;
}

void SimpleLoadBalancer::setupFrame(const RenderContext& context)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ProcessorInfo& pi = ci->processorInfo[context.proc];
  pi.done = false;
}

bool SimpleLoadBalancer::getNextAssignment(const RenderContext& context,
					   int& s, int& e)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ProcessorInfo& pi = ci->processorInfo[context.proc];
  if(pi.done)
    return false;
  
  s = (ci->numAssignments * context.proc)/context.numProcs;
  e = (ci->numAssignments * (context.proc+1))/context.numProcs;
  pi.done=true;
  return true;
}
