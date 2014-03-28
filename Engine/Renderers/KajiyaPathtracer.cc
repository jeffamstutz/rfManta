
#include <Engine/Renderers/KajiyaPathtracer.h>
#include <Engine/Shadows/NoDirect.h>
#include <Interface/Background.h>
#include <Interface/Camera.h>
#include <Interface/Context.h>
#include <Interface/Material.h>
#include <Interface/Object.h>
#include <Interface/RayPacket.h>
#include <Interface/SampleGenerator.h>
#include <Interface/Scene.h>
#include <Interface/ShadowAlgorithm.h>
#include <Core/Math/MiscMath.h>
#include <Core/Util/Assert.h>
#include <Core/Util/NotFinished.h>
#include <Core/Color/ColorSpace_fancy.h>
#include <iostream>
using namespace std;

using namespace Manta;

Renderer* KajiyaPathtracer::create(const vector<string>& args)
{
  return new KajiyaPathtracer(args);
}

KajiyaPathtracer::KajiyaPathtracer(const vector<string>& /*args*/)
{
  ambient_only_sa = new NoDirect();
  override_maxdepth = -1;
  // No presorting, since none are implemented yet.
  presort_mode.push_back(PreSortNone);

  // Full sort on materials
  matlsort_mode.push_back(MatlSortBackgroundSweep);
  matlsort_mode.push_back(MatlSortSweep);
  
  //Note (thiago): For now it seems to be faster not to do the full
  //sort. At least for the scenes I've tested.
  //matlsort_mode.push_back(MatlSortFull);

  // Do RR only after the first bounce
  russian_roulette_mode.push_back(RussianRouletteNone);
  russian_roulette_mode.push_back(RussianRouletteFull);

  do_ambient_on_maxdepth = true;
}

KajiyaPathtracer::~KajiyaPathtracer()
{
}

void KajiyaPathtracer::setupBegin(const SetupContext&, int)
{
}

void KajiyaPathtracer::setupDisplayChannel(SetupContext&)
{
}

void KajiyaPathtracer::setupFrame(const RenderContext&)
{
}

static inline bool sort_needs_swap(const RayPacketData* data, int i, int j)
{
  return (long)data->hitMatl[i] > (long)data->hitMatl[j];
}

template<class T>
static inline void swapInArray(T* array, int i, int j)
{
  T tmp = array[i];
  array[i] = array[j];
  array[j] = tmp;
}

/*

  Basic algorithm:

  while there are active rays and depth < maxdepth
    1. Pre-sort rays in the packet according to some criteria
       Options for each depth
       a. No sort
       b. Sort using a space filling curve on origin (not implemented yet)
       c. Sort using a space filling curve on direction (not implemented yet)
       d. Sort using a space filling curve in 5D
    2. intersect
    3. Sort on materials, separating background rays
       Options for each depth
       a. Only partition out background rays
       b. Group identical materials based on a sweep
       c. Full sort on material
    4. compute new rays and reflectance
    5. compute direct light
    6. Russian roulete to kill rays
       Options for each depth
       a. No russian roulette
       b. Full russian roulette

  After the loop:
  (optional) for remaining rays, compute ambient light
  compute background for all rays that struck the background

  Presort (step 1), material sort (step 3), and russian roulette (step 4) can all be controlled
  per depth with the presort, matlsort, and russian_roulette arrays, respectively.  Each array
  contains an enum with the option for that depth.  If the depth is larger than the array, the
  last element is used.

*/

