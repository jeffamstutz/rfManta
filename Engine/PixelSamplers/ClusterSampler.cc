
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Math/MiscMath.h>
#include <Core/Util/Args.h>
#include <Engine/PixelSamplers/ClusterSampler.h>
#include <Engine/SampleGenerators/Stratified2D.h>
#include <Engine/SampleGenerators/UniformRandomGenerator.h>
#include <Interface/Camera.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/Object.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
#include <Interface/Scene.h>

using namespace Manta;

#include <iostream>
using namespace std;

PixelSampler* ClusterSampler::create(const vector<string>& args)
{
  return new ClusterSampler(args);
}

ClusterSampler::ClusterSampler(const vector<string>& args):
  num_samples(64)
{
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-numberOfSamples"){
      if(!getIntArg(i, args, num_samples))
        throw IllegalArgument("ClusterSampler -numberOfSamples", i, args);
      if (num_samples < 1)
        throw IllegalArgument("-numberOfSamples must be greater than 0",
                              i, args);
    }

    else {
      throw IllegalArgument("ClusterSampler", i, args);
    }
  }

  // Compute nx and ny
  findFactorsNearRoot(num_samples, nx, ny);
  min_shading_samples = 16;
  maxSqrt = Ceil(Sqrt(static_cast<Real>(num_samples)));
  //cerr << "maxSqrt = " << maxSqrt;
  antialiasGenerator = new Stratified2D(num_samples);
  //antialiasGenerator = new UniformRandomGenerator();
  sampleGenerators = new SampleGenerator*[maxSqrt];
  for (int i = 0; i < maxSqrt; i++) {
    int samples = (i+1) * (i+1);
    sampleGenerators[i] = new Stratified2D(samples);
    //sampleGenerators[i] = new UniformRandomGenerator();
  }
}

ClusterSampler::~ClusterSampler()
{
  delete antialiasGenerator;
  for (int i = 0; i < maxSqrt; i++) {
    delete sampleGenerators[i];
  }
  delete[] sampleGenerators;
}

void ClusterSampler::setupBegin(const SetupContext& context, int numChannels)
{
  channelInfo.resize(numChannels);
  context.renderer->setupBegin(context, numChannels);
  context.sample_generator->setupBegin(context, numChannels);
  antialiasGenerator->setupBegin(context, numChannels);
  for (int i = 0; i < maxSqrt; i++) {
    sampleGenerators[i]->setupBegin(context, numChannels);
  }
}

void ClusterSampler::setupDisplayChannel(SetupContext& context)
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
  antialiasGenerator->setupDisplayChannel(context);
  for (int i = 0; i < maxSqrt; i++) {
    sampleGenerators[i]->setupDisplayChannel(context);
  }
}

void ClusterSampler::setupFrame(const RenderContext& context)
{
  context.renderer->setupFrame(context);
  context.sample_generator->setupFrame(context);
  antialiasGenerator->setupFrame(context);
  for (int i = 0; i < maxSqrt; i++) {
    sampleGenerators[i]->setupFrame(context);
  }
}

