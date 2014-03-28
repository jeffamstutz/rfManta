#include <Model/AmbientLights/AmbientOcclusionBackground.h>
#include <Interface/RayPacket.h>
#include <Interface/Context.h>
#include <Interface/Scene.h>
#include <Core/Math/MT_RNG.h>
#include <Core/Math/Trig.h>
#include <Core/Util/AlignedAllocator.h>
#include <Interface/Renderer.h>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;
using namespace Manta;

AmbientOcclusionBackground::AmbientOcclusionBackground(const Color& color, float cutoff_dist,
    int num_dirs, bool bouncen, Background* bg) :
  cutoff(cutoff_dist), color(color), bounce(bouncen), num_directions(num_dirs), background(bg)
{
  for (int i=0; i < numPermutations; ++i) {
    directions[i][0] = NULL;
    directions[i][1] = NULL;
    directions[i][2] = NULL;
  }

  inv_num_directions = 1.0/num_dirs;
  generateDirections(num_dirs);

#ifdef MANTA_SSE
  for (int i=0; i < Color::NumComponents; ++i)
    color4[i] = set4(color[i]);
#endif
  bcolor = color;
  samplingAngle = 2.0f*Pi;
}
AmbientOcclusionBackground::AmbientOcclusionBackground() {  bounce = false; background = NULL; num_directions = 0; cutoff = 0; bcolor = Color(RGB(1,1,1)); samplingAngle = 2.0f*Pi; }

AmbientOcclusionBackground::~AmbientOcclusionBackground()
{
  for (int k=0; k < numPermutations; ++k) {
    if (directions[k][0]) deallocateAligned(directions[k][0]);
    if (directions[k][1]) deallocateAligned(directions[k][1]);
    if (directions[k][2]) deallocateAligned(directions[k][2]);
  }
}

void AmbientOcclusionBackground::generateDirections(int num_dir)
{
  MT_RNG rng;
  num_directions = num_dir;
  int binsY = Sqrt(num_directions);
  int binsX = binsY;

  if (binsX*binsY != num_directions || num_directions%4 != 0) {
    binsX = (binsX/2)*2;
    binsY = (binsY/2)*2;
    if (binsX*binsY < 4) {
      binsX = binsY = 2;
    }
    num_directions = binsX*binsY;
    inv_num_directions = 1.0/num_directions;
    cerr << "Warning, number of samples to use for Ambient Occlusion is not a perfect square and multiple of 4!\n";
    cerr << "Using " << num_directions << " samples instead\n";
  }

  vector< pair<float, float> > sortedSamples(num_directions);

  for (int k=0; k < numPermutations; ++k) {
    if (directions[k][0]) deallocateAligned(directions[k][0]);
    if (directions[k][1]) deallocateAligned(directions[k][1]);
    if (directions[k][2]) deallocateAligned(directions[k][2]);
    // Need 16 byte aligned for SIMD loads.
    directions[k][0] = (Real*)allocateAligned(sizeof(Real)*num_directions, 16);
    directions[k][1] = (Real*)allocateAligned(sizeof(Real)*num_directions, 16);
    directions[k][2] = (Real*)allocateAligned(sizeof(Real)*num_directions, 16);

    int index = 0;

    // wrap back and forth so that each sample is spatially next to its
    // previous sample. Also, break the Y samples in half.
    const int numYBins = binsY/2;

    for (int startY=0; startY < binsY; startY += numYBins) {
      for (int xBin=0; xBin < binsX; ++xBin) {
        if (xBin%2==0)
          for (int yBin=startY; yBin < startY+numYBins; ++yBin) {
            float r1 = rng.next<float>() / binsX;
            float r2 = rng.next<float>() / binsY;
            r1 += static_cast<float>(xBin) / binsX;
            r2 += static_cast<float>(yBin) / binsY;

            sortedSamples[index++] = make_pair(r1, r2);
          }
        else
          for (int yBin=startY+numYBins-1; yBin >= startY; --yBin) {
            float r1 = rng.next<float>() / binsX;
            float r2 = rng.next<float>() / binsY;
            r1 += static_cast<float>(xBin) / binsX;
            r2 += static_cast<float>(yBin) / binsY;

            sortedSamples[index++] = make_pair(r1, r2);
          }
      }
    }

    for (int i = 0; i < num_directions; ++i) {
      float r1 = sortedSamples[i].first;
      float r2 = sortedSamples[i].second;

      float phi = samplingAngle * r1;
      float r   = sqrt(r2);
      float s, c;
      SinCos(phi, s, c);
      float x   = r * c;
      float y   = r * s;
      float z   = 1.0 - r2;
      z = (z > 0.0) ? Sqrt(z) : 0.0;

      directions[k][0][i] = x;
      directions[k][1][i] = y;
      directions[k][2][i] = z;
    }
  }
}