void KajiyaPathtracer::traceEyeRays(const RenderContext& context, RayPacket& rays)
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  context.camera->makeRays(context, rays);
  rays.initializeImportance();

  Packet<Color> result;
  result.fill(rays.begin(), rays.end(), Color::black());
  Packet<Color> reflectance;

  Packet<int> permute;
  if(!rays.getFlag(RayPacket::ConstantPixel)){
    for(int i=rays.begin();i<rays.end();i++)
      permute.data[i] = i;
  }

  int originalBegin = rays.begin();
  int originalEnd = rays.end();

  RayPacketData* data = rays.data;
  int depth = 0;
  int maxdepth = override_maxdepth >= 0? override_maxdepth:context.scene->getRenderParameters().maxDepth;
  for(;;){

    // 1. Pre-sort rays in the packet according to some criteria
    PreSortMode psm = depth >= static_cast<int>(presort_mode.size())?presort_mode[presort_mode.size()-1]:presort_mode[depth];
    switch(psm){
    case PreSortOrigin:
      presort_origin(context, rays, result, permute);
      break;
    case PreSortDirection:
      presort_direction(context, rays, result, permute);
      break;
    case PreSort5D:
      presort_5d(context, rays, result, permute);
      break;
    case PreSortNone:
    default:
      // Do nothing
      break;
    }

    // 2. intersect
    rays.resetHits();
    context.scene->getObject()->intersect(context, rays);
    rays.computeHitPositions();
    rays.computeFFGeometricNormals<false>(context); // Must do this before writing over the ray direction

    // 3. Sort on materials, separating background rays
    MatlSortMode msm = depth >= static_cast<int>(matlsort_mode.size())?matlsort_mode[matlsort_mode.size()-1]:matlsort_mode[depth];
    switch(msm){
    case MatlSortSweep:
      matlsort_sweep(context, rays, result, reflectance, permute);
      break;
    case MatlSortFull:
      matlsort_full(context, rays, result, permute);
      break;
    case MatlSortBackgroundSweep:
    default:
      matlsort_bgsweep(context, rays, result, permute);
      break;
    }
    if(rays.begin() == rays.end())
      break;

    // 4. compute new rays and reflectance
    // For MatlSortSweep, this was already done simultaneous with sorting
    if(msm != MatlSortSweep){
      for(int i = rays.begin();i<rays.end();){
        const Material* hit_matl = rays.getHitMaterial(i);
        int end = i+1;
        while(end < rays.end() &&  rays.getHitMaterial(end) == hit_matl)
          end++;
        RayPacket subPacket(rays, i, end);
        hit_matl->sampleBSDF(context, subPacket, reflectance);
        i=end;
      }
    }

    // Propagate reflectance
    for(int i=rays.begin();i<rays.end();i++){
      for(int j=0;j<3;j++)
        data->importance[j][i] *= reflectance.colordata[j][i];
    }

    rays.flags &= ~(RayPacket::HaveCornerRays | RayPacket::HaveInverseDirections | RayPacket::HaveSigns);
    // For now, assume that the rays coming out of sampleBSDF are unit length.  With
    // transformations, this may not be true, so consider gathering the unit length
    // flags from sampleBSDF above
    rays.flags |= RayPacket::NormalizedDirections;

    // 5. compute direct light
    ShadowAlgorithm::StateBuffer shadowState;
    do {
      RayPacketData shadowData;
      RayPacket shadowRays(shadowData, RayPacket::UnknownShape, 0, 0, depth, false);

      // Call the shadowalgorithm(sa) to generate shadow rays.  We may not be
      // able to compute all of them, so we pass along a buffer for the sa
      // object to store it's state.
      context.shadowAlgorithm->computeShadows(context, shadowState, context.scene->getLights(),
                                              rays, shadowRays);

      // We need normalized directions for proper dot product computation.
      shadowRays.normalizeDirections();

      for(int i=shadowRays.begin(); i < shadowRays.end(); i++){
        if(!shadowRays.wasHit(i)){
          // Not in shadow, so compute the direct lighting contributions.
          const Vector normal = rays.getFFGeometricNormal(i);
          const Vector light_dir = shadowRays.getDirection(i);
          // TODO(boulos): Replace this with BSDFEval
          const double weight=Dot(normal, light_dir);
          const Color light_color = shadowRays.getColor(i);
          result.set(i, result.get(i) + rays.getImportance(i) * light_color * weight);
        }
      }
    } while(!shadowState.done());

    // (5.b) Mark rays as having had direct light computed already.
    for(int i=rays.begin();i<rays.end();i++){
      rays.data->ignoreEmittedLight[i] = 1;
    }

    // 6. Russian roulete to kill rays
    RussianRouletteMode rrm = depth >= static_cast<int>(russian_roulette_mode.size())?russian_roulette_mode[russian_roulette_mode.size()-1]:russian_roulette_mode[depth];
    switch(rrm){
    case RussianRouletteFull:
      if(depth != maxdepth-1)
        russian_roulette_full(context, rays, result, reflectance, permute);
      break;
    case RussianRouletteNone:
    default:
      // Do nothing
      break;
    }
    if(rays.begin() == rays.end())
      break;

    depth++;
    if(depth >= maxdepth)
      break;

    rays.flags &= RayPacket::ConstantEye |  RayPacket::ConstantPixel | RayPacket::ConstantSampleRegion | RayPacket::NormalizedDirections;
    rays.shape = RayPacket::UnknownShape;
  }

  // Add ambient contribution for any remaining rays
  if(do_ambient_on_maxdepth){
    // Should there be an option here for doing a sweep/sort on the remaining rays???
    ShadowAlgorithm* save_sa = context.shadowAlgorithm;
    // Sneaky ugly cast-away-const.  Consider making RenderContext not const for this reason
    const_cast<RenderContext&>(context).shadowAlgorithm = ambient_only_sa;
    for(int i = rays.begin();i<rays.end();){
      const Material* hit_matl = rays.getHitMaterial(i);
      int end = i+1;
      while(end < rays.end() &&  rays.getHitMaterial(end) == hit_matl)
        end++;
      RayPacket subPacket(rays, i, end);
      hit_matl->shade(context, subPacket);
      i=end;
    }
    const_cast<RenderContext&>(context).shadowAlgorithm = save_sa;

    for(int i = rays.begin(); i < rays.end(); i++){
      for(int j=0;j<3;j++)
        result.colordata[j][i] += data->color[j][i] * data->importance[j][i];
    }
  }

  // Add contribution from background rays, which will be at the beginning.
  if(rays.begin() != originalBegin){
    RayPacket subPacket(rays, originalBegin, rays.begin());
    context.scene->getBackground()->shade(context, subPacket);
    for(int i=originalBegin;i<rays.begin();i++){
      for(int j=0;j<3;j++)
        result.colordata[j][i] += data->color[j][i] * data->importance[j][i];
    }
  }

  rays.resize(originalBegin, originalEnd);
  rays.resetHits();
  rays.flags &= RayPacket::ConstantEye |  RayPacket::ConstantPixel | RayPacket::ConstantSampleRegion;
  rays.flags |= RayPacket::NormalizedDirections;

  for(int i=rays.begin();i<rays.end();i++){
    rays.setColor(permute.get(i), result.get(i));
  }

}

