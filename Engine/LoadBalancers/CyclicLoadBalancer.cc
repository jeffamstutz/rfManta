
#include <Engine/LoadBalancers/CyclicLoadBalancer.h>
#include <Interface/Context.h>
#include <Core/Util/NotFinished.h>

using namespace Manta;

LoadBalancer* CyclicLoadBalancer::create(const vector<string>& args)
{
  return new CyclicLoadBalancer(args);
}

CyclicLoadBalancer::CyclicLoadBalancer(const vector<string>& /* args */)
{
}

CyclicLoadBalancer::~CyclicLoadBalancer()
{
}

CyclicLoadBalancer::ChannelInfo::ChannelInfo(int numProcs)
{
  processorInfo.resize(numProcs);
}

CyclicLoadBalancer::ChannelInfo::~ChannelInfo()
{
}

void CyclicLoadBalancer::setupBegin(const SetupContext& context, int numChannels)
{
  int oldchannels = static_cast<int>(channelInfo.size());
  for(int i=numChannels;i<oldchannels;i++)
    delete channelInfo[i];
  channelInfo.resize(numChannels);
  for(int i=oldchannels;i<numChannels;i++)
    channelInfo[i]=new ChannelInfo(context.numProcs);
}

void CyclicLoadBalancer::setupDisplayChannel(SetupContext& context,
					     int numAssignments)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ci->numAssignments = numAssignments;
  for(int i=0;i<context.numProcs;i++){
    ProcessorInfo& pi = ci->processorInfo[i];
    pi.cur = i;
    pi.count = 0;
  }
}

void CyclicLoadBalancer::setupFrame(const RenderContext& context)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ProcessorInfo& pi = ci->processorInfo[context.proc];
  pi.cur = (context.proc+ ++pi.count)%context.numProcs;
  //pi.cur = context.proc;
}

bool CyclicLoadBalancer::getNextAssignment(const RenderContext& context,
					   int& s, int& e)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ProcessorInfo& pi = ci->processorInfo[context.proc];
  if(pi.cur >= ci->numAssignments)
    return false;
  
  s = pi.cur;
  e = pi.cur+1;
  pi.cur += context.numProcs;
  return true;
}
