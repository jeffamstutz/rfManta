
#include <Engine/ImageTraversers/DissolveTiledImageTraverser.h>
#include <Core/Math/CheapRNG.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Util/Args.h>
#include <Interface/Context.h>
#include <Interface/FrameState.h>
#include <Interface/Fragment.h>
#include <Interface/Image.h>
#include <Interface/LoadBalancer.h>
#include <Interface/PixelSampler.h>
#include <Core/Thread/Mutex.h>
#include <Core/Util/NotFinished.h>

using namespace Manta;

// Use this mask to produce values from 1 to N (in the comments)
static unsigned int MaskValues[] = {
  /* 00 */       0x00,  //             0
  /* 01 */       0x01,  //             1
  /* 02 */       0x03,  //             3
  /* 03 */       0x06,  //             7
  /* 04 */       0x0C,  //            15
  /* 05 */       0x14,  //            31
  /* 06 */       0x30,  //            63
  /* 07 */       0x60,  //           127
  /* 08 */       0xB8,  //           255
  /* 09 */     0x0110,  //           511
  /* 10 */     0x0240,  //         1,023
  /* 11 */     0x0500,  //         2,047
  /* 12 */     0x0CA0,  //         4,095
  /* 13 */     0x1B00,  //         8,191
  /* 14 */     0x3500,  //        16,383
  /* 15 */     0x6000,  //        32,767
  /* 16 */     0xB400,  //        65,535
  /* 17 */ 0x00012000,  //       131,071
  /* 18 */ 0x00020400,  //       262,143
  /* 19 */ 0x00072000,  //       524,287
  /* 20 */ 0x00090000,  //     1,048,575
  /* 21 */ 0x00140000,  //     2,097,151
  /* 22 */ 0x00300000,  //     4,194,303
  /* 23 */ 0x00420000,  //     8,388,607
  /* 24 */ 0x00D80000,  //    16,777,215
  /* 25 */ 0x01200000,  //    33,554,431
  /* 26 */ 0x03880000,  //    67,108,863
  /* 27 */ 0x07200000,  //   134,217,727
  /* 28 */ 0x09000000,  //   268,435,575
  /* 29 */ 0x14000000,  //   536,870,911
  /* 30 */ 0x32800000,  // 1,073,741,823
  /* 31 */ 0x48000000,  // 2,147,483,647
  /* 32 */ 0xA3000000   // 4,294,967,295
};

ImageTraverser* DissolveTiledImageTraverser::create(const vector<string>& args)
{
  return new DissolveTiledImageTraverser(args);
}

DissolveTiledImageTraverser::DissolveTiledImageTraverser(const vector<string>& args):
  xtilesize(32), ytilesize(2), iters_per_frame(16)
{
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-itersPerFrame"){
      if(!getArg<unsigned int>(i, args, iters_per_frame))
	throw IllegalArgument("DissolveTiledImageTraverser -itersPerFrame",
                              i, args);
      if (iters_per_frame < 1)
        throw IllegalArgument("-itersPerFrame must be greater than 0",
                              i, args);
    } else if(arg == "-tilesize"){
      if(!getResolutionArg(i, args, xtilesize, ytilesize))
        throw IllegalArgument("TiledImageTraverser -tilesize", i, args);
    } else {
      throw IllegalArgument("DissolveTiledImageTraverser", i, args);
    }
  }
}

DissolveTiledImageTraverser::~DissolveTiledImageTraverser()
{
}

void DissolveTiledImageTraverser::setupBegin(SetupContext& context, int numChannels)
{
  per_thread_data.resize(context.numProcs);
  for(size_t i = 0; i < per_thread_data.size(); i++) {
    per_thread_data[i].channel_data.resize(numChannels);
  }

  context.pixelSampler->setupBegin(context, numChannels);
}

