#include <Engine/ImageTraversers/MPI_ImageTraverser.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/Expon.h>
#include <Core/Thread/Mutex.h>
#include <Core/Util/Args.h>
#include <Core/Util/NotFinished.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/Image.h>
#include <Interface/LoadBalancer.h>
#include <Interface/PixelSampler.h>
#include <Interface/Scene.h>
#include <Interface/RayPacket.h> // for maxsize
#include <Interface/RandomNumberGenerator.h>
#include <Model/Groups/Group.h>
#include <MantaSSE.h>

#include <Engine/LoadBalancers/MPI_LoadBalancer.h>
#include <Interface/Camera.h>

#include <Interface/FrameState.h>
#include <Interface/MantaInterface.h>
#include <Core/Thread/Thread.h>

#include <stdio.h>

#include <mpi.h>

//#define ENABLE_STATS

#ifdef ENABLE_STATS
# define PRINTDBG \
  if (true && context.frameState->frameSerialNumber > 0) \
    printf("%ld - ", context.frameState->frameSerialNumber),
#endif

using namespace Manta;

ImageTraverser* MPI_ImageTraverser::create(const vector<string>& args)
{
  return new MPI_ImageTraverser(args);
}

MPI_ImageTraverser::MPI_ImageTraverser( const int xtilesize_, const int ytilesize_ )
  : xtilesize( xtilesize_ ), ytilesize( ytilesize_ ),
    fragBuffer(NULL), fraglQueue(NULL), barrier("")
{
  if (xtilesize == ytilesize)
    shape = Fragment::SquareShape;
  else
    shape = Fragment::LineShape;

}

MPI_ImageTraverser::MPI_ImageTraverser(const vector<string>& args) :
  fragBuffer(NULL), fraglQueue(NULL), barrier("")
{
#if 0
  xtilesize = Fragment::MaxSize;
  ytilesize = Fragment::MaxSize;
#else
  // NOTE(boulos): Personally, I think our tiles should be smaller
  // than this. But this maintains the current performance for a
  // RayPacket::MaxSize = 64, and improves performance when using even
  // larger packets.
  xtilesize = Min(8, 8 * static_cast<int>(Sqrt(static_cast<Real>(Fragment::MaxSize))));
  ytilesize = Min(8, 8 * static_cast<int>(Sqrt(static_cast<Real>(Fragment::MaxSize))));
#endif
  shape = Fragment::LineShape;
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-tilesize"){
      if(!getResolutionArg(i, args, xtilesize, ytilesize))
        throw IllegalArgument("MPI_ImageTraverser -tilesize", i, args);
    } else if (arg == "-square") {
/*
        int sqrt_size = static_cast<int>(Sqrt(RayPacket::MaxSize));
        if (sqrt_size * sqrt_size != RayPacket::MaxSize)
            throw IllegalArgument("MPI_ImageTraverser -square (RayPacket::MaxSize is not square",
                                  i, args);
*/
        shape = Fragment::SquareShape;
    } else {
      throw IllegalArgument("MPI_ImageTraverser", i, args);
    }
  }
}

MPI_ImageTraverser::~MPI_ImageTraverser()
{
  if (fragBuffer)
    delete[] fragBuffer;

  if (fraglQueue)
    delete fraglQueue;

}

void MPI_ImageTraverser::setupBegin(SetupContext& context, int numChannels)
{

  static bool firstTime = true;
  if (!firstTime)
    return;
  firstTime = false;

  if (fragBuffer)
    delete[] fragBuffer;
  fragBuffer = new FragmentBuffer[context.numProcs];

  if (MPI::COMM_WORLD.Get_rank() == 0) {
    if (fraglQueue)
      delete fraglQueue;
    fraglQueue = new FragLightQueue;
  }

  context.loadBalancer->setupBegin(context, numChannels);
  LB_master = static_cast<MPI_LoadBalancer*>(context.loadBalancer)->getMasterRank();
  context.pixelSampler->setupBegin(context, numChannels);
}