void KajiyaPathtracer::matlsort_full(const RenderContext& context, RayPacket& rays,
                                      Packet<Color>& result,
                                      Packet<int>& permute)
{
  // Sort the rays based on the material that they hit
  // Simple bubble sort for now - performance will come later
  // This may not be worthwhile if we sort by directions, but
  // we would still need to sort out the background rays (one pass)
  // Another option is to use a sort that picks one unique value and then
  // searches for all pointers of this type.   It could probably be SSEd.
  RayPacketData* data = rays.data;
  for(int i=rays.begin();i<rays.end()-1;i++){
    for(int j=i+1;j<rays.end();j++){
      if(sort_needs_swap(data, i, j)){
        // Move hitPrim, hitMatl, hitTex, emitted light flag, origin, direction, minT, time
        swapInArray(data->hitPrim, i, j);
        swapInArray(data->hitMatl, i, j);
        swapInArray(data->hitTex, i, j);
        swapInArray(data->ignoreEmittedLight, i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->origin[k], i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->direction[k], i, j);
        swapInArray(data->minT, i, j);
        swapInArray(data->time, i, j);

        // Conditionally: normal
        for(int k=0;k<3;k++)
          swapInArray(data->normal[k], i, j);

        // Conditionally: ffnormal,
        for(int k=0;k<3;k++)
          swapInArray(data->ffnormal[k], i, j);

        // Conditionally: geometricNormal
        for(int k=0;k<3;k++)
          swapInArray(data->geometricNormal[k], i, j);

        // Conditionally: ffgeometricNormal,
        for(int k=0;k<3;k++)
          swapInArray(data->ffgeometricNormal[k], i, j);

        // Conditionally: hitPosition,
        for(int k=0;k<3;k++)
          swapInArray(data->hitPosition[k], i, j);

        // Conditionally: texCoords, dPdu, dpDv
        // Perhaps these do not need to be moved???
        for(int k=0;k<3;k++)
          swapInArray(data->texCoords[k], i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->dPdu[k], i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->dPdv[k], i, j);

        // Move importance
        for(int k=0;k<3;k++)
          swapInArray(data->importance[k], i, j);

        // Move sample_id, region_id
        swapInArray(data->sample_id, i, j);
        swapInArray(data->region_id, i, j);

        //Move scratchpad
        for (int k=0; k < RayPacketData::MaxScratchpad4; ++k)
          swapInArray(data->scratchpad4[k], i, j);
        for (int k=0; k < RayPacketData::MaxScratchpad8; ++k)
          swapInArray(data->scratchpad8[k], i, j);

        // Move result
        for(int k=0;k<3;k++)
          swapInArray(result.colordata[k], i, j);

        // Move permute
        swapInArray(permute.data, i, j);
      }
    }
  }
  // Pull out background rays - they will be shaded all at once below
  int newBegin = rays.begin();
  while(newBegin < rays.end() && !rays.wasHit(newBegin))
    newBegin++;
  rays.resize(newBegin, rays.end());
}