void ClusterSampler::renderFragment(const RenderContext& context,
                                   Fragment& fragment)
{
  ChannelInfo& ci = channelInfo[context.channelIndex];

  int flags = RayPacket::HaveImageCoordinates;
  if(fragment.getFlag(Fragment::ConstantEye))
    flags |= RayPacket::ConstantEye;

  int depth = 0;
  RayPacketData raydata;
  RayPacket rays(raydata, RayPacket::UnknownShape, 0, RayPacket::MaxSize, depth, flags);

  RayPacketData shade_data;
  RayPacket shading_rays(shade_data, RayPacket::UnknownShape, 0, RayPacket::MaxSize, depth, flags);

  Real inx = (Real)1/nx;
  Real iny = (Real)1/ny;
  Real inv_samples = inx * iny;
  Real px, py;

  RenderContext& mutable_context = const_cast<RenderContext&>(context);
  SampleGenerator* original_samplegen = context.sample_generator;

  for (int frag_index = fragment.begin(); frag_index < fragment.end(); frag_index++) {
    int fx = fragment.getX(frag_index);
    int fy = fragment.getY(frag_index);
    int pixel_id = fy * ci.xres + fx;

    rays.resize(nx * ny);
    // NOTE(boulos): Setup region_id first
    int sample_count = 0;
    for(int xs = 0; xs < nx; xs++) {
      for(int ys = 0; ys < ny; ys++) {
        rays.data->sample_id[sample_count] = sample_count;
        rays.data->region_id[sample_count] = pixel_id;
        sample_count++;
      }
    }

    mutable_context.sample_generator = antialiasGenerator;
    context.sample_generator->setupPacket(context, rays);
    Packet<Real> r1;
    Packet<Real> r2;
    context.sample_generator->nextSeeds(context, r1, rays);
    context.sample_generator->nextSeeds(context, r2, rays);
    sample_count = 0;

    for(int xs = 0; xs < nx; xs++) {
      for(int ys = 0; ys < ny; ys++) {
        Real x_sample = (xs + r1.get(sample_count)) * inx;
        Real y_sample = (ys + r2.get(sample_count)) * iny;
        px = (fx+x_sample)*ci.xscale+ci.xoffset;
        py = (fy+y_sample)*ci.yscale+ci.yoffset;
        rays.setPixel(sample_count, 0, px, py);
        sample_count++;
      }
    }

    // Copied from rayTracer traceEyeRays
    context.camera->makeRays(context, rays);
    rays.initializeImportance();

    rays.resetHits();
    rays.setAllFlags(flags);
    context.scene->getObject()->intersect(context, rays);

    // Build clusters and determine how many shading rays are needed.
    int uniquePrims = 0;
    bool visited[RayPacket::MaxSize] = {false};
    int counts[RayPacket::MaxSize] = {0};
    int prim_id[RayPacket::MaxSize];
    const Primitive* prims[RayPacket::MaxSize];

    for (int i = rays.begin(); i < rays.end(); i++) {
      if (visited[i]) continue;
      const Primitive* prim = rays.getHitPrimitive(i);
      for (int j = i+1; j < rays.end(); j++) {
        if (rays.getHitPrimitive(j) == prim) {
          visited[j] = true;
          prim_id[j] = uniquePrims;
          counts[uniquePrims]++;
        }
      }
      visited[i] = true;
      prim_id[i] = uniquePrims;
      counts[uniquePrims]++;
      prims[uniquePrims] = prim;
      uniquePrims++;
    }

    int neededRays = Max(min_shading_samples, uniquePrims);
    int sqrtRays = Ceil(Sqrt(static_cast<Real>(neededRays)));
    int realNumRays = sqrtRays * sqrtRays;

    //if (sqrtRays < 1) throw InternalError("Error SqrtRays is < 1");
    mutable_context.sample_generator = sampleGenerators[sqrtRays - 1];
    shading_rays.resize(realNumRays);
    sample_count = 0;
    // Again setup the region id first.
    for(int xs = 0; xs < sqrtRays; xs++) {
      for(int ys = 0; ys < sqrtRays; ys++) {
        shading_rays.data->sample_id[sample_count] = sample_count;
        shading_rays.data->region_id[sample_count] = pixel_id;
        sample_count++;
      }
    }
    context.sample_generator->setupPacket(context, shading_rays);

    context.sample_generator->nextSeeds(context, r1, shading_rays);
    context.sample_generator->nextSeeds(context, r2, shading_rays);

    Real inv_sqrt = 1.f/static_cast<Real>(sqrtRays);
    sample_count = 0;
    for(int xs = 0; xs < sqrtRays; xs++) {
      for(int ys = 0; ys < sqrtRays; ys++) {
        Real x_sample = (xs + r1.get(sample_count)) * inv_sqrt;
        Real y_sample = (ys + r2.get(sample_count)) * inv_sqrt;
        px = (fx+x_sample)*ci.xscale+ci.xoffset;
        py = (fy+y_sample)*ci.yscale+ci.yoffset;
        shading_rays.setPixel(sample_count, 0, px, py);
        sample_count++;
      }
    }
    shading_rays.resetHits();
    shading_rays.setAllFlags(flags);
    context.renderer->traceEyeRays(context, shading_rays);

    // Now the results of our shading_rays have come back, determine
    // for each unique primitive the average result and then determine
    // the final average as a weighted sum of those primitives
    Color primColor[RayPacket::MaxSize];
    for (int i=0; i < RayPacket::MaxSize; ++i)
      primColor[i] = Color::black();  // black is the additive fixpoint.
    int shade_counts[RayPacket::MaxSize] = {0};
    for (int i = shading_rays.begin(); i < shading_rays.end(); i++) {
      int which_prim = -1;
      const Primitive* prim = shading_rays.getHitPrimitive(i);
      for (int j = 0; j < uniquePrims; j++) {
        if (prims[j] == prim) {
          which_prim = j;
          break;
        }
      }
      if (which_prim != -1) {
        shade_counts[which_prim]++;
        primColor[which_prim] += shading_rays.getColor(i);
      }
    }
    // Go through all the unique prims and determine the average
    // shading results
    Color final_color = Color::black();
#if 1
    for (int i = 0; i < uniquePrims; i++) {
      if (shade_counts[i]) {
        Color avgPrimColor = primColor[i] / static_cast<Real>(shade_counts[i]);
        final_color += avgPrimColor * (counts[i] * inv_samples);
      }
    }
#else
#if 0
    // Efficiency metric
    fragment.setColor(frag_index, Color::white() * static_cast<Real>(realNumRays)/static_cast<Real>(num_samples));
#else
    // Just the result from the shading rays
    for (int i = shading_rays.begin(); i < shading_rays.end(); i++) {
      final_color += shading_rays.getColor(i);
    }
    final_color *= (inv_sqrt * inv_sqrt);
#endif
#endif
    fragment.setColor(frag_index, final_color);
  }
  mutable_context.sample_generator = original_samplegen;
}
