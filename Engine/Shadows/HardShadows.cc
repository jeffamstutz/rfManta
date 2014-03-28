
#include <Engine/Shadows/HardShadows.h>
#include <Interface/Context.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Object.h>
#include <Interface/RayPacket.h>
#include <Interface/Scene.h>
#include <MantaSSE.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Interface/Material.h>
#include <iostream>
// TODO
// 0 copy in light stuff
// eliminate cleanup loops???
// 2-sided lighting

using namespace Manta;
using std::cerr;

ShadowAlgorithm* HardShadows::create(const vector<string>& args)
{
  return new HardShadows(args);
}

HardShadows::HardShadows(const vector<string>& args)
  : attenuateShadows(false)
{
  int argc = static_cast<int>(args.size());
  for(int i = 0; i<argc;i++){
    string arg = args[i];
    if(arg == "-attenuate" || arg == "-attenuateShadows"){
      attenuateShadows = true;
    }
    else {
      throw IllegalArgument("HardShadows", i, args);
    }
  }
}

HardShadows::~HardShadows()
{
}

void HardShadows::computeShadows(const RenderContext& context,
                                 StateBuffer& stateBuffer,
                                 const LightSet* lights,
                                 RayPacket& sourceRays,        // Input rays.
                                 RayPacket& shadowRays)        // Output shadow rays, already intersected.
{
  int debugFlag = sourceRays.getAllFlags() & RayPacket::DebugPacket;
  if (debugFlag) {
    cerr << "HardShadows::computeShadows called\n";
    //    cerr << getStackTrace();
  }

  int nlights = lights->numLights();

  // Compute the hit positions.
  sourceRays.computeHitPositions();
  sourceRays.computeFFGeometricNormals<true>( context );

  int j;
  if(stateBuffer.state == StateBuffer::Finished){
    return; // Shouldn't happen, but just in case...
  } else if(stateBuffer.state == StateBuffer::Start){
    if(nlights == 0){
      stateBuffer.state = StateBuffer::Finished;
      return;
    }
    j = 0;
  } else {
    // Continuing
    j = stateBuffer.i1;
  }

  if (!attenuateShadows)
    shadowRays.setFlag(RayPacket::AnyHit);

  // Compute the contribution for this light.
  int first = -1;
  int last = -1;
  do {
    lights->getLight(j)->computeLight(shadowRays, context, sourceRays);

#ifdef MANTA_SSE
    int b = (sourceRays.rayBegin + 3) & (~3);
    int e = sourceRays.rayEnd & (~3);
    // NOTE(boulos): b can be > e for small packets
    if(b >= e){
      for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
        Vector dir = shadowRays.getDirection(i);
        if(Dot(dir, sourceRays.getFFGeometricNormal(i)) > 0) {
          shadowRays.setOrigin(i, sourceRays.getHitPosition(i));
          shadowRays.setTime(i, sourceRays.getTime(i));
          // This is a version of resetHit that only sets the material
          // to NULL and doesn't require us to also modify the hit
          // distance which was set for us by the call to
          // computeLight.
          shadowRays.setHitMaterial(i, NULL);
          last = i;
          if (first < 0)
            first = i;
        }
        else if (first >= 0) {
          //if we've already found a valid ray, use that to copy valid
          //data into the invalid ray.
          shadowRays.setOrigin(i, shadowRays.getOrigin(first));
          shadowRays.setDirection(i, shadowRays.getDirection(first));
          shadowRays.setTime(i, shadowRays.getTime(first));

          shadowRays.maskRay(i); //this sets minT -MAXT and hitMatl to -1
          //We set minT to 0 instead of -MAXT just in case an algorithm
          //uses the ray hit position to compute things. In which case
          //having the hit position at the origin would probably break
          //things less than having it be infinitely far away in the
          //negative direction.
          shadowRays.overrideMinT(i, 0);//TOOD: verify 0 is ok (should we use -eps?).
        }
        else {
          shadowRays.maskRay(i);
        }
      }
    } else {
      int i = sourceRays.rayBegin;
      for(;i<b;i++){
        Vector dir = shadowRays.getDirection(i);
        if(Dot(dir, sourceRays.getFFGeometricNormal(i)) > 0) {
          shadowRays.setOrigin(i, sourceRays.getHitPosition(i));
          shadowRays.setTime(i, sourceRays.getTime(i));
          // See comment above.
          shadowRays.setHitMaterial(i, NULL);
          last = i;
          if (first < 0)
            first = i;
        }
        else if (first >= 0) {
          //if we've already found a valid ray, use that to copy valid
          //data into the invalid ray.
          shadowRays.setOrigin(i, shadowRays.getOrigin(first));
          shadowRays.setDirection(i, shadowRays.getDirection(first));
          shadowRays.setTime(i, shadowRays.getTime(first));

          shadowRays.maskRay(i); //this sets minT -MAXT and hitMatl to -1
          //We set minT to 0 instead of -MAXT just in case an algorithm
          //uses the ray hit position to compute things. In which case
          //having the hit position at the origin would probably break
          //things less than having it be infinitely far away in the
          //negative direction.
          shadowRays.overrideMinT(i, 0);//TOOD: verify 0 is ok (should we use -eps?).
        } else {
          shadowRays.maskRay(i);
        }
      }

      RayPacketData* sourceData = sourceRays.data;
      RayPacketData* shadowData = shadowRays.data;
      sse_t validOx=set4(0), validOy=set4(0), validOz=set4(0);
      sse_t validDx=set4(0), validDy=set4(0), validDz=set4(0);
      sse_t validTimes=set4(0);

      if (first >= 0) {
        validOx = set4(shadowData->origin[0][first]);
        validOy = set4(shadowData->origin[1][first]);
        validOz = set4(shadowData->origin[2][first]);

        validDx = set4(shadowData->direction[0][first]);
        validDy = set4(shadowData->direction[0][first]);
        validDz = set4(shadowData->direction[0][first]);

        validTimes = set4(shadowData->time[first]);
      }

      int firstSSE = first;
      for(;i<e;i+=4){
        __m128 normalx = _mm_load_ps(&sourceData->ffgeometricNormal[0][i]);
        __m128 normaly = _mm_load_ps(&sourceData->ffgeometricNormal[1][i]);
        __m128 normalz = _mm_load_ps(&sourceData->ffgeometricNormal[2][i]);
        __m128 dx = _mm_load_ps(&shadowData->direction[0][i]);
        __m128 dy = _mm_load_ps(&shadowData->direction[1][i]);
        __m128 dz = _mm_load_ps(&shadowData->direction[2][i]);
        __m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, normalx), _mm_mul_ps(dy, normaly)), _mm_mul_ps(dz, normalz));

        __m128 mask = _mm_cmple_ps(dot, _mm_setzero_ps());