void KajiyaPathtracer::russian_roulette_full(const RenderContext& context, RayPacket& rays,
                                             Packet<Color>& result,
                                             Packet<Color>& reflectance,
                                             Packet<int>& permute)
{
  Packet<Real> rr;
  context.sample_generator->nextSeeds(context, rr, rays);
  RayPacketData* data = rays.data;

  int newEnd = rays.end()-1;
  while(newEnd >= rays.begin() && rr.get(newEnd) >= reflectance.get(newEnd).maxComponent())
    newEnd--;

  for(int i=rays.begin();i<=newEnd;i++){
    double max = reflectance.get(i).maxComponent();
    if(rr.get(i) > max){
      // Kill it by swapping with the last ray
      // We may not need to move origin/direction back....
      // Move origin, direction
      // Look at one-way move optimization
      for(int j=0;j<3;j++)
        swapInArray(data->origin[j], i, newEnd);
      for(int j=0;j<3;j++)
        swapInArray(data->direction[j], i, newEnd);

      // Move ffnormal
      for(int j=0;j<3;j++)
        swapInArray(data->ffnormal[j], i, newEnd);

      // Move ffgeometricNormal
      for(int j=0;j<3;j++)
        swapInArray(data->ffgeometricNormal[j], i, newEnd);

      // Move hitPosition
      for(int j=0;j<3;j++)
        swapInArray(data->hitPosition[j], i, newEnd);

      // Move time
      // Look at one-way move optimization
      swapInArray(data->time, i, newEnd);

      // Move importance
      // Look at one-way move optimization
      for(int j=0;j<3;j++)
        swapInArray(data->importance[j], i, newEnd);

      // Move sample_id, region_id
      // Look at one-way move optimization
      swapInArray(data->sample_id, i, newEnd);
      swapInArray(data->region_id, i, newEnd);

      //Move scratchpad
      for (int k=0; k < RayPacketData::MaxScratchpad4; ++k)
        swapInArray(data->scratchpad4[k], i, newEnd);
      for (int k=0; k < RayPacketData::MaxScratchpad8; ++k)
        swapInArray(data->scratchpad8[k], i, newEnd);

      // Move result
      for(int j=0;j<3;j++)
        swapInArray(result.colordata[j], i, newEnd);

      // Move reflectance
      // Look at one-way move optimization
      for(int j=0;j<3;j++)
        swapInArray(reflectance.colordata[j], i, newEnd);

      // Move rr
      // Look at one-way move optimization
      swapInArray(rr.data, i, newEnd);

      // Move permute
      swapInArray(permute.data, i, newEnd);

      i--;
      newEnd--;
      while(newEnd > i && rr.get(newEnd) >= reflectance.get(newEnd).maxComponent())
        newEnd--;
    }
  }
  for(int i=newEnd+1;i<rays.end();i++){
    float max = reflectance.get(i).maxComponent();
    Real scale = Real(1.)/max;
    for(int j=0;j<3;j++)
      reflectance.colordata[j][i] *= scale;
    for(int j=0;j<3;j++)
      rays.data->importance[j][i] *= scale;
  }

  rays.resize(rays.begin(), newEnd+1);
}

void KajiyaPathtracer::presort_origin(const RenderContext&, RayPacket&, Packet<Color>&, Packet<int>&)
{
  NOT_FINISHED("KajiyaPathtracer::traceEyeRays");
}

