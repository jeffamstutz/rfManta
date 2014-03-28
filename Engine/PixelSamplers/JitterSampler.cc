
#include <Core/Exceptions/IllegalArgument.h>
#include <Engine/PixelSamplers/JitterSampler.h>
#include <Core/Util/Args.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
#include <Core/Math/CheapRNG.h>
#include <Core/Math/MiscMath.h>

using namespace Manta;

#include <iostream>
using namespace std;

PixelSampler* JitterSampler::create(const vector<string>& args)
{

  return new JitterSampler(args);
}

JitterSampler::JitterSampler(const vector<string>& args):
  num_samples(4), use_cheaprng(true), random(0)
{
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-numberOfSamples"){
      if(!getIntArg(i, args, num_samples))
        throw IllegalArgument("JitterSampler -numberOfSamples", i, args);
      if (num_samples < 1)
        throw IllegalArgument("-numberOfSamples must be greater than 0",
                              i, args);
    } else if (arg == "-nocheap") {
      use_cheaprng = false;
    }

    else {
      throw IllegalArgument("JitterSampler", i, args);
    }
  }

  // Compute nx and ny
  findFactorsNearRoot(num_samples, nx, ny);
}

JitterSampler::~JitterSampler()
{
  if (random) delete[] random;
}

void JitterSampler::setupBegin(const SetupContext& context, int numChannels)
{
  channelInfo.resize(numChannels);
  if (random) delete[] random;
  random = new MT_RNG[context.numProcs];
  context.renderer->setupBegin(context, numChannels);
  context.sample_generator->setupBegin(context, numChannels);
}

void JitterSampler::setupDisplayChannel(SetupContext& context)
{
  ChannelInfo& ci = channelInfo[context.channelIndex];
  bool stereo;
  context.getResolution(stereo, ci.xres, ci.yres);

  // Set up the scale from -1 to 1
  ci.xscale = (Real)2/ci.xres;
  ci.yscale = (Real)2/ci.yres;
  ci.xoffset = (-ci.xres+1)*(Real)0.5*ci.xscale; // Offset to pixel center
  ci.yoffset = (-ci.yres+1)*(Real)0.5*ci.yscale;

  context.renderer->setupDisplayChannel(context);
  context.sample_generator->setupDisplayChannel(context);
}

void JitterSampler::setupFrame(const RenderContext& context)
{
  context.renderer->setupFrame(context);
  context.sample_generator->setupFrame(context);
}

// The fragment, sample_color, and sample_color_size paramters are
// obvious enough, but the other two need some explaination.
//
// samples_collected_return represent the number of samples that have
// already been used to color a fragment element.  If a fragment
// element's samples span more than one RayPacket, then we account for
// that with this variable.
//
// current_fragment_return keeps track of which fragment elements have
// had their color computed.
void JitterSampler::computeAverages(Fragment& fragment, const RayPacket& rays,
                                    int& samples_collected_return,
                                    int& current_fragment_return) {
  // Copy the values, so the we can hopefully use the cache when
  // assigning the values.
  int samples_collected = samples_collected_return;
  int current_fragment = current_fragment_return;
  Color fragment_color = Color::black();
  for(int sample_index = rays.begin(); sample_index < rays.end(); sample_index++) {
    fragment_color += rays.getColor(sample_index);
    samples_collected++;
    // We've collected enough samples, so compute the average and
    // assign it to the fragment.
    if(samples_collected == num_samples) {
      fragment.addColor(current_fragment, fragment_color / num_samples);
      // Reset our accumulation variables
      fragment_color = Color::black();
      samples_collected = 0;
      current_fragment++;
    }
  }
  // Pick up left over fragments.  It is OK if this is a partial sum.
  // The remaining sum will be picked up in the next call to
  // computeAverages.
  if (samples_collected > 0)
    fragment.addColor(current_fragment, fragment_color / num_samples);

  // Assign the values back to the parameters.
  samples_collected_return = samples_collected;
  current_fragment_return = current_fragment;
}

