
#include <Model/Materials/MetalMaterial.h>
#include <Core/Math/ipow.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Interface/Context.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Primitive.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
#include <Interface/Scene.h>
#include <Interface/ShadowAlgorithm.h>
#include <Model/Textures/Constant.h>

using namespace Manta;

MetalMaterial::MetalMaterial()
{
  specular_reflectance = 0;
}

MetalMaterial::MetalMaterial(const Color& specular, int phong_exponent)
  : phong_exponent(phong_exponent)
{
   specular_reflectance = new Constant<Color>(specular);
}

MetalMaterial::MetalMaterial(const Texture<Color>* specular_reflectance,
                             int phong_exponent)
  : specular_reflectance(specular_reflectance), phong_exponent(phong_exponent)
{
}

MetalMaterial::~MetalMaterial()
{
}

void MetalMaterial::shade(const RenderContext& context, RayPacket& rays) const
{
  // Compute only if we haven't hit the max ray depth.
  if(rays.getDepth() < context.scene->getRenderParameters().maxDepth) {
    rays.normalizeDirections();
    rays.computeFFNormals<true>(context);
    Packet<Color> specular;
    specular_reflectance->mapValues(specular, context, rays);

    rays.computeHitPositions();
    RayPacketData rdata;
    RayPacket refl_rays(rdata, RayPacket::UnknownShape, rays.begin(), rays.end(),
                        rays.getDepth()+1, RayPacket::NormalizedDirections);
    for(int i=rays.begin();i<rays.end();i++) {
      Vector rayD = rays.getDirection(i);
      Vector normal = rays.getFFNormal(i);
      Vector refl_dir = rayD - normal*(2*Dot(normal, rayD));
      refl_rays.setRay(i, rays.getHitPosition(i),  refl_dir);
      refl_rays.data->ignoreEmittedLight[i] = 0;
      refl_rays.setImportance(i, rays.getImportance(i));
      refl_rays.setTime(i, rays.getTime(i));
    }

    refl_rays.resetHits();
    context.sample_generator->setupChildPacket(context, rays, refl_rays);
    context.renderer->traceRays(context, refl_rays);
    for(int i=rays.begin();i<rays.end();i++) {
      // compute Schlick Fresnel approximation
      Real cosine = -Dot(rays.getFFNormal(i), rays.getDirection(i));
      // Since we are using forward facing normals the dot product will
      // always be negative.
      //      if(cosine < 0) cosine =-cosine;
      Real k = 1 - cosine;
      k*=k*k*k*k;

      // Doing the explicit cast to ColorComponent here, so that we
      // don't do things like multiply all the colors by a double,
      // thus promoting those expressions when we don't need to.
      ColorComponent kc = (ColorComponent)k;
      Color R = specular.get(i) * (1-kc) + Color::white()*kc;

      rays.setColor(i, R * refl_rays.getColor(i));
    }
  } else {
    // Stuff black in it.
    for(int i=rays.begin();i<rays.end();i++)
      rays.setColor(i, Color::black());
  }
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(MetalMaterial, LitMaterial, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(MetalMaterial);
}

void MetalMaterial::readwrite(ArchiveElement* archive)
{
  MantaRTTI<LitMaterial>::readwrite(archive, *this);
  archive->readwrite("reflectance", specular_reflectance);
  archive->readwrite("phong_exponent", phong_exponent);
}