void KajiyaPathtracer::matlsort_sweep(const RenderContext& context, RayPacket& rays,
                                      Packet<Color>& result,
                                      Packet<Color>& reflectance,
                                      Packet<int>& permute)
{
  matlsort_bgsweep(context, rays, result, permute);
  // Sort the rays into two groups - those that hit an object
  // and those that hit the background.  The background rays should
  // be first, so those are processed first by the bgsweep.
  // Technically, this is O(N^2), but if there are relatively few unique
  // materials in an average packet, this should perform pretty well.
  // We try to minimize movement of rays wherever possible.
  RayPacketData* data = rays.data;
  for(int i=rays.begin(); i < rays.end();){
    const Material* matl = data->hitMatl[i];
    int begin = i;
    i++;

    // Skip to find the next hole
    while(i < rays.end() && data->hitMatl[i] == matl)
      i++;

    // Find any other rays that also hit that material.  Start from the end so that
    // We can fill in gaps and minimize the total number of moves in many cases
    for(int j=rays.end()-1;j>=i+1;j--){
      // Skip over all of the rays that hit the same material
      if(data->hitMatl[j] == matl){
        // Move hitPrim, hitMatl, hitTex, origin, direction, minT, time
        swapInArray(data->hitPrim, i, j);
        swapInArray(data->hitMatl, i, j);
        swapInArray(data->hitTex, i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->origin[k], i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->direction[k], i, j);
        swapInArray(data->minT, i, j);
        swapInArray(data->time, i, j);

        // Conditionally: normal
        for(int k=0;k<3;k++)
          swapInArray(data->normal[k], i, j);

        // Conditionally: ffnormal,
        for(int k=0;k<3;k++)
          swapInArray(data->ffnormal[k], i, j);

        // Conditionally: geometricNormal
        for(int k=0;k<3;k++)
          swapInArray(data->geometricNormal[k], i, j);

        // Conditionally: ffgeometricNormal,
        for(int k=0;k<3;k++)
          swapInArray(data->ffgeometricNormal[k], i, j);

        // Conditionally: hitPosition,
        for(int k=0;k<3;k++)
          swapInArray(data->hitPosition[k], i, j);

        // Conditionally: texCoords, dPdu, dpDv
        // Perhaps these do not need to be moved???
        for(int k=0;k<3;k++)
          swapInArray(data->texCoords[k], i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->dPdu[k], i, j);
        for(int k=0;k<3;k++)
          swapInArray(data->dPdv[k], i, j);

        // Move importance
        for(int k=0;k<3;k++)
          swapInArray(data->importance[k], i, j);

        // Move sample_id, region_id
        swapInArray(data->sample_id, i, j);
        swapInArray(data->region_id, i, j);

        //Move scratchpad
        for (int k=0; k < RayPacketData::MaxScratchpad4; ++k)
          swapInArray(data->scratchpad4[k], i, j);
        for (int k=0; k < RayPacketData::MaxScratchpad8; ++k)
          swapInArray(data->scratchpad8[k], i, j);

        // Move result
        for(int k=0;k<3;k++)
          swapInArray(result.colordata[k], i, j);

        // Move permute
        swapInArray(permute.data, i, j);
        i++;

        // Find another hole
        while(i <= j && data->hitMatl[i] == matl)
          i++;
      }
    }
    RayPacket subPacket(rays, begin, i);
    matl->sampleBSDF(context, subPacket, reflectance);
  }
}

void KajiyaPathtracer::presort_direction(const RenderContext&, RayPacket&, Packet<Color>&, Packet<int>&)
{
  NOT_FINISHED("KajiyaPathtracer::presort_direction");
}

void KajiyaPathtracer::presort_5d(const RenderContext&, RayPacket&, Packet<Color>&, Packet<int>&)
{
  NOT_FINISHED("KajiyaPathtracer::presort_5d");
}

