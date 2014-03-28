#include <Engine/SampleGenerators/UniformRandomGenerator.h>
#include <Interface/Context.h>
#include <Interface/RandomNumberGenerator.h>
#include <Interface/RayPacket.h>



using namespace Manta;

UniformRandomGenerator::UniformRandomGenerator() {
}

UniformRandomGenerator::~UniformRandomGenerator() {
}

void UniformRandomGenerator::setupBegin(const SetupContext&, int numChannels) {
}

void UniformRandomGenerator::setupDisplayChannel(SetupContext& context) {
}

void UniformRandomGenerator::setupFrame(const RenderContext& context) {
}

void UniformRandomGenerator::setupPacket(const RenderContext& context, RayPacket& rays) {
}

void UniformRandomGenerator::setupChildPacket(const RenderContext& context, RayPacket& parent, RayPacket& child) {
}

void UniformRandomGenerator::setupChildRay(const RenderContext& context, RayPacket& parent, RayPacket& child, int i, int j) {
}

void UniformRandomGenerator::nextSeeds(const RenderContext& context, Packet<float>& results, RayPacket& rays) {
  context.rng->nextPacket(results, rays);
}

void UniformRandomGenerator::nextSeeds(const RenderContext& context, Packet<double>& results, RayPacket& rays) {
  context.rng->nextPacket(results, rays);
}
