
#include <Engine/Renderers/Raytracer.h>
#include <Interface/Background.h>
#include <Interface/Camera.h>
#include <Interface/Context.h>
#include <Interface/Material.h>
#include <Interface/Object.h>
#include <Interface/RayPacket.h>
#include <Interface/Scene.h>
#include <Core/Util/Assert.h>
#include <iostream>
using namespace std;

using namespace Manta;

Renderer* Raytracer::create(const vector<string>& args)
{
  return new Raytracer(args);
}

Raytracer::Raytracer(const vector<string>& /*args*/)
{
}

Raytracer::~Raytracer()
{
}

void Raytracer::setupBegin(const SetupContext&, int)
{
}

void Raytracer::setupDisplayChannel(SetupContext&)
{
}

void Raytracer::setupFrame(const RenderContext&)
{
}

void Raytracer::traceEyeRays(const RenderContext& context, RayPacket& rays)
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  context.camera->makeRays(context, rays);
  rays.initializeImportance();
  traceRays(context, rays);
}

void Raytracer::traceRays(const RenderContext& context, RayPacket& rays)
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
	       << " image coords ("<< rays.getImageCoordinates(i, 0) << ", " << rays.getImageCoordinates(i, 1) << ")"
               << "\n";
        }
      }
      RayPacket subPacket(rays, i, end);
      context.scene->getBackground()->shade(context, subPacket);
      i=end;
    }
  }
}

void Raytracer::traceRays(const RenderContext& context, RayPacket& rays, Real cutoff)
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
