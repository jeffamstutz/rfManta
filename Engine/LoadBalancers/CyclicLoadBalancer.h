
#ifndef Manta_Engine_CyclicLoadBalancer_h
#define Manta_Engine_CyclicLoadBalancer_h

#include <Interface/LoadBalancer.h>
#include <Parameters.h>
#include <Core/Thread/WorkQueue.h>
#include <string>
#include <vector>

namespace Manta {

  using namespace std;
  class CyclicLoadBalancer : public LoadBalancer {
  public:
    CyclicLoadBalancer(const vector<string>& args);
    virtual ~CyclicLoadBalancer();

    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext& context,
				     int numAssignments);
    virtual void setupFrame(const RenderContext& context);
    virtual bool getNextAssignment(const RenderContext& context,
				   int& start, int& end);

    static LoadBalancer* create(const vector<string>& args);
  private:
    CyclicLoadBalancer(const CyclicLoadBalancer&);
    CyclicLoadBalancer& operator=(const CyclicLoadBalancer&);

    struct ProcessorInfo {
      char pad0[MAXCACHELINESIZE];
      int cur;
      int count;
      char pad1[MAXCACHELINESIZE-2*sizeof(int)];
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