void MPI_ImageTraverser::setupDisplayChannel(SetupContext& context)
{
  // Determine the resolution.
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);

  // Determine how many tiles are needed.
  xtiles = (xres + xtilesize-1)/xtilesize;
  ytiles = (yres + ytilesize-1)/ytilesize;

  // Tell the load balancer how much work to assign.
  int numAssignments = xtiles * ytiles;
  context.loadBalancer->setupDisplayChannel(context, numAssignments);

  // Continue setting up the rendering stack.
  context.pixelSampler->setupDisplayChannel(context);
}

void MPI_ImageTraverser::setupFrame(const RenderContext& context)
{
  context.loadBalancer->setupFrame(context);
  context.pixelSampler->setupFrame(context);

  if (MPI::COMM_WORLD.Get_rank() == 0) {
    fraglQueue->reset();
  }
}

void MPI_ImageTraverser::writeMPIFragment(const RenderContext& context,
                                          Image* image)
{
  const int rank = MPI::COMM_WORLD.Get_rank();
  if (rank == 0) {
    image->set(*fragBuffer[context.proc].frag);
  }
  else {
#ifdef ENABLE_STATS
    double startTime = MPI::Wtime();
#endif

    fragBuffer[context.proc].nextFrag();

    if (fragBuffer[context.proc].numFrags() >= FragmentBuffer::maxFrags()) {
      for (int i=0; i < FragmentBuffer::maxFrags(); ++i) {
        image->set(fragBuffer[context.proc].getFragmentList()[i]);
        fragBuffer[context.proc].frag_light[i].set(fragBuffer[context.proc].getFragmentList()[i],
                                                   image);
      }

//       // If we want to disable sending of pixel results for testing purposes
//        if (fragBuffer[context.proc].frag_light[0].size == 0)
      MPI::COMM_WORLD.Send(fragBuffer[context.proc].frag_light,
                           FragmentBuffer::maxFrags()*sizeof(Fragment_Light),
                           MPI_BYTE, 0, TAG_BUFFER);

      fragBuffer[context.proc].reset();
    }

#ifdef ENABLE_STATS
  double endTime = MPI::Wtime();
  double time = (endTime-startTime)*1000;
  if (time > 5)
    PRINTDBG printf("%d/%d took %f ms to write Frag\n", rank, context.proc, time);
#endif
  }
}

void MPI_ImageTraverser::exit(const RenderContext& context)
{
  delete dynamic_cast<const Group*>(context.scene->getObject())->get(0);
  MPI_Finalize();
  Thread::exitAll(0);
}