#ifdef __x86_64
        _mm_store_ps((float*)&shadowData->hitMatl[i], _mm_unpacklo_ps(mask, mask));
        _mm_store_ps((float*)&shadowData->hitMatl[i+2], _mm_unpackhi_ps(mask, mask));
#else
        _mm_store_ps((float*)&shadowData->hitMatl[i], mask);
#endif

        _mm_store_ps((float*)&shadowData->minT[i],
                     _mm_or_ps(_mm_andnot_ps(mask,
                                             _mm_load_ps((float*)&shadowData->minT[i])),
                               _mm_and_ps(mask, _mm_setzero_ps())));

        const int maskResults = getmask4(mask);
        if(maskResults != 0xf){
          // Some rays are valid (0 is valid, 1 are invalid => 0xf all invalid)
          last = i+3;
          if (first < 0) {
            // If we haven't found our first valid ray, set first to be the
            // first ray that is valid.
            firstSSE = i;
            if (maskResults == 0) {
              // All rays are valid, so just use the first one
              first = i;
              validOx = load44(&sourceData->hitPosition[0][i]);
              validOy = load44(&sourceData->hitPosition[1][i]);
              validOz = load44(&sourceData->hitPosition[2][i]);

              validDx = dx;
              validDy = dy;
              validDz = dz;

              validTimes = load44(&sourceData->time[i]);
            }
            else {
              // Only some of the rays in the set are valid, so loop over each
              // ray and figure out which one is our first valid hit.
              for (int r=0; r < 4; r++) {
                if ( ((maskResults>>r)&1) == 0 ) { //r is valid
                  // Set this ray as our first
                  first = i+r;
                  validOx = set4(sourceData->hitPosition[0][i+r]);
                  validOy = set4(sourceData->hitPosition[1][i+r]);
                  validOz = set4(sourceData->hitPosition[2][i+r]);

                  validDx = set4(sourceData->direction[0][i+r]);
                  validDy = set4(sourceData->direction[0][i+r]);
                  validDz = set4(sourceData->direction[0][i+r]);

                  validTimes = set4(sourceData->time[i+r]);

                  break;
                }
              }
            }
          }
        }

        if (maskResults == 0) {
          store44(&shadowData->origin[0][i], load44(&sourceData->hitPosition[0][i]));
          store44(&shadowData->origin[1][i], load44(&sourceData->hitPosition[1][i]));
          store44(&shadowData->origin[2][i], load44(&sourceData->hitPosition[2][i]));

          store44(&shadowData->time[i], load44(&sourceData->time[i]));
        }
        else if (first >= 0){
          store44(&shadowData->origin[0][i],
                  masknot4(mask, load44(&sourceData->hitPosition[0][i]), validOx));
          store44(&shadowData->origin[1][i],
                  masknot4(mask, load44(&sourceData->hitPosition[1][i]), validOy));
          store44(&shadowData->origin[2][i],
                  masknot4(mask, load44(&sourceData->hitPosition[2][i]), validOz));

          store44(&shadowData->direction[0][i], masknot4(mask, dx, validDx));
          store44(&shadowData->direction[1][i], masknot4(mask, dy, validDy));
          store44(&shadowData->direction[2][i], masknot4(mask, dz, validDz));

          store44(&shadowData->time[i],
                  masknot4(mask, load44(&sourceData->time[i]), validTimes));
        }
      }

      for(;i<sourceRays.rayEnd;i++){
        Vector dir = shadowRays.getDirection(i);
        if(Dot(dir, sourceRays.getFFGeometricNormal(i)) > 0) {
          shadowRays.setOrigin(i, sourceRays.getHitPosition(i));
          shadowRays.setTime(i, sourceRays.getTime(i));
          // See comment above.
          shadowRays.setHitMaterial(i, NULL);
          last = i;
          if (first < 0)
            first = i;
        }
        else if (first >= 0) {
          //if we've already found a valid ray, use that to copy valid
          //data into the invalid ray.
          shadowRays.setOrigin(i, shadowRays.getOrigin(first));
          shadowRays.setDirection(i, shadowRays.getDirection(first));
          shadowRays.setTime(i, shadowRays.getTime(first));

          shadowRays.maskRay(i); //this sets minT -MAXT and hitMatl to -1
          //We set minT to 0 instead of -MAXT just in case an algorithm
          //uses the ray hit position to compute things. In which case
          //having the hit position at the origin would probably break
          //things less than having it be infinitely far away in the
          //negative direction.
          shadowRays.overrideMinT(i, 0);//TOOD: verify 0 is ok (should we use -eps?).
        }
        else {
          shadowRays.maskRay(i);
        }
      }

      //It should be usually faster to have the ray packet be aligned
      //to a simd boundary.
      if (firstSSE >= 0)
        first = firstSSE;
    }

