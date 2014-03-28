#include <mpi.h>
#include <Engine/LoadBalancers/MPI_LoadBalancer.h>
#include <Interface/Context.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Core/Math/MiscMath.h>

using namespace Manta;

int MPI_LoadBalancer::renderThreadsPerNode = 8; // default to 8 threads

LoadBalancer* MPI_LoadBalancer::create(const vector<string>& args)
{
  return new MPI_LoadBalancer(args);
}

MPI_LoadBalancer::MPI_LoadBalancer(const vector<string>& args) :
  granularity(5), // tunable
  master(1), noMoreData(true)
{
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i] == "-granularity") {
      if (!getIntArg(i, args, granularity)) {
        throw IllegalArgument("MPILoadBalancer -granularity", i, args);
      }
    } else {
      throw IllegalArgument("MPILoadBalancer", i, args);
    }
  }
}

MPI_LoadBalancer::~MPI_LoadBalancer()
{
}

MPI_LoadBalancer::ChannelInfo::ChannelInfo(int numProcs)
  : workq("MPILoadBalancer Work Queue"), workqMaster("MPILoadBalancer Master Work Queue"),
    frameStartTime(0), mutex("MPILoadbalancer")
{
  processorInfo.resize(numProcs);
}

MPI_LoadBalancer::ChannelInfo::~ChannelInfo()
{
}

void MPI_LoadBalancer::setupBegin(const SetupContext& context, int numChannels)
{
  int oldchannels = static_cast<int>(channelInfo.size());
  for(int i=numChannels;i<oldchannels;i++)
    delete channelInfo[i];
  channelInfo.resize(numChannels);
  for(int i=oldchannels;i<numChannels;i++)
    channelInfo[i]=new ChannelInfo(context.numProcs);
}

void MPI_LoadBalancer::setupDisplayChannel(SetupContext& context,
                         int numTotalAssignments)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ci->numAssignments = numTotalAssignments;
}

void MPI_LoadBalancer::setupFrame(const RenderContext& context)
{
  ChannelInfo* ci = channelInfo[context.channelIndex];
  ProcessorInfo& pi = ci->processorInfo[context.proc];

  if(context.proc == 0){

    // 2 masters don't render, so remove them from computation
    const int rank = MPI::COMM_WORLD.Get_rank() - 2;
    const int numNodes = MPI::COMM_WORLD.Get_size() - 2;
    const int numTotalAssignments = ci->numAssignments;

    // Each assignment to a node should actually contain an assignment for
    // every render thread.
    const int numMasterAssignments = Ceil(numTotalAssignments/(float)renderThreadsPerNode);

    // Very important to tune the masterGranularity! Large values are more
    // important when there is high load imbalance.  When at low frame rates,
    // I've found larger values work better, possibly because there is little
    // communication per second, so we can make use of smaller tiles.  At
    // higher frame rates we start to get communication bound and so a lower
    // granularity helps to lower the amount of communication to the load
    // balancer.
    int masterGranularity = 10;

    ci->workqMaster.refill(numMasterAssignments, numNodes,
                            masterGranularity);

    if (rank+2 == master) {
      // Consume what are the statically assigned assignments so that when a
      // render node actually needs to get an mpi assignment it can get an
      // unassigned assignment.
      int dummy1, end;
      bool gotWork = true;
      for (int i=0; gotWork && i < numNodes; ++i)
        gotWork = ci->workqMaster.nextAssignment(dummy1, end);
      if (gotWork) {
        // Re-granulate WQ to take into account how many assignments per second
        // are likely to be made.  Note that we have to do this after the
        // static assignments since if we did it before, the nodes would all
        // need to communicate with each other to establish a common render
        // time for the previous frame and global communication like that can
        // be expensive...

        double currTime = MPI::Wtime();
        float frameTime = static_cast<float>(currTime - ci->frameStartTime);

        const float secPerAssignment = frameTime/numMasterAssignments;
        const float ASSIGNMENT_TIME = 1E-8; // this is pseudo time (has a magic
                                            // scalar factored in).  Might need
                                            // to tune for each cluster
        const float maxSecPerAssignment = ASSIGNMENT_TIME * numMasterAssignments;

        masterGranularity = static_cast<int>(5*(secPerAssignment / maxSecPerAssignment));

        if (masterGranularity < 5)
          masterGranularity = 5;

        ci->startAssignment = end;

        ci->workqMaster.refill(numMasterAssignments-end, numNodes, masterGranularity);
        ci->frameStartTime = currTime;
      }
    }

    if (rank < 0) {
      pi.done = true; // masters don't render.
      noMoreData = true;
      return;
    }

    pi.done = false; // render nodes have work to do...

    // We statically assign the initial assignments for each render node
    // instead of requiring the render nodes to have to ask the master for
    // their initial assignments.  This is faster and more scalable than having
    // everyone all at once talking to the master at the start of the frame.
    // Because of this static assignment, it is more tricky for the master to
    // assign from a WorkQueue since that WorkQueue needs to be emptied of the
    // initial static assignments and the render nodes have to know what the
    // initial assignments from the WorkQueue would have been.

    int start, end;
    bool gotWork = true;
    for (int i=0; i <= rank && gotWork; ++i)
      gotWork = ci->workqMaster.nextAssignment(start, end);

    if (gotWork) {
      start *= renderThreadsPerNode;
      end *= renderThreadsPerNode;

      if (end > numTotalAssignments)
        end = numTotalAssignments;
    }
    else {
      start = numTotalAssignments;
      end = numTotalAssignments;
    }
    ci->startAssignment = start;

    ci->workq.refill(end-start, context.numProcs, granularity);
    noMoreData = numTotalAssignments <= 0;
  }
}