void MPI_ImageTraverser::renderImage(const RenderContext& context, Image* image)
{

#ifdef ENABLE_STATS
# if 0
  static double starttime = 0;//MPI::Wtime();
  if (context.proc == 0)
    starttime = MPI::Wtime();
# else
  double starttime = MPI::Wtime();
# endif
#endif

  static int nThreads;
  if (context.proc == 0)
    nThreads = context.numProcs;

#ifdef ENABLE_STATS
  int tilesRendered = 0;
#endif

  const int rank = MPI::COMM_WORLD.Get_rank();
  const int numprocs = MPI::COMM_WORLD.Get_size();

  bool terminate = false;
  if (context.proc == 0) {
    BasicCameraData data;
    if (rank == 0) {
      MPI::COMM_WORLD.Bcast(&terminate, sizeof(terminate), MPI_BYTE, 0);
      if (terminate) {
        exit(context);
      }

      data = context.camera->getBasicCameraData();
      MPI::COMM_WORLD.Bcast(&data, sizeof(data), MPI_BYTE, 0);
    }
    else {
      if (rank != LB_master || context.numProcs == 1) {
        MPI::COMM_WORLD.Bcast(&terminate, sizeof(terminate), MPI_BYTE, 0);
        if (terminate) {
          exit(context);
        }

        MPI::COMM_WORLD.Bcast(&data, sizeof(data), MPI_BYTE, 0);
        const_cast<Camera*>(context.camera)->setBasicCameraData(data);
      }
    }
  }
  else if (rank == LB_master && context.numProcs > 1 && context.proc == 1) {
    MPI::COMM_WORLD.Bcast(&terminate, sizeof(terminate), MPI_BYTE, 0);
    if (terminate) {
      exit(context);
    }

    BasicCameraData data;
    MPI::COMM_WORLD.Bcast(&data, sizeof(data), MPI_BYTE, 0);
    const_cast<Camera*>(context.camera)->setBasicCameraData(data);
  }

  // This slows performance by a few percent, but we need it in order to ensure
  // that everyone has the updated camera data.
  if (rank == LB_master && context.numProcs > 1) {
    if (context.proc != 0)
      barrier.wait(context.numProcs-1);
  }
  else if (rank!=0)
    barrier.wait(context.numProcs);

  if (rank == 0) {
    if (context.proc == 0) {
      int activeTasks = (numprocs-2);
      while (true) {
        MPI::COMM_WORLD.Recv(fraglQueue->data[fraglQueue->nextFree],
                             FragmentBuffer::maxFrags()*sizeof(Fragment_Light),
                             MPI_BYTE, MPI_ANY_SOURCE, TAG_BUFFER);

        if (fraglQueue->data[fraglQueue->nextFree][0].size == 0) {
          --activeTasks;
          if (activeTasks == 0) {
            fraglQueue->done = true;
            break;
          }
        }
        else {
          fraglQueue->producedOne();
        }
      }
    // Now that the for loop is done we know there are no more assignments from the
    // load balancer and everything has been rendered.
    }
    else {
      while (true) {

        int k;
        while ((k = fraglQueue->getNextAvailable())==-1 && !fraglQueue->done); // TODO: use something other than spin?

        if (k==-1 && fraglQueue->done)
          return;

        const Fragment_Light* fragls = fraglQueue->data[k];

        for (int i=0; i < FragmentBuffer::maxFrags(); ++i)
          for (unsigned short j=0; j < fragls[i].size; ++j) {
            const int x = fragls[i].pixelID[j][0];
            const int y = fragls[i].pixelID[j][1];
            static_cast<SimpleImage<RGB8Pixel>*>(image)->set(fragls[i].pixels[j], x, y, 0);
          }
      }
    }
  }
  else if (rank == LB_master && context.proc == 0) {
    int activeTasks = numprocs-2;
    while(activeTasks>0) {
      int slave;
      MPI::COMM_WORLD.Recv(&slave, 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORK);
#ifdef ENABLE_STATS
      double startTime = MPI::Wtime();
#endif
      MPI_LoadBalancer* loadBalancer =
        static_cast<MPI_LoadBalancer*>(context.loadBalancer);
      bool stillGoing = loadBalancer->giveMPIAssignments(context, slave);
      if (!stillGoing)
        --activeTasks;

#ifdef ENABLE_STATS
      double time2 = MPI::Wtime();
      double time = (time2-startTime)*1000;
      if (time > 1)
        PRINTDBG printf("Took %lfms to just giveMPIAssignments to %d in LB_master loop\n",
                        time, slave);
#endif
    }
  }
  else {

    if (fragBuffer[context.proc].frag->shape != shape) // assume the rest are all the same
      switch (shape) {
      case Fragment::LineShape:
        fragBuffer[context.proc].initialize(Fragment::LineShape,
                                            Fragment::ConsecutiveX|Fragment::ConstantEye);
        break;
      case Fragment::SquareShape:
        fragBuffer[context.proc].initialize(Fragment::SquareShape,
                                            Fragment::ConstantEye);
        break;
      case Fragment::UnknownShape:
        fragBuffer[context.proc].initialize(Fragment::UnknownShape,
                                            Fragment::ConstantEye);
        break;
      };
  }

  // Determine number of tiles.
  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);
  int numEyes = stereo?2:1;

  int s,e;

#ifdef ENABLE_STATS
  double minTileRenderTime = 1000;
  double maxTileRenderTime = 0;
  double averageRenderTime = 0;

  double startAssignmentTime = MPI::Wtime();
  int numGets = 0;