#else // ifdef MANTA_SSE
    for(int i = sourceRays.begin(); i < sourceRays.end(); i++){
      // Check to see if the light is on the front face.
      Vector dir = shadowRays.getDirection(i);
      if(Dot(dir, sourceRays.getFFGeometricNormal(i)) > 0) {
        shadowRays.setOrigin(i, sourceRays.getHitPosition(i));
        shadowRays.setTime(i, sourceRays.getTime(i));
        // See comment above.
        shadowRays.setHitMaterial(i, NULL);
        last = i;
        if (first < 0)
          first = i;
      }
      else if (first >= 0) {
        //if we've already found a valid ray, use that to copy valid
        //data into the invalid ray.
        shadowRays.setOrigin(i, shadowRays.getOrigin(first));
        shadowRays.setDirection(i, shadowRays.getDirection(first));
        shadowRays.setTime(i, shadowRays.getTime(first));

        shadowRays.maskRay(i); //this sets minT -MAXT and hitMatl to -1
        //We set minT to 0 instead of -MAXT just in case an algorithm
        //uses the ray hit position to compute things. In which case
        //having the hit position at the origin would probably break
        //things less than having it be infinitely far away in the
        //negative direction.
        shadowRays.overrideMinT(i, 0);//TOOD: verify 0 is ok (should we use -eps?).
      }
      else
        shadowRays.maskRay(i);
    }
