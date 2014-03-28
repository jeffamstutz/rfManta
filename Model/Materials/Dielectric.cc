/*
 For more information, please see: http://software.sci.utah.edu

 The MIT License

 Copyright (c) 2005
 Scientific Computing and Imaging Institute, University of Utah.

 License for the specific language governing rights and limitations under
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 */

#include <Model/Materials/Dielectric.h>
#include <Core/Color/ColorSpace_fancy.h>
#include <Core/Math/Expon.h>
#include <Core/Math/ReflectRefract.h>
#include <Interface/AmbientLight.h>
#include <Interface/Context.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Primitive.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
#include <Interface/Scene.h>
#include <Interface/ShadowAlgorithm.h>

#include <iostream>
using namespace Manta;
using std::cerr;

Dielectric::Dielectric(const Texture<Real>* n, const Texture<Real>* nt,
                       const Texture<Color>* sigma_a,
                       Real cutoff)
  : n(n), nt(nt), sigma_a(sigma_a), localCutoffScale(cutoff), doSchlick(false)
{
}

Dielectric::~Dielectric()
{
}

void Dielectric::shade(const RenderContext& context, RayPacket& rays) const
{
  int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;
  if(rays.getDepth() >= context.scene->getRenderParameters().maxDepth) {
    for(int i=rays.begin();i<rays.end();i++)
      rays.setColor(i, Color::black());
    return;
  }
  if (debugFlag) {
    cerr << "Dielectric::shade: depth = "<< rays.getDepth() << "\n";
  }

  rays.computeHitPositions();
  rays.normalizeDirections();
  rays.computeNormals<true>(context);
  rays.computeGeometricNormals<true>(context);

  Packet<Real> n_values;
  Packet<Real> nt_values;
  Packet<Color> sigma_a_values;

  n->mapValues(n_values, context, rays);
  nt->mapValues(nt_values, context, rays);
  sigma_a->mapValues(sigma_a_values, context, rays);

  RayPacketData reflected_data;
  RayPacketData refracted_data;

  RayPacket reflected_rays(reflected_data, RayPacket::UnknownShape,
                           0, 0, rays.getDepth()+1, RayPacket::NormalizedDirections | debugFlag);
  RayPacket refracted_rays(refracted_data, RayPacket::UnknownShape,
                           0, 0, rays.getDepth()+1, RayPacket::NormalizedDirections | debugFlag);

  Color results[RayPacket::MaxSize];
  Color refl_attenuation[RayPacket::MaxSize];
  Color refr_attenuation[RayPacket::MaxSize];

  int refl_source[RayPacket::MaxSize];
  int num_refl = 0;
  int refr_source[RayPacket::MaxSize];
  int num_refr = 0;

  Real cutoff = localCutoffScale * context.scene->getRenderParameters().importanceCutoff;

  // Compute coefficients and set up raypackets
  for(int i=rays.begin();i<rays.end();i++) {
    results[i] = Color::black();

    Vector rayD = rays.getDirection(i);
    Vector normal = rays.getNormal(i);
    Vector geo_normal = rays.getGeometricNormal(i);
    Real costheta = -Dot(normal, rayD);

    // Need to use geometric normal when determining entrance/exit sides.
    // Geometric normal is only really needed when the shading normal gives the
    // wrong answer for determining entrance/exit.
    if (Dot(geo_normal, rayD) * costheta > 0) {
      // This will introduce a visual discontinuity, but is fast and much less
      // wrong than doing nothing.
      normal = geo_normal;
      costheta =  -Dot(normal, rayD);
    }

    if (debugFlag) {
      cerr << "ray      = "<<rayD<<"\n";
      cerr << "normal   = "<<normal<<"\n";
      cerr << "costheta = "<<costheta<<"\n";
    }
    Real eta_inverse;
    Color beers_color;
    bool exiting;
    if ( costheta < 0 ) {
      // Exiting surface
      normal = -normal;
      costheta = -costheta;
      eta_inverse = n_values.data[i]/nt_values.data[i];
      exiting = true;
      beers_color = sigma_a_values.get(i).Pow(rays.getMinT(i));
    } else {
      eta_inverse = nt_values.data[i]/n_values.data[i];
      exiting = false;
      beers_color = Color::white();
    }

    Vector refl_dir = rayD + 2*costheta*normal;
    Real costheta2squared = 1+(costheta*costheta-1)*(eta_inverse*eta_inverse);
    Color in_importance = rays.getImportance(i);
    Vector hitpos = rays.getHitPosition(i);
    if ( costheta2squared < 0 ) {
      // total internal reflection - no attenuation
#if 1
      Color refl_importance = in_importance * beers_color;
      if(refl_importance.luminance() > cutoff){
        reflected_rays.setImportance(num_refl, refl_importance);
        reflected_rays.setTime(num_refl, rays.getTime(i));
        reflected_rays.setRay(num_refl, hitpos, refl_dir);
        reflected_rays.data->ignoreEmittedLight[num_refl] = 0;
        context.sample_generator->setupChildRay(context, rays, reflected_rays, i, num_refl);
        refl_source[num_refl] = i;
        refl_attenuation[num_refl] = beers_color;
        num_refl++;
      }
#endif
      if (debugFlag) cerr << "Total internal reflection: " << costheta2squared << "\n";
    } else {
      Real costheta2 = Sqrt(costheta2squared);
      Real refl;

      if (doSchlick) {
        refl = SchlickReflection(costheta, costheta2, eta_inverse);
      } else {
        refl = FresnelReflection(costheta, costheta2,
                                 n_values.data[i], nt_values.data[i]);
      }
      if (debugFlag) {
        Real schlick_refl, fresnel_refl;
        if (doSchlick) {
          schlick_refl = refl;
          fresnel_refl = FresnelReflection(costheta, costheta2,
                                           n_values.data[i],
                                           nt_values.data[i]);
        } else {
          schlick_refl = SchlickReflection(costheta, costheta2, eta_inverse);
          fresnel_refl = refl;
        }
        cerr << "schlick_refl = "<<schlick_refl<<", fresnel_refl = "<<fresnel_refl<<", refl = "<<refl<<"\n";
      }

#if 1
      // Possibly create reflection ray
      refl_attenuation[num_refl] = beers_color * refl;
      Color refl_importance = in_importance * refl_attenuation[num_refl];
      if(refl_importance.luminance() > cutoff){
        reflected_rays.setImportance(num_refl, refl_importance);
        reflected_rays.setTime(num_refl, rays.getTime(i));
        reflected_rays.setRay(num_refl, hitpos, refl_dir);
        reflected_rays.data->ignoreEmittedLight[num_refl] = 0;
        context.sample_generator->setupChildRay(context, rays, reflected_rays, i, num_refl);
        refl_source[num_refl] = i;
        num_refl++;
      }
#endif

      // Possibly create refraction ray
      refr_attenuation[num_refr] = beers_color * (1-refl);
      Color refr_importance = in_importance * refr_attenuation[num_refr];
      if(refr_importance.luminance() > cutoff){
        refracted_rays.setImportance(num_refr, refr_importance);
        refracted_rays.setTime(num_refr, rays.getTime(i));
        Vector refr_dir = (eta_inverse * rayD +
                           (eta_inverse * costheta - costheta2) * normal);
        context.sample_generator->setupChildRay(context, rays, refracted_rays, i, num_refr);
        refracted_rays.setRay(num_refr, hitpos, refr_dir);
        refracted_rays.data->ignoreEmittedLight[num_refr] = 0;
        if (debugFlag) { cerr << "refr_dir.length() = "<<refr_dir.length()<<"\n"; }
        refr_source[num_refr] = i;
        num_refr++;
      }
    }
  }

  // Resize the packets.
  reflected_rays.resize(num_refl);
  refracted_rays.resize(num_refr);

  // Trace the rays.
  if(num_refl) {
    context.renderer->traceRays(context, reflected_rays);
  }
  if(num_refr) {
    context.renderer->traceRays(context, refracted_rays);
  }

  // compute their results
  for (int i = 0; i < num_refl; i++) {
    results[refl_source[i]] += refl_attenuation[i] * reflected_rays.getColor(i);
  }
  for (int i = 0; i < num_refr; i++) {
    results[refr_source[i]] += refr_attenuation[i] * refracted_rays.getColor(i);
  }

  for(int i = rays.begin(); i < rays.end(); i++)
    rays.setColor(i, results[i]);
}

void Dielectric::attenuateShadows(const RenderContext& context,
                                  RayPacket& shadowRays) const
{
  // This is basically a hack for now.
  Packet<Color> sigma_a_values;
  sigma_a->mapValues(sigma_a_values, context, shadowRays);

  for(int i = shadowRays.begin(); i < shadowRays.end(); ++i){
    if (shadowRays.getFlag(RayPacket::DebugPacket)) cerr << i<<":shadowColor * sigma_a_values = "<<shadowRays.getColor(i).toString()<<" * "<<sigma_a_values.get(i).toString()<<" = ";
    shadowRays.setColor(i, shadowRays.getColor(i) * sigma_a_values.get(i));
    //    shadowRays.setColor(i, Color::black());
    if (shadowRays.getFlag(RayPacket::DebugPacket)) cerr << shadowRays.getColor(i).toString()<<"\n";
  }
}
