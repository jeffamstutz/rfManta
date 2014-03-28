
#ifndef Manta_Engine_WQLoadBalancer_h
#define Manta_Engine_WQLoadBalancer_h

#include <Interface/LoadBalancer.h>
#include <Core/Thread/WorkQueue.h>
#include <string>
#include <vector>

namespace Manta {

  using namespace std;
  class WQLoadBalancer : public LoadBalancer {
  public:
    WQLoadBalancer(const vector<string>& args);
    virtual ~WQLoadBalancer();

    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext& context,
                                     int numAssignments);
    virtual void setupFrame(const RenderContext& context);
    virtual bool getNextAssignment(const RenderContext& context,
                                   int& start, int& end);

    static LoadBalancer* create(const vector<string>& args);
  private:
    WQLoadBalancer(const WQLoadBalancer&);
    WQLoadBalancer& operator=(const WQLoadBalancer&);

    struct ChannelInfo {
      ChannelInfo();
      ~ChannelInfo();
      WorkQueue workq;
      int numAssignments;
    };
    vector<ChannelInfo*> channelInfo;
    int granularity;
  };
}

#endif