bool MPI_LoadBalancer::needMoreMPIAssignments(const RenderContext& context) {
  ChannelInfo* ci = channelInfo[context.channelIndex];
  return ci->startAssignment >= ci->numAssignments;
}

bool MPI_LoadBalancer::giveMPIAssignments(const RenderContext& context, const int slave) {
//   assert( MPI::COMM_WORLD.Get_rank() == master );
  ChannelInfo* ci = channelInfo[context.channelIndex];

  int start, end;
  bool gotWork = ci->workqMaster.nextAssignment(start, end);

  if (gotWork) {
    start+=ci->startAssignment;
    end+=ci->startAssignment;

    start *= renderThreadsPerNode;
    end *= renderThreadsPerNode;

    if (end > ci->numAssignments)
      end = ci->numAssignments;
  }
  else {
    start = 0;
    end = 0;
  }

  int range[2] = {start, end};
  MPI::COMM_WORLD.Send(range, 2, MPI_INT, slave, TAG_WORK);
  return gotWork; // were we able to give useful data?
}

// Note: This must be wrapped in a mutex.  This is not thread safe otherwise!
bool MPI_LoadBalancer::getMPIAssignments(const RenderContext& context) {
  const int rank = MPI::COMM_WORLD.Get_rank();
  if (noMoreData) return false;
  //assert(rank != master); // the master node does not render.

  ChannelInfo* ci = channelInfo[context.channelIndex];

  int newAssignments = 0;

  // get more work from master
  MPI::COMM_WORLD.Send(&rank, 1, MPI_INT, master, TAG_WORK);
  int range[2];
  MPI::COMM_WORLD.Recv(range, 2, MPI_INT, master, TAG_WORK);

  ci->startAssignment = range[0];
  newAssignments = range[1] - range[0];
  ci->workq.refill(newAssignments, context.numProcs, granularity);

  if (newAssignments == 0)
    noMoreData = true;

  return (newAssignments > 0);
}

bool MPI_LoadBalancer::getNextAssignment(const RenderContext& context,
                       int& s, int& e)
{
  if (noMoreData) return false;

//   double startTime = MPI::Wtime();

  ChannelInfo* ci = channelInfo[context.channelIndex];
  ProcessorInfo& pi = ci->processorInfo[context.proc];
  if(pi.done)
    return false;

  ci->mutex.lock();
  int startAssignment = ci->startAssignment;
  bool gotWork = ci->workq.nextAssignment(s, e);
  if (!gotWork) {
    gotWork = getMPIAssignments(context);
    if (!gotWork) {
      ci->mutex.unlock();
      return false;
    }

    gotWork = ci->workq.nextAssignment(s, e);
    startAssignment = ci->startAssignment;
  }
  ci->mutex.unlock();

  s+= startAssignment;
  e+= startAssignment;

//   printf("getNextAssignment: node %d got %d - %d assignments\n", MPI::COMM_WORLD.Get_rank(), s, e);
//    double endTime = MPI::Wtime();
//    double time = (endTime-startTime)*1000;
//    if (time > 100)
//      printf("%d - %d/%d took %f ms to get next Assignment\n",
//             context.frameState->frameSerialNumber, rank, context.proc, time );

  return gotWork;
}