#endif // ifdef MANTA_SSE
    j++;
  } while(last == -1 && j < nlights);

  // Send the shadow rays, if any
  if(last != -1){
    shadowRays.resize ( first, last + 1);

    // We need to save the original distances before the rays are cast into the scene again.
    Packet<Real> distance_left;
    if(attenuateShadows) {
      for(int i = shadowRays.begin(); i < shadowRays.end(); ++i) {
        distance_left.set(i, shadowRays.getMinT(i));
      }
    }

    // Cast shadow rays
    context.scene->getObject()->intersect(context, shadowRays);

    // And attenuate if required
    if (attenuateShadows) {
      bool raysActive;
      Real cutoff = context.scene->getRenderParameters().importanceCutoff;
      int currentDepth = shadowRays.getDepth();
      int maxDepth = context.scene->getRenderParameters().maxDepth;
      int pass = 0;
      bool rayAttenuated[RayPacket::MaxSize];
      for(int i = shadowRays.begin(); i < shadowRays.end(); ++i) {
        rayAttenuated[i] = false;
      }
      do {
        pass++;
        if (debugFlag) cerr << "================   pass  "<<pass<<"   =====\n";
        // Those rays that did hit something, we need to compute the
        // attenuation.

        // Compute the attenuation of the shadow rays.
        // This is when none of the rays hit, we can avoid the loop below.
        bool anyHit = false;
        for(int i = shadowRays.begin();i<shadowRays.end();){
          int end = i+1;
          const Material* hit_matl = shadowRays.getHitMaterial(i);
          if(shadowRays.wasHit(i) && !shadowRays.rayIsMasked(i)){
            anyHit = true;
            rayAttenuated[i] = true;
            while(end < shadowRays.end() && shadowRays.wasHit(end) &&
                  shadowRays.getHitMaterial(end) == hit_matl) {
              rayAttenuated[end] = true;
              end++;
            }
            RayPacket subPacket(shadowRays, i, end);
            if (debugFlag) cerr << "attenuating shadow rays ("<<shadowRays.begin()<<", "<<shadowRays.end()<<")\n";
            hit_matl->attenuateShadows(context, subPacket);
          }
          i=end;
        }

        // Propagate shadow rays through surfaces, attenuating along the way.
        raysActive = false;
        if (anyHit) {
          // If there are any rays left that have attenuation, create a
          // new shadow ray and keep it going.
          for(int i = shadowRays.begin();i<shadowRays.end();){
            int end = i+1;
            if(rayAttenuated[i]) {
              if (shadowRays.getColor(i).luminance() > cutoff) {
                if (!raysActive) {
                  raysActive = true;
                  currentDepth++;
                }
                while(end < shadowRays.end() &&
                      rayAttenuated[end] &&
                      (shadowRays.getColor(end).luminance() > cutoff))
                  end++;
                // Change all the origins and reset the hits
                RayPacket subPacket(shadowRays, i, end);
                subPacket.setDepth(currentDepth);
                subPacket.computeHitPositions();
                for(int s_index = subPacket.begin(); s_index < subPacket.end();
                    ++s_index) {
                  subPacket.setHitMaterial(s_index, NULL);
                  subPacket.setOrigin(s_index, subPacket.getHitPosition(s_index));
                  Real new_distance = distance_left.get(s_index) - subPacket.getMinT(s_index);
                  subPacket.overrideMinT(s_index, new_distance);
                  distance_left.set(s_index, new_distance);
                }
                context.scene->getObject()->intersect(context, subPacket);
              } else {
                // The ray has reached its saturation point, mask it off
                rayAttenuated[i] = false;
                shadowRays.maskRay(i);
              }
            }
            i=end;
          } // end foreach (shadowRay)
        } // if (anyHit)
      } while (raysActive && (currentDepth < maxDepth));
    }
  }

  if(j == nlights){
    stateBuffer.state = StateBuffer::Finished;
  } else {
    stateBuffer.state = StateBuffer::Continuing;
    stateBuffer.i1 = j;
  }
}

string HardShadows::getName() const {
  return "hard";
}

string HardShadows::getSpecs() const {
  return "none";
}