#endif

  while(context.loadBalancer->getNextAssignment(context, s, e)){
#ifdef ENABLE_STATS
    double endTime = MPI::Wtime();
    double time = (endTime-startAssignmentTime)*1000;
    if (time > 2)
      PRINTDBG printf("%d/%d took %f ms to get Assignment\n",rank, context.proc, time );

    numGets++;
#endif

    for(int assignment = s; assignment < e; assignment++){
      int xtile = assignment/ytiles;
      int ytile = assignment%ytiles;
      int xstart = xtile * xtilesize;
      int xend = (xtile+1) * xtilesize;

      if(xend > xres)
        xend = xres;

      int ystart = ytile * ytilesize;
      int yend = (ytile+1) * ytilesize;

      if(yend > yres)
        yend = yres;

#ifdef MANTA_SSE
      __m128i vec_4 = _mm_set1_epi32(4);
      __m128i vec_cascade = _mm_set_epi32(3, 2, 1, 0);
#endif
      // Create a Fragment that is consecutive in X pixels
      switch (shape) {
      case Fragment::LineShape:
      {
        Fragment &frag = *fragBuffer[context.proc].frag;
        int fsize = Min(Fragment::MaxSize, xend-xstart);
        for(int eye = 0; eye < numEyes; eye++){
#ifdef MANTA_SSE
              // This finds the upper bound of the groups of 4.  Even if you
              // write more than you need (say 8 for a fragment size of 6),
              // you won't blow the top of the array, because
              // (Fragment::MaxSize+3)&(~3) == Fragment::MaxSize.
              int e = (fsize+3)&(~3);
              __m128i vec_eye = _mm_set1_epi32(eye);
              for(int i=0;i<e;i+=4)
                  _mm_store_si128((__m128i*)&frag.whichEye[i], vec_eye);
#else
              for(int i=0;i<fsize;i++)
                  frag.whichEye[i] = eye;
#endif

              // Two versions.  If the assignment is narrower than a fragment, we
              // can enable a few optimizations
              if(xend-xstart <= Fragment::MaxSize){
                  // Common case - one packet in X direction
                  int size = xend-xstart;
#ifdef MANTA_SSE
                  __m128i vec_x = _mm_add_epi32(_mm_set1_epi32(xstart), vec_cascade);
                  for(int i=0;i<size;i+=4){
                      // This will spill over by up to 3 pixels
                      _mm_store_si128((__m128i*)&frag.pixel[0][i], vec_x);
                      vec_x = _mm_add_epi32(vec_x, vec_4);
                  }
#else
                  for(int i=0;i<size;i++)
                      frag.pixel[0][i] = i+xstart;
#endif
                  frag.setSize(size);
                  for(int y = ystart; y<yend; y++){
#ifdef MANTA_SSE
                      int e = (fsize+3)&(~3);
                      __m128i vec_y = _mm_set1_epi32(y);
                      for(int i=0;i<e;i+=4)
                          _mm_store_si128((__m128i*)&frag.pixel[1][i], vec_y);
#else
                      for(int i=0;i<fsize;i++)
                          frag.pixel[1][i] = y;
#endif
                      context.rng->seed(xstart*xres+y);
                      context.pixelSampler->renderFragment(context, frag);
                      writeMPIFragment(context, image);
                  }
              } else {
                  // General case (multiple packets in X direction)
                  for(int y = ystart; y<yend; y++){
#ifdef MANTA_SSE
                      int e = (fsize+3)&(~3);
                      __m128i vec_y = _mm_set1_epi32(y);
                      for(int i=0;i<e;i+=4)
                          _mm_store_si128((__m128i*)&frag.pixel[1][i], vec_y);
#else
                      for(int i=0;i<fsize;i++)
                          frag.pixel[1][i] = y;
#endif
                      for(int x = xstart; x<xend; x+= Fragment::MaxSize){
                          // This catches cases where xend-xstart is larger than
                          // Fragment::MaxSize.
                          int xnarf = x+Fragment::MaxSize;
                          if (xnarf > xend) xnarf = xend;
                          int size = xnarf-x;
#ifdef MANTA_SSE
                          __m128i vec_x = _mm_add_epi32(_mm_set1_epi32(x), vec_cascade);
                          for(int i=0;i<size;i+=4){
                              // This will spill over by up to 3 pixels
                              _mm_store_si128((__m128i*)&frag.pixel[0][i], vec_x);
                              vec_x = _mm_add_epi32(vec_x, vec_4);
                          }
#else
                          for(int i=0;i<size;i++)
                              frag.pixel[0][i] = i+x;
#endif
                          frag.setSize(size);
                          context.rng->seed(x*xres+y);
                          context.pixelSampler->renderFragment(context, frag);

                          writeMPIFragment(context, image);
                      }
                  }
              }
          }
      }
      break;
      case Fragment::SquareShape:
      {
        // Square Shaped fragments of about RayPacket::MaxSize each
        static const int sqrt_size = static_cast<int>(Sqrt(RayPacket::MaxSize));
        static const int full_size = sqrt_size * sqrt_size;

        for(int eye = 0; eye < numEyes; eye++){

          if (numEyes > 1) {
#if 0
            //implement me!
            for(int i=0;i<full_size;i++)
              fragBuffer[context.proc].frag->whichEye[i] = eye;
            fragBuffer[context.proc].swap();
            for(int i=0;i<full_size;i++)
              fragBuffer[context.proc].frag->whichEye[i] = eye;
#endif
          }

          for (int y = ystart; y < yend; y += sqrt_size) {
            for (int x = xstart; x < xend; x += sqrt_size) {
              Fragment &frag = *fragBuffer[context.proc].frag;

              int j_end = Min(yend - y, sqrt_size);
              int i_end = Min(xend - x, sqrt_size);

              for (int j = 0; j < j_end; ++j) {
                for (int i = 0; i < i_end; ++i) {
                  frag.pixel[0][j*i_end + i] = x + i;
                  frag.pixel[1][j*i_end + i] = y + j;
                }
              }
              // NOTE(boulos): If these get clipped, it's not
              // actually Square (but it is a
              // Rectangle... maybe we should add that?)
              if (i_end != sqrt_size ||
                  j_end != sqrt_size) {
                frag.shape = Fragment::UnknownShape;
              }
              frag.setSize(j_end * i_end);
              context.rng->seed(x*xres+y);
              context.pixelSampler->renderFragment(context, frag);

              writeMPIFragment(context, image);
            }
          }
        }
      }
      break;
      default:
        break;
      }
#ifdef ENABLE_STATS
      tilesRendered++;
#endif
    }
#ifdef ENABLE_STATS
    startAssignmentTime = MPI::Wtime();
    const double renderTime = startAssignmentTime - endTime;
    minTileRenderTime = min(minTileRenderTime, renderTime);
    maxTileRenderTime = max(maxTileRenderTime, renderTime);
    averageRenderTime+=renderTime;
#endif
  }

  if (rank > 1) {
    // Finish writing out remaining buffers.
    if (fragBuffer[context.proc].numFrags() > 0) {
      fragBuffer[context.proc].fillRemainingWithNullFrags();
      writeMPIFragment(context, image);
    }
  }

