
#include <Engine/PixelSamplers/FastSampler.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
using namespace Manta;

PixelSampler* FastSampler::create(const vector<string>& args)
{
  return new FastSampler(args);
}

FastSampler::FastSampler(const vector<string>& /* args */)
{
}

FastSampler::~FastSampler()
{
}

void FastSampler::setupBegin(const SetupContext& context, int numChannels)
{
  channelInfo.resize(numChannels);
  context.renderer->setupBegin(context, numChannels);
  context.sample_generator->setupBegin(context, numChannels);
}

void FastSampler::setupDisplayChannel(SetupContext& context)
{
  ChannelInfo& ci = channelInfo[context.channelIndex];
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);

  // Set up the scale from -1 to 1
  ci.xscale = (Real)2/xres;
  ci.yscale = ci.xscale;
  ci.xoffset = (-xres/(Real)2+(Real)0.5)*ci.xscale; // Offset to pixel center
  ci.yoffset = (-yres/(Real)2+(Real)0.5)*ci.yscale;
  context.renderer->setupDisplayChannel(context);
  context.sample_generator->setupDisplayChannel(context);
}

void FastSampler::setupFrame(const RenderContext& context)
{
  context.renderer->setupFrame(context);
  context.sample_generator->setupFrame(context);
}

void FastSampler::renderFragment(const RenderContext& context,
                                   Fragment& fragment)
{
  ChannelInfo& ci = channelInfo[context.channelIndex];
  int flags = RayPacket::HaveImageCoordinates;
  if(fragment.getFlag(Fragment::ConstantEye))
    flags |= RayPacket::ConstantEye;
  for(int f=fragment.begin();f<fragment.end();f+=RayPacket::MaxSize){
    // We want to fill our RayPacket with as many as
    // RayPacket::MaxSize rays.
    int size = RayPacket::MaxSize;
    if(size >= fragment.end()-f)
      // We don't have enough fragments left to fill a ray packet, so
      // set the size of the RayPacket to the number of fragments we
      // have left.
      size = fragment.end()-f;
    // Create a ray packet
    int depth = 0;
    RayPacketData raydata;
    RayPacket rays(raydata, RayPacket::UnknownShape, 0, size, depth, flags);

    // Check to see if the fragment is consecutive in x.
    if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)){

      // If so place each pixel in the ray packet relative to the first
      // fragment.
      int b = fragment.begin();
      Real px = fragment.getX(b)*ci.xscale+ci.xoffset;
      Real py = fragment.getY(b)*ci.yscale+ci.yoffset;
      int eye = fragment.getWhichEye(b);

      for(int i=fragment.begin();i<fragment.end();i++){
        rays.setPixel(i, eye, px, py);
        px += ci.xscale;
      }

    }

    // Otherwise, set each pixel individually.
    else {
      for(int i=fragment.begin();i<fragment.end();i++){
        Real px = fragment.getX(i)*ci.xscale+ci.xoffset;
        Real py = fragment.getY(i)*ci.yscale+ci.yoffset;
        rays.setPixel(i, fragment.getWhichEye(i), px, py);
      }
    }

    // Trace the rays.  The results will automatically go into the fragment
    context.renderer->traceEyeRays(context, rays);

    for(int i=0;i<size;i++)
    {
        for ( int c = 0; c < Color::NumComponents; c++ )
            fragment.color[c][i] = raydata.color[c][i];
    }
  }
}
