#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Core/Util/NotFinished.h>
#include <Engine/LoadBalancers/WQLoadBalancer.h>
#include <Interface/Context.h>

using namespace Manta;

LoadBalancer* WQLoadBalancer::create(const vector<string>& args)
{
  return new WQLoadBalancer(args);
}

WQLoadBalancer::WQLoadBalancer(const vector<string>& args) {
  granularity = 5;
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-granularity") {
      if (!getIntArg(i, args, granularity)) {
        throw IllegalArgument("WQLoadBalancer -granularity", i, args);
      }
    } else {
      throw IllegalArgument("WQLoadBalancer", i, args);
    }
  }
}

WQLoadBalancer::~WQLoadBalancer()
{
}

WQLoadBalancer::ChannelInfo::ChannelInfo()
  : workq("WQLoadBalancer Work Queue")
{
}

WQLoadBalancer::ChannelInfo::~ChannelInfo()
{
}

void WQLoadBalancer::setupBegin(const SetupContext&, int numChannels)
{
  int oldchannels = (int)channelInfo.size();
  for(int i = numChannels; i < oldchannels; i++)
    delete channelInfo[i];
  channelInfo.resize(numChannels);
  for(int i = oldchannels; i < numChannels; i++)
    channelInfo[i] = new ChannelInfo();
}

void WQLoadBalancer::setupDisplayChannel(SetupContext& context,
                                         int numAssignments)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ci->numAssignments = numAssignments;
}

void WQLoadBalancer::setupFrame(const RenderContext& context)
{
  if(context.proc == 0){
    ChannelInfo* ci = channelInfo[context.channelIndex];
    ci->workq.refill(ci->numAssignments, context.numProcs, granularity);
  }
}

bool WQLoadBalancer::getNextAssignment(const RenderContext& context,
                                       int& s, int& e)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  return ci->workq.nextAssignment(s, e);
}