#ifdef ENABLE_STATS
  double endtime = MPI::Wtime();
  double finalTime = 1000*(endtime-starttime);

  averageRenderTime/=numGets;
//   printf("%d/%d has stats: %f %f %f\n", rank, context.proc, minTileRenderTime*1000,
//          maxTileRenderTime*1000, averageRenderTime*1000);

//   PRINTDBG printf("%d %d is out of render loop\n", rank, context.proc);
#endif

  if (rank > 1) {
    barrier.wait(context.numProcs);
    // This clears it
    fragBuffer[context.proc].reset();

    if (context.proc == 0) {
      // Inform the master that this rank is done.
      fragBuffer[context.proc].fillRemainingWithNullFrags();
      writeMPIFragment(context, image);
    }
  }

  fragBuffer[context.proc].reset();

  //PRINTDBG
  //printf("%d/%d took %fms to finish all its work.\n", rank, context.proc, 1000*(MPI::Wtime()-starttime));
//   if (rank==1&&context.proc==0)
//     PRINTDBG printf("\n");

  if(context.proc == 0)
    image->setValid(true);

#ifdef ENABLE_STATS
//   endtime = MPI::Wtime();
//   finalTime = 1000*(endtime-starttime);
//   if (finalTime > 30) {
//     PRINTDBG printf("%d/%d took %fms to finish %d tiles got in %d gets for final final time.\n",
//            rank, context.proc, finalTime, tilesRendered, numGets);
//   }
#endif
}