void AmbientOcclusionBackground::computeAmbient(const RenderContext& context,
    RayPacket& rays,
    ColorArray ambient) const
{
  // one bounce diffuse global illumination hack only works when depth==0.
  // This is especially bad with reflections/refractions/transparencies since
  // what could have been completely clear (bright) now becomes very dark.  The
  // more correct way to handle this would be to have some state flag (in the
  // ray packet?) that specifies if we are in the midst of computing the one
  // bounce global illumination.
  if (bounce && rays.getDepth() >= 1)
  {
    for(int i = rays.begin(); i < rays.end(); ++i) {
      for(int j=0; j<Color::NumComponents; j++)
        ambient[j][i] = 0;
    }
    return;
  }

  // Compute normals
  rays.computeFFNormals<true>(context);
  rays.computeHitPositions();
  Color resultingColor = Color::black();

  RayPacketData occlusion_data;
  int flag = RayPacket::NormalizedDirections | RayPacket::ConstantOrigin;

  if (!bounce)
    flag |= RayPacket::AnyHit;

  RayPacket occlusion_rays(occlusion_data, RayPacket::UnknownShape,
      0, RayPacket::MaxSize,
      rays.getDepth()+1,
      flag);

  for(int i = rays.begin(); i < rays.end(); ++i) {
    const int whichDir = static_cast<int>(context.rng->next<float>()*numPermutations);

    // for each position, compute a local coordinate frame
    // and build a set of rays to push into a ray packet
    const Vector W(rays.getFFNormal(i)); // surface ONB
    const Vector U = W.findPerpendicular().normal();
    const Vector V(Cross(W, U));

#ifdef MANTA_SSE
    const sse_t W_x4 = set4(W[0]);
    const sse_t W_y4 = set4(W[1]);
    const sse_t W_z4 = set4(W[2]);

    const sse_t U_x4 = set4(U[0]);
    const sse_t U_y4 = set4(U[1]);
    const sse_t U_z4 = set4(U[2]);

    const sse_t V_x4 = set4(V[0]);
    const sse_t V_y4 = set4(V[1]);
    const sse_t V_z4 = set4(V[2]);
#endif

    // Send out the ambient occlusion tests
    int num_sent = 0;
    while ( num_sent < num_directions) {
      const int start = 0;
      int end = start + RayPacket::MaxSize;

      if ( (end-start + num_sent) > num_directions)
        end = start + ((num_directions-1) % RayPacket::MaxSize) + 1;

      occlusion_rays.resize(start, end);
      occlusion_rays.setAllFlags(flag);
#ifdef MANTA_SSE
      const sse_t origX = set4(rays.data->hitPosition[0][i]);
      const sse_t origY = set4(rays.data->hitPosition[1][i]);
      const sse_t origZ = set4(rays.data->hitPosition[2][i]);

      const sse_t time = set4(rays.data->time[i]);

      assert(start==0);
      const int sse_end = (end) & (~3);
      assert(sse_end == end);  //Normally not always legit! This is a hack for performance right now
      for (int r=0; r < sse_end; r+=4) {
        sse_t dirX = load44(&directions[whichDir][0][num_sent+r]);
        sse_t dirY = load44(&directions[whichDir][1][num_sent+r]);
        sse_t dirZ = load44(&directions[whichDir][2][num_sent+r]);

        sse_t trans_dirX = add4(add4(mul4(dirX, U_x4),
              mul4(dirY, V_x4)),
            mul4(dirZ, W_x4));
        sse_t trans_dirY = add4(add4(mul4(dirX, U_y4),
              mul4(dirY, V_y4)),
            mul4(dirZ, W_y4));
        sse_t trans_dirZ = add4(add4(mul4(dirX, U_z4),
              mul4(dirY, V_z4)),
            mul4(dirZ, W_z4));

        store44(&occlusion_rays.data->direction[0][r], trans_dirX);
        store44(&occlusion_rays.data->direction[1][r], trans_dirY);
        store44(&occlusion_rays.data->direction[2][r], trans_dirZ);

        store44(&occlusion_rays.data->origin[0][r], origX);
        store44(&occlusion_rays.data->origin[1][r], origY);
        store44(&occlusion_rays.data->origin[2][r], origZ);

        store44(&occlusion_rays.data->time[r], time);

        // Store AO ray color so we can have attenuating shadows.
        if (!bounce)
          for (int cc = 0; cc < Color::NumComponents; ++cc)
            store44(&occlusion_rays.data->color[cc][r], color4[cc]);
      }
#else
      for ( int r = start; r < end; r++ ) {
        Vector trans_dir = (directions[whichDir][0][num_sent+r]*U +
            directions[whichDir][1][num_sent+r]*V +
            directions[whichDir][2][num_sent+r]*W);
        occlusion_rays.setRay(r, rays.getHitPosition(i), trans_dir);
        occlusion_rays.setTime(r, rays.getTime(i));

        // Store AO ray color so we can have attenuating shadows.
        if (!bounce)
          for (int cc = 0; cc < Color::NumComponents; ++cc)
            occlusion_rays.data->color[cc][r] = color[cc];
      }
#endif
      // set max distance
      occlusion_rays.resetHits(cutoff);

      // packet is ready, test it for occlusion
      if (bounce) {
        occlusion_rays.initializeImportance(); // Maybe we should copy over the importances?
        context.renderer->traceRays(context, occlusion_rays); // Need shading to occur.
      }
      else if (!bounce)
      {
        if (background)
        {
          background->shade(context, occlusion_rays);
          for (int r = start; r < end; r++) {
            //TODO: SSE
            occlusion_rays.setColor(r, occlusion_rays.getColor(r)*bcolor);
          }
        }
        context.scene->getObject()->intersect(context, occlusion_rays);
      }


      for (int r = start; r < end; r++) {
        if (bounce) {
          if (!occlusion_rays.wasHit(r))
            resultingColor += occlusion_rays.getColor(r)*bcolor;
          else if (occlusion_rays.getMinT(r) < cutoff)
            resultingColor += occlusion_rays.getColor(r);
          else
            resultingColor += color; // won't handle attenuation
        }
        else {
          if (!occlusion_rays.wasHit(r))
            resultingColor += occlusion_rays.getColor(r);
        }
      }
      num_sent += end-start;
    }
    resultingColor *=  inv_num_directions;
    for(int j=0; j<Color::NumComponents; j++)
      ambient[j][i] = resultingColor[j];
  }
}

string AmbientOcclusionBackground::toString() const {
  ostringstream out;
  out << "--------  AmbientOcclusionBackground  ---------\n";
  RGB color(this->color.convertRGB());
  out << "(color) = ";
  out << "("<<color.r()<<", "<<color.g()<<", "<<color.b()<<"), ";
  out << "cutoff = "<<cutoff<<"\n";
  out << "numSamples = "<<num_directions<<"\n";
  return out.str();
}
