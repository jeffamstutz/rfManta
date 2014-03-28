#ifndef Manta_Engine_MPI_LoadBalancer_h
#define Manta_Engine_MPI_LoadBalancer_h

#include <Interface/LoadBalancer.h>
#include <Parameters.h>
#include <Core/Thread/Mutex.h>
#include <Core/Thread/WorkQueue.h>
#include <string>
#include <vector>

namespace Manta {

  enum MPI_TAGS{TAG_BUFFER, TAG_DONE_RENDERING, TAG_WORK, TAG_CONTROL};

  using namespace std;
  class MPI_LoadBalancer : public LoadBalancer {
  public:
    MPI_LoadBalancer(const vector<string>& args);
    virtual ~MPI_LoadBalancer();

    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext& context,
                     int numAssignments);
    virtual void setupFrame(const RenderContext& context);
    virtual bool getNextAssignment(const RenderContext& context,
                   int& start, int& end);

    static LoadBalancer* create(const vector<string>& args);

    bool getMPIAssignments(const RenderContext& context);
    bool giveMPIAssignments(const RenderContext& context, const int slave); // to be called by master only!
    bool needMoreMPIAssignments(const RenderContext& context);

    int getMasterRank() { return master; }

    static void setNumRenderThreadsPerNode(int n) { renderThreadsPerNode = n; }

  private:
    MPI_LoadBalancer(const MPI_LoadBalancer&);
    MPI_LoadBalancer& operator=(const MPI_LoadBalancer&);

    struct ProcessorInfo {
      bool done;
      char pad[MAXCACHELINESIZE-sizeof(bool)];
    };
    struct ChannelInfo {
      ChannelInfo(int numProcs);
      ~ChannelInfo();

      vector<ProcessorInfo> processorInfo;
      WorkQueue workq; // each node has their own workq for assigning to threads
      WorkQueue workqMaster; // workq owned by master for assigning to nodes
      int numAssignments;
      int startAssignment;
      double frameStartTime;

      Mutex mutex;
    };
    vector<ChannelInfo*> channelInfo;
    int granularity; // granularity for the local workQueue.

    // since the master process might only have 1 thread total, we need to
    // record how many threads the render processes have.
    static int renderThreadsPerNode;

    int master;
    bool noMoreData;
  };
}

#endif
