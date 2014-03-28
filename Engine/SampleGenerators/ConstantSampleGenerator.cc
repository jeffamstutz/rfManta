#include <Engine/SampleGenerators/ConstantSampleGenerator.h>
#include <Interface/RayPacket.h>

using namespace Manta;

ConstantSampleGenerator::ConstantSampleGenerator(float value) : sample_value(value) {
}

ConstantSampleGenerator::~ConstantSampleGenerator() {
}

void ConstantSampleGenerator::setupBegin(const SetupContext&, int numChannels) {
}

void ConstantSampleGenerator::setupDisplayChannel(SetupContext& context) {
}

void ConstantSampleGenerator::setupFrame(const RenderContext& context) {
}

void ConstantSampleGenerator::setupPacket(const RenderContext& context, RayPacket& rays) {
}

void ConstantSampleGenerator::setupChildPacket(const RenderContext& context, RayPacket& parent, RayPacket& child) {
}

void ConstantSampleGenerator::setupChildRay(const RenderContext& context, RayPacket& parent, RayPacket& child, int i, int j) {
}

void ConstantSampleGenerator::nextSeeds(const RenderContext& context, Packet<float>& results, RayPacket& rays) {
#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = (rays.rayEnd) & (~3);
  if (b >= e) {
    for (int i = rays.begin(); i < rays.end(); i++) {
      results.set(i, sample_value);
    }
  } else {
    int i = rays.rayBegin;
    for (; i < b; i++) {
       results.set(i, sample_value);
    }
    __m128 sse_val = _mm_set1_ps(sample_value);
    for (; i < e; i += 4) {
      _mm_store_ps(&results.data[i], sse_val);
    }
    for (; i < rays.rayEnd; i++) {
      results.set(i, sample_value);
    }
  }
#else
  for (int i = rays.begin(); i < rays.end(); i++) {
    results.set(i, sample_value);
  }
#endif // MANTA_SSE
}

void ConstantSampleGenerator::nextSeeds(const RenderContext& context, Packet<double>& results, RayPacket& rays) {
  for (int i = rays.begin(); i < rays.end(); i++) {
    results.set(i, sample_value);
  }
}