void KajiyaPathtracer::matlsort_bgsweep(const RenderContext&, RayPacket& rays,
                                        Packet<Color>& result,
                                        Packet<int>& permute)
{
  // Sort the rays into two groups - those that hit an object
  // and those that hit the background.  The background rays should
  // be first, so we start and the end and push rays to the beginning
  // that did not hit anything
  RayPacketData* data = rays.data;
  int newBegin = rays.begin();
  while(newBegin < rays.end() && !data->hitMatl[newBegin])
    newBegin++;
  for(int i=rays.end()-1;i>=newBegin;i--){
    if(!data->hitMatl[i]){
      // Move hitPrim, hitMatl, hitTex, origin, direction, minT, time
      swapInArray(data->hitPrim, i, newBegin);
      swapInArray(data->hitMatl, i, newBegin);
      swapInArray(data->hitTex, i, newBegin);
      for(int k=0;k<3;k++)
        swapInArray(data->origin[k], i, newBegin);
      for(int k=0;k<3;k++)
        swapInArray(data->direction[k], i, newBegin);
      swapInArray(data->minT, i, newBegin);
      swapInArray(data->time, i, newBegin);

      // Conditionally: normal
      for(int k=0;k<3;k++)
        swapInArray(data->normal[k], i, newBegin);

      // Conditionally: ffnormal,
      for(int k=0;k<3;k++)
        swapInArray(data->ffnormal[k], i, newBegin);

      // Conditionally: geometricNormal
      for(int k=0;k<3;k++)
        swapInArray(data->geometricNormal[k], i, newBegin);

      // Conditionally: ffgeometricNormal,
      for(int k=0;k<3;k++)
        swapInArray(data->ffgeometricNormal[k], i, newBegin);

      // Conditionally: hitPosition,
      for(int k=0;k<3;k++)
        swapInArray(data->hitPosition[k], i, newBegin);

      // Conditionally: texCoords, dPdu, dpDv
      // Perhaps these do not need to be moved???
      for(int k=0;k<3;k++)
        swapInArray(data->texCoords[k], i, newBegin);
      for(int k=0;k<3;k++)
        swapInArray(data->dPdu[k], i, newBegin);
      for(int k=0;k<3;k++)
        swapInArray(data->dPdv[k], i, newBegin);

      // Move importance
      for(int k=0;k<3;k++)
        swapInArray(data->importance[k], i, newBegin);

      // Move sample_id, region_id
      swapInArray(data->sample_id, i, newBegin);
      swapInArray(data->region_id, i, newBegin);

      //Move scratchpad
      for (int k=0; k < RayPacketData::MaxScratchpad4; ++k)
        swapInArray(data->scratchpad4[k], i, newBegin);
      for (int k=0; k < RayPacketData::MaxScratchpad8; ++k)
        swapInArray(data->scratchpad8[k], i, newBegin);

      // Move result
      for(int k=0;k<3;k++)
        swapInArray(result.colordata[k], i, newBegin);

      // Move permute
      swapInArray(permute.data, i, newBegin);
      newBegin++;
      while(!data->hitMatl[newBegin] && newBegin < i)
        newBegin++;
      i++;
    }
  }
  rays.resize(newBegin, rays.end());
}


void KajiyaPathtracer::traceRays(const RenderContext& context, RayPacket& rays)
{
  int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;
  rays.resetHits();
  context.scene->getObject()->intersect(context, rays);

  // Go through the ray packet and shade them.  Group rays that hit the
  // same object and material to shade with a single shade call
  for(int i = rays.begin();i<rays.end();){
    if(rays.wasHit(i)){
      const Material* hit_matl = rays.getHitMaterial(i);
      int end = i+1;
      while(end < rays.end() && rays.wasHit(end) &&
            rays.getHitMaterial(end) == hit_matl)
        end++;
      if (debugFlag) {
        rays.computeHitPositions();
        for (int j = i; j < end; ++j) {
          cerr << "raytree: ray_index "<<j
               << " depth " << rays.getDepth()
               << " origin "<< rays.getOrigin(j)
               << " direction "<< rays.getDirection(j)
               << " hitpos " << rays.getHitPosition(j)
               << "\n";
        }
      }
      RayPacket subPacket(rays, i, end);
      hit_matl->shade(context, subPacket);
      i=end;
    } else {
      int end = i+1;
      while(end < rays.end() && !rays.wasHit(end))
        end++;
      if (debugFlag) {
        for (int j = i; j < end; ++j) {
          cerr << "raytree: ray_index "<<j
               << " depth " << rays.getDepth()
               << " origin "<< rays.getOrigin(j)
               << " direction "<< rays.getDirection(j)
               << "\n";
        }
      }
      RayPacket subPacket(rays, i, end);
      context.scene->getBackground()->shade(context, subPacket);
      i=end;
    }
  }
}

void KajiyaPathtracer::traceRays(const RenderContext& context, RayPacket& rays, Real cutoff)
{
  for(int i = rays.begin(); i != rays.end();) {
    if(rays.getImportance(i).luminance() > cutoff) {
      int end = i + 1;
      while(end < rays.end() && rays.getImportance(end).luminance() > cutoff)
        end++;

      RayPacket subPacket(rays, i, end);
      traceRays(context, subPacket);
      i = end;
    } else {
      // need to set the color to black
      rays.setColor(i, Color::black());
      int end = i + 1;
      while(end < rays.end() && rays.getImportance(end).luminance() <= cutoff) {
        rays.setColor(end, Color::black());
        end++;
      }
      i = end;
    }
  }
}
