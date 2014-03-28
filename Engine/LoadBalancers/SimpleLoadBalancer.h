
#ifndef Manta_Engine_SimpleLoadBalancer_h
#define Manta_Engine_SimpleLoadBalancer_h

#include <Interface/LoadBalancer.h>
#include <Parameters.h>
#include <Core/Thread/WorkQueue.h>
#include <string>
#include <vector>

namespace Manta {

  using namespace std;
  class SimpleLoadBalancer : public LoadBalancer {
  public:
    SimpleLoadBalancer(const vector<string>& args);
    virtual ~SimpleLoadBalancer();

    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext& context,
				     int numAssignments);
    virtual void setupFrame(const RenderContext& context);
    virtual bool getNextAssignment(const RenderContext& context,
				   int& start, int& end);

    static LoadBalancer* create(const vector<string>& args);
  private:
    SimpleLoadBalancer(const SimpleLoadBalancer&);
    SimpleLoadBalancer& operator=(const SimpleLoadBalancer&);

    struct ProcessorInfo {
      bool done;
      char pad[MAXCACHELINESIZE-sizeof(bool)];
    };
    struct ChannelInfo {
      ChannelInfo(int numProcs);
      ~ChannelInfo();

      vector<ProcessorInfo> processorInfo;
      int numAssignments;
    };
    vector<ChannelInfo*> channelInfo;
  };
}

#endif
