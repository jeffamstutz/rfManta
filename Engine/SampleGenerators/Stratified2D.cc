#include <Core/Util/Preprocessor.h>
#include <Engine/SampleGenerators/Stratified2D.h>
#include <Interface/Context.h>
#include <Interface/Scene.h>
#include <Interface/RenderParameters.h>
#include <Interface/RandomNumberGenerator.h>
#include <Interface/RayPacket.h>
#include <Core/Math/Expon.h>
#include <Core/Exceptions/InternalError.h>

#include <iostream>
using namespace Manta;

Stratified2D::Stratified2D(unsigned int spp) :
  spp(spp), thread_samples(0), max_depth(0), num_threads(0),
  current_region_id(0), current_raw_region_id(0), reached_depth(0),
  xres(0), yres(0) {
  sqrt_spp = static_cast<unsigned int>(Sqrt(static_cast<double>(spp)));
  if (sqrt_spp * sqrt_spp != spp) {
    throw InternalError("Stratified2D expects spp to be a perfect square");
  }
  inv_spp = 1.f/static_cast<float>(spp);
  inv_sqrt_spp = 1.f/static_cast<float>(sqrt_spp);

  // Determine the maximum number of active regions at
  // once. NOTE(boulos): Currently assuming that a Pixel = a Region
  if (spp > RayPacket::MaxSize) {
    if (spp % RayPacket::MaxSize == 0) {
      max_regions = 1;
    } else {
      max_regions = 2;
    }
  } else {
    if (RayPacket::MaxSize % spp == 0) {
      max_regions = RayPacket::MaxSize/spp;
    } else {
      // max_regions = Ceil(MaxSize/spp) + 1
      max_regions = (RayPacket::MaxSize/spp) + 2;
    }
  }
}

Stratified2D::~Stratified2D() {
}

void Stratified2D::setupBegin(const SetupContext&, int numChannels) {
}

void Stratified2D::allocateSamples(unsigned int new_threads, unsigned int new_depth) {
  // Already have the storage
  if (new_threads == num_threads &&
      new_depth == max_depth) return;

  if (thread_samples) {
    for (unsigned int i = 0; i < num_threads; i++) {
      for (unsigned int r = 0; r < max_regions; r++) {
        delete[] thread_samples[i][r];
      }
      delete[] thread_samples[i];
    }
    delete[] thread_samples;
    thread_samples = 0;
  }

  if (reached_depth) {
    for (unsigned int i = 0; i < num_threads; i++) {
      delete[] reached_depth[i];
    }
    delete[] reached_depth;
    reached_depth = 0;
  }

  // TODO(boulos): Handle the false sharing issues here...
  if (current_region_id) {
    delete[] current_region_id;
    current_region_id = 0;
  }
  if (current_raw_region_id) {
    delete[] current_raw_region_id;
    current_raw_region_id = 0;
  }

  num_threads = new_threads;
  max_depth = new_depth;
  thread_samples = new float**[num_threads];
  current_region_id = new unsigned int[num_threads];
  current_raw_region_id = new unsigned int[num_threads];
  reached_depth = new int*[num_threads];
  for (unsigned int i = 0; i < num_threads; i++) {
    thread_samples[i] = new float*[max_regions];
    reached_depth[i] = new int[max_regions];
    for (unsigned int r = 0; r < max_regions; r++) {
      thread_samples[i][r] = new float[spp * max_depth];
      reached_depth[i][r] = -1;
    }
    current_region_id[i] = 0;
    current_raw_region_id[i] = static_cast<unsigned int>(-1);
  }
}

void Stratified2D::buildSamplesAllDepths(RandomNumberGenerator* rng,
                                         unsigned int thread_id,
                                         unsigned int region_id) {
  for (unsigned int i = 0; i < max_depth; i++) {
    buildSamples(rng, thread_id, region_id, i);
  }
}

void Stratified2D::buildSamples(RandomNumberGenerator* rng,
                                unsigned int thread_id,
                                unsigned int region_id,
                                unsigned int depth) {
  int cur_depth = reached_depth[thread_id][region_id];
  if (cur_depth >= static_cast<int>(depth)) return;
  float* all_samples = thread_samples[thread_id][region_id];
  if (max_depth - depth > 1) {
    // NOTE(boulos): We only want to generate the even cases
    if (depth % 2 == 0) {
      float* x_samples = &(all_samples[spp * depth]);
      float* y_samples = &(all_samples[spp * (depth+1)]);
      build2DSamples(rng, x_samples, y_samples);
      reached_depth[thread_id][region_id] = depth + 1;
    }
  } else {
    float* sample_buf = &(all_samples[spp * depth]);
    build1DSamples(rng, sample_buf);
    reached_depth[thread_id][region_id] = depth;
  }
}


void Shuffle2DSamples(float* x_samples,
                      float* y_samples,
                      unsigned int num_samples,
                      RandomNumberGenerator* rng) {
  for (int i = static_cast<int>(num_samples) - 2; i >= 0; i--) {
    int target = static_cast<int>(rng->nextFloat() * i);
    float tmp_x = x_samples[i+1];
    float tmp_y = y_samples[i+1];
    x_samples[i+1] = x_samples[target];
    y_samples[i+1] = y_samples[target];

    x_samples[target] = tmp_x;
    y_samples[target] = tmp_y;
  }
}