void DissolveTiledImageTraverser::setupDisplayChannel(SetupContext& context)
{
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);

  // Figure out how many tiles we have
  unsigned int xtiles = (xres + xtilesize-1)/xtilesize;
  unsigned int ytiles = (yres + ytilesize-1)/ytilesize;
  unsigned int numAssignments = xtiles * ytiles;
  unsigned int numTiles = numAssignments/context.numProcs;
  unsigned int numTiles_leftover = numAssignments%context.numProcs;

  // We need to seed each thread with a different seed point randomly
  // to give it a more mixed up look.
  CheapRNG rng;
  rng.seed(1);
  
  // Try an educated guess for the tiles per pass
  for(size_t i = 0; i < per_thread_data.size(); ++i) {
    ChannelContext& cdata = per_thread_data[i].channel_data[context.channelIndex];
    cdata.numTiles = numTiles;
    // Add extra assignments to the latter processors
    if (per_thread_data.size()-i >= numTiles_leftover)
      cdata.numTiles++;
    cdata.tiles_per_pass = cdata.numTiles/iters_per_frame;
    // Inialize the random number stuff.  Since we are using the
    // modulous we need to add 1 to get the range from [0,numTiles-1]
    // to [1,numTiles].
    cdata.next_tile = rng.nextInt()%cdata.numTiles + 1;
    // Figure out the mask
    int num_bits = numBinaryDigits(cdata.numTiles);
    if (num_bits > 32)
      throw InternalError("DissolveTiledImageTraverser::setupDisplayChannel::number of bits needed for random number exceeds 32");
    cdata.rng_mask = MaskValues[num_bits];
  }

  // Single bufferd, please.
  context.constrainPipelineDepth(1,1);

  //  context.loadBalancer->setupDisplayChannel(context, numAssignments);
  context.pixelSampler->setupDisplayChannel(context);
}

void DissolveTiledImageTraverser::setupFrame(const RenderContext& context)
{
  //  context.loadBalancer->setupFrame(context);
  context.pixelSampler->setupFrame(context);
}

void DissolveTiledImageTraverser::renderImage(const RenderContext& context,
                                         Image* image)
{
  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);
  int ytiles = (yres + ytilesize-1)/ytilesize;

  // Pull out this variable for convience.
  ChannelContext& cdata = per_thread_data[context.proc].channel_data[context.channelIndex];

  unsigned int next_tile = cdata.next_tile;

  Fragment frag(Fragment::UnknownShape, Fragment::ConstantEye);
  
  // Loop over all the assignments.  tc stands for tile count.
  for(unsigned int tc = 0; tc < cdata.tiles_per_pass; ++tc) {
    // Get the next assignment.
    unsigned int tile = (next_tile-1)*context.numProcs + context.proc;
    // Get the geometry of the tile
    int xtile = tile/ytiles;
    int ytile = tile%ytiles;
    int xstart = xtile * xtilesize;
    int xend = (xtile+1) * xtilesize;
    if(xend > xres)
      xend = xres;
    int ystart = ytile * ytilesize;
    int yend = (ytile+1) * ytilesize;
    if(yend > yres)
      yend = yres;
    for(int y = ystart; y<yend; ++y) {
      for(int x = xstart; x<xend; ++x) {
        // Add the fragment
        frag.addElement(x, y, 0);
        if(stereo){
          // I'm not sure you want to do stereo
        }
        // Check to see if your fragment is full.  If it is, then render it.
        if (frag.end() == Fragment::MaxSize) {
//           // It might be useful to check for ConsecutiveX, but only if
//           // certain conditions exist.
//           if ((xend-xstart) == Fragment::MaxSize)
//             frag.testSetConsecutiveX();
          // Render the fragments
          context.pixelSampler->renderFragment(context, frag);
          image->set(frag);
          // Reset the Fragment, so we can start filling it up again.
          frag.resetSize();
          frag.setAllFlags(Fragment::ConstantEye);
        }
      }
    } // end looping over the tile
    
    // Get the next random number, skipping values that aren't within
    // our range.
    do {
      computeNextSample(next_tile, cdata.rng_mask);
    } while (next_tile > cdata.numTiles);
    
  }
  // Pick up strays
  if (frag.end() != 0) {
    context.pixelSampler->renderFragment(context, frag);
    image->set(frag);
  }
  // Save the next tile for the next iteration.
  cdata.next_tile = next_tile;
  
  // This can potentially happen before the other procesors are finished
  // rendering, but that is okay because it won't get displayed until
  // everyone enters the barrier anyway
  if(context.proc == 0)
    image->setValid(true);
}
