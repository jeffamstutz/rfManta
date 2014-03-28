
#include <Engine/ImageTraversers/DissolveImageTraverser.h>
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

ImageTraverser* DissolveImageTraverser::create(const vector<string>& args)
{
  return new DissolveImageTraverser(args);
}

DissolveImageTraverser::DissolveImageTraverser(const vector<string>& args):
  iters_per_frame(16)
{
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-itersPerFrame"){
      if(!getArg<unsigned int>(i, args, iters_per_frame))
	throw IllegalArgument("DissolveImageTraverser -itersPerFrame",
                              i, args);
      if (iters_per_frame < 1)
        throw IllegalArgument("-itersPerFrame must be greater than 0",
                              i, args);
    } else {
      throw IllegalArgument("DissolveImageTraverser", i, args);
    }
  }
}

DissolveImageTraverser::~DissolveImageTraverser()
{
}

void DissolveImageTraverser::setupBegin(SetupContext& context, int numChannels)
{
  per_thread_data.resize(context.numProcs);
  for(size_t i = 0; i < per_thread_data.size(); i++)
    per_thread_data[i].next_pixel.resize(numChannels);
  channel_data.resize(numChannels);

  //  context.loadBalancer->setupBegin(context, numChannels);
  context.pixelSampler->setupBegin(context, numChannels);
}

void DissolveImageTraverser::setupDisplayChannel(SetupContext& context)
{
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);

  // Try an educated guess for the pixels per sample
  pixels_per_pass = (xres*yres)/context.numProcs/iters_per_frame;

  // Compute the mask
  unsigned int num_x_bits = numBinaryDigits(xres-1);
  unsigned int num_y_bits = numBinaryDigits(yres-1);
  unsigned int num_all_bits = num_x_bits + num_y_bits;
  if (num_all_bits > 32)
    throw InternalError("DissolveImageTraverser::setupDisplayChannel::number of bits needed for random number exceeds 32");

  ChannelContext& cdata = channel_data[context.channelIndex];
  cdata.mask = MaskValues[num_all_bits];
  cdata.num_y_bits = num_y_bits;
  cdata.y_bit_mask = (1 << num_y_bits) - 1;
  
  // Reset the starting pixel
  unsigned int next_pixel = 1;
  per_thread_data[0].next_pixel[context.channelIndex] = next_pixel;
  for(size_t i = 1; i < per_thread_data.size(); i++) {
    int x,y;
    do {
      computeNextSample(next_pixel, cdata.mask);
      x = next_pixel >> cdata.num_y_bits;
      y = next_pixel &  cdata.y_bit_mask;
    } while (x >= xres || y >= yres);
    per_thread_data[i].next_pixel[context.channelIndex] = next_pixel;
  }
  
  // Single bufferd, please.
  context.constrainPipelineDepth(1,1);

  //  context.loadBalancer->setupDisplayChannel(context, numAssignments);
  context.pixelSampler->setupDisplayChannel(context);
}

void DissolveImageTraverser::setupFrame(const RenderContext& context)
{
  //  context.loadBalancer->setupFrame(context);
  context.pixelSampler->setupFrame(context);
}

void DissolveImageTraverser::renderImage(const RenderContext& context,
                                         Image* image)
{
  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);

  // Pull out needed parameters for this frame based on the channel
  // and the processor index.
  unsigned int next_pixel = per_thread_data[context.proc].next_pixel[context.channelIndex];
  ChannelContext& cdata = channel_data[context.channelIndex];

  Fragment frag(Fragment::UnknownShape, Fragment::ConstantEye);
  unsigned int pixel = 0;
  // We need to render the (0,0) pixel every once in a while, so see
  // if we are the last processor and if we've gone through so many
  // frames.
  if (context.proc == context.numProcs-1 &&
      context.frameState->frameSerialNumber%iters_per_frame == 0) {
    frag.addElement(0,0,0);
    // In order to make sure the compute the same number of pixels per
    // iteration pixel should not be incremented.
    pixel++;
  }
  while ( pixel < pixels_per_pass ) {
    int x = next_pixel >> cdata.num_y_bits;
    int y = next_pixel &  cdata.y_bit_mask;
    if (x < xres && y < yres) {
      // This is a good pixel so let's add it.
      pixel++;
      frag.addElement(x, y, 0);
      if(stereo){
        // I'm not sure you want to do stereo
      }
      // Check to see if your fragment is full.  If it is, then render it.
      if (frag.end() == Fragment::MaxSize) {
        context.pixelSampler->renderFragment(context, frag);
#if 0
        for(int f = 0; f < frag.getSize(); f++)
          switch (context.proc) {
          case 0: frag.setColor(f, Color(RGBColor(1,0,0))); break;
          case 1: frag.setColor(f, Color(RGBColor(0,0,1))); break;
          case 2: frag.setColor(f, Color(RGBColor(0,1,0))); break;
          case 3: frag.setColor(f, Color(RGBColor(1,1,0))); break;
          }
#endif
        image->set(frag);
        // Reset the Fragment, so we can start filling it up again.
        frag.resetSize();
      }
    }
    // Generate the next pixel.  But we need to find the Nth good
    // pixel, so only count pixels if we find them.
    for(int i = 0; i < context.numProcs;) {
      computeNextSample(next_pixel, cdata.mask);
      int x = next_pixel >> cdata.num_y_bits;
      int y = next_pixel &  cdata.y_bit_mask;
      // Found a good pixel, so find the next one.
      if (x < xres && y < yres) i++;
    }
  }
  // Pick up strays
  if (frag.end() != 0) {
    context.pixelSampler->renderFragment(context, frag);
    image->set(frag);
  }

  // Stash out next_pixel for the next iteration of the frame.
  per_thread_data[context.proc].next_pixel[context.channelIndex] = next_pixel;

  // This can potentially happen before the other procesors are finished
  // rendering, but that is okay because it won't get displayed until
  // everyone enters the barrier anyway
  if(context.proc == 0)
    image->setValid(true);
}