void Stratified2D::build2DSamples(RandomNumberGenerator* rng,
                                  float* x_samples,
                                  float* y_samples) {
  unsigned int index = 0;
  float ii = 0.f;
  float jj = 0.f;
  for (unsigned int i = 0; i < sqrt_spp; i++) {
    jj = 0.f;
    for (unsigned int j = 0; j < sqrt_spp; j++) {
      float x = (ii + rng->nextFloat()) * inv_sqrt_spp;
      float y = (jj + rng->nextFloat()) * inv_sqrt_spp;
      jj += 1.f;
      x_samples[index + j] = x;
      y_samples[index + j] = y;
    }
    ii += 1.f;
    index += sqrt_spp;
  }
  Shuffle2DSamples(x_samples, y_samples, spp, rng);
}

void Shuffle1DSamples(float* samples,
                      unsigned int num_samples,
                      RandomNumberGenerator* rng) {
  for (int i = static_cast<int>(num_samples) - 2; i >= 0; i--) {
    int target = static_cast<int>(rng->nextFloat() * i);
    float tmp = samples[i+1];
    samples[i+1] = samples[target];
    samples[target] = tmp;
  }
}


void Stratified2D::build1DSamples(RandomNumberGenerator* rng, float* sample_buf) {
  for (unsigned int i = 0; i < spp; i++) {
    sample_buf[i] = (i + rng->nextFloat()) * inv_spp;
  }
  Shuffle1DSamples(sample_buf, spp, rng);
}

void Stratified2D::setupDisplayChannel(SetupContext& context) {
  bool stereo;
  int width, height;
  context.getResolution(stereo, width, height);
  if (stereo) throw InternalError("Stratified2D doesn't handle stereo yet");
  xres = width;
  yres = height;
}

void Stratified2D::setupFrame(const RenderContext& context) {
  if (context.proc == 0) {
    // NOTE(boulos): Sample depth is different than bounce
    // depth. Assume we need about 2 seeds per bounce. Above the max
    // depth, just get random
    allocateSamples(context.numProcs, 2 * context.scene->getRenderParameters().maxDepth);
  }
}

unsigned int Stratified2D::computeRegion(const RenderContext& context, RayPacket& rays, int i) {
  // Compute pixel index
  unsigned int raw_region_id = static_cast<unsigned int>(rays.data->region_id[i]);

  if (raw_region_id != current_raw_region_id[context.proc]) {
    // generate a new region_id
    //std::cerr << "Generating a new region id because rri != crri (" << raw_region_id << ", ";
    //std::cerr << current_raw_region_id[context.proc] << ")" << std::endl;
    current_region_id[context.proc]++;
    current_region_id[context.proc] %= max_regions;
    current_raw_region_id[context.proc] = raw_region_id;
    reached_depth[context.proc][current_region_id[context.proc]] = -1;
#if 0
    // NOTE(boulos): Old behavior which costs a lot if you never end
    // up using the samples...
    buildSamplesAllDepths(context.rng, context.proc, current_region_id[context.proc]);
#endif
  }

  return current_region_id[context.proc];
}

void Stratified2D::setupPacket(const RenderContext& context, RayPacket& rays) {
  bool constant_region = true;
  for (int i = rays.begin(); i < rays.end(); i++) {
    rays.data->sample_depth[i] = 0;
    rays.data->region_id[i] = computeRegion(context, rays, i);
    constant_region &= (rays.data->region_id[i] == rays.data->region_id[rays.begin()]);
  }
  if (constant_region)
    rays.setFlag(RayPacket::ConstantSampleRegion);
}

void Stratified2D::setupChildPacket(const RenderContext& context, RayPacket& parent, RayPacket& child) {
  for (int i = parent.begin(); i < parent.end(); i++) {
    child.data->sample_depth[i] = parent.data->sample_depth[i];
    child.data->sample_id[i] = parent.data->sample_id[i];
    child.data->region_id[i] = parent.data->region_id[i];
  }
  if (parent.getFlag(RayPacket::ConstantSampleRegion)) {
    child.setFlag(RayPacket::ConstantSampleRegion);
  }
}

void Stratified2D::setupChildRay(const RenderContext& context, RayPacket& parent, RayPacket& child, int i, int j) {
  child.data->sample_depth[j] = parent.data->sample_depth[i];
  child.data->sample_id[j] = parent.data->sample_id[i];
  child.data->region_id[j] = parent.data->region_id[i];
  if (parent.getFlag(RayPacket::ConstantSampleRegion)) {
    child.setFlag(RayPacket::ConstantSampleRegion);
  }
}

void Stratified2D::nextSeeds(const RenderContext& context, Packet<float>& results, RayPacket& rays) {
  for (int i = rays.begin(); i < rays.end(); i++) {
    if (rays.data->sample_depth[i] < max_depth) {
      buildSamples(context.rng, context.proc, rays.data->region_id[i], rays.data->sample_depth[i]);
      results.set(i, thread_samples[context.proc][rays.data->region_id[i]][spp * rays.data->sample_depth[i] + rays.data->sample_id[i]]);
    } else {
      results.set(i, context.rng->nextFloat());
    }
    rays.data->sample_depth[i]++;
  }
}

void Stratified2D::nextSeeds(const RenderContext& context, Packet<double>& results, RayPacket& rays) {
  for (int i = rays.begin(); i < rays.end(); i++) {
    if (rays.data->sample_depth[i] < max_depth) {
      buildSamples(context.rng, context.proc, rays.data->region_id[i], rays.data->sample_depth[i]);
      results.set(i, static_cast<double>(thread_samples[context.proc][rays.data->region_id[i]][spp * rays.data->sample_depth[i] + rays.data->sample_id[i]]));
    } else {
      results.set(i, context.rng->nextDouble());
    }
    rays.data->sample_depth[i]++;
  }
}