void JitterSampler::renderFragment(const RenderContext& context,
                                   Fragment& fragment)
{
  ChannelInfo& ci = channelInfo[context.channelIndex];
  int thd_num = context.proc;

  int flags = RayPacket::HaveImageCoordinates;
  if(fragment.getFlag(Fragment::ConstantEye))
    flags |= RayPacket::ConstantEye;

  int consecutivex_flag = fragment.getFlag(Fragment::ConsecutiveX);

  CheapRNG rng;
  if (consecutivex_flag) {
    int b = fragment.begin();
    if (use_cheaprng)
      rng.seed(fragment.getX(b)*ci.xres+fragment.getY(b));
    else
      random[thd_num].seed(fragment.getX(b)*ci.xres+fragment.getY(b));
  }
  else if (use_cheaprng)
    rng.seed(0); //need to initialize rng.

  int depth = 0;
  RayPacketData raydata;
  RayPacket rays(raydata, RayPacket::UnknownShape, 0, RayPacket::MaxSize, depth, flags);

  Real inx = (Real)1/nx;
  Real iny = (Real)1/ny;
  Real px, py;

  int sample_count = 0;
  // No samples accumulated yet.
  int samples_collected = 0;
  // Index to the first fragment's element.
  int current_fragment = 0;

  Packet<Real> r1;
  Packet<Real> r2;
  if (use_cheaprng) {
    rng.nextPacket(r1, rays);
    rng.nextPacket(r2, rays);
  } else {
    random[thd_num].nextPacket(r1, rays);
    random[thd_num].nextPacket(r2, rays);
  }

  // We can compute at most RayPacket::MaxSize number of rays at time.
  for(int frag_index = fragment.begin(); frag_index < fragment.end(); frag_index++) {
    // Initialize the color
    fragment.setColor(frag_index, Color::black());

    if (!consecutivex_flag) {
      if (use_cheaprng)
        rng.seed(fragment.getX(frag_index)*ci.xres+fragment.getY(frag_index));
      else
        random[thd_num].seed(fragment.getX(frag_index)*ci.xres+fragment.getY(frag_index));
    }

    // For each fragment start filling up the RayPacket with samples.
    // When you filled up the empty_slots compute the rays, do the
    // average, and store the results in the fragment.
    int fx = fragment.getX(frag_index);
    int fy = fragment.getY(frag_index);
    int pixel_id = fy * ci.xres + fx;
    // Which sample does this ray packet start at?

    for(int xs = 0; xs < nx; xs++) {
      for(int ys = 0; ys  < ny; ys++) {
          Real x_sample = (xs + r1.get(sample_count)) * inx;
          Real y_sample = (ys + r2.get(sample_count)) * iny;

          px = (fx+x_sample)*ci.xscale+ci.xoffset;
          py = (fy+y_sample)*ci.yscale+ci.yoffset;
          rays.setPixel(sample_count, 0, px, py);
          rays.data->sample_id[sample_count] = xs * ny + ys;
          rays.data->region_id[sample_count] = pixel_id;
          sample_count++;

          if (sample_count == RayPacket::MaxSize) {
            // Filled up the ray packet, so send them off!
            rays.resize(sample_count);
            context.sample_generator->setupPacket(context, rays);
            context.renderer->traceEyeRays(context, rays);

            computeAverages(fragment, rays,
                            samples_collected, current_fragment);

            // Now reset the index, so that we can start filling up
            // the RayPacket again.
            sample_count = 0;

            // Make sure we start with a fresh slate
            rays.resetHits();
            rays.setAllFlags(flags);

            // Grab more samples
            if (use_cheaprng) {
              rng.nextPacket(r1, rays);
              rng.nextPacket(r2, rays);
            } else {
              random[thd_num].nextPacket(r1, rays);
              random[thd_num].nextPacket(r2, rays);
            }
          } // end packet shoot and reset test
      }
    } // end sample filling loops
  } // end fragment loop

  // Pick up any stragling samples
  if (sample_count > 0) {
    rays.resize(sample_count);
    context.sample_generator->setupPacket(context, rays);
    context.renderer->traceEyeRays(context, rays);

    computeAverages(fragment, rays,
                    samples_collected, current_fragment);
  }
}
