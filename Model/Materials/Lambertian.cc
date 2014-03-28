/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2006
  Scientific Computing and Imaging Institute, University of Utah

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

#include <Model/Materials/Lambertian.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Primitive.h>
#include <Interface/RayPacket.h>
#include <Interface/AmbientLight.h>
#include <Interface/Context.h>
#include <Interface/SampleGenerator.h>
#include <Interface/ShadowAlgorithm.h>
#include <Core/Math/Expon.h>
#include <Core/Math/Trig.h>
#include <Core/Math/TrigSSE.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Interface/InterfaceRTTI.h>
#include <Model/Textures/Constant.h>
#include <iostream>
using namespace Manta;
using std::cerr;

Lambertian::Lambertian(const Color& color)
{
  colortex = new Constant<Color>(color);
}

Lambertian::Lambertian(const Texture<Color>* colortex)
  : colortex(colortex)
{
}

Lambertian::~Lambertian()
{
}

void Lambertian::shade(const RenderContext& context, RayPacket& rays) const
{
  int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;
  if (debugFlag) {
    cerr << "Lambertian::shade called (rays["<<rays.begin()<<", "<<rays.end()<<"])\n";
    //    cerr << getStackTrace();
  }
  // Shade a bunch of rays.  We know that they all have the same intersected
  // object and are all of the same material

  // We normalized directions for proper dot product computation.
  rays.normalizeDirections();

  // Compute colors
  Packet<Color> diffuse;
  colortex->mapValues(diffuse, context, rays);

  // Compute normals
  rays.computeFFNormals<true>(context);

  // Compute ambient contributions for all rays
  MANTA_ALIGN(16) ColorArray totalLight;
  activeLights->getAmbientLight()->computeAmbient(context, rays, totalLight);

  ShadowAlgorithm::StateBuffer shadowState;
  do {
    RayPacketData shadowData;
    RayPacket shadowRays(shadowData, RayPacket::UnknownShape, 0, 0, rays.getDepth(), debugFlag);

    // Call the shadowalgorithm(sa) to generate shadow rays.  We may not be
    // able to compute all of them, so we pass along a buffer for the sa
    // object to store it's state.
    context.shadowAlgorithm->computeShadows(context, shadowState, activeLights,
                                            rays, shadowRays);

    // We need normalized directions for proper dot product computation.
    shadowRays.normalizeDirections();

#ifdef MANTA_SSE
    //we iterate over the shadowRays instead of the rays for both
    //performance reasons (the shadowRays iteration bounds are a
    //subset of the rays bounds), and for correctness reasons
    //(shadowRays might not be defined for certain rays since they are
    //a subset).
    int b = (shadowRays.rayBegin + 3) & (~3);
    int e = shadowRays.rayEnd & (~3);
    if(b >= e){
      for(int i = shadowRays.begin(); i < shadowRays.end(); i++){
        if(!shadowRays.wasHit(i)){
          // Not in shadow, so compute the direct lighting contributions.
          Vector normal = rays.getFFNormal(i);
          Vector shadowdir = shadowRays.getDirection(i);
          ColorComponent cos_theta = Dot(shadowdir, normal);
          Color light = shadowRays.getColor(i);
          for(int k = 0; k < Color::NumComponents;k++)
            totalLight[k][i] += light[k]*cos_theta;
        }
      }
    } else {
      int i = shadowRays.rayBegin;
      for(;i<b;i++){
        if(!shadowRays.wasHit(i)){
          // Not in shadow, so compute the direct lighting contributions.
          Vector normal = rays.getFFNormal(i);
          Vector shadowdir = shadowRays.getDirection(i);
          ColorComponent cos_theta = Dot(shadowdir, normal);
          Color light = shadowRays.getColor(i);
          for(int k = 0; k < Color::NumComponents;k++)
            totalLight[k][i] += light[k]*cos_theta;
        }
      }
      RayPacketData* data = rays.data;
      RayPacketData* shadowData = shadowRays.data;
      for(;i<e;i+=4){

        // We are interested in the rays that didn't hit anything
        __m128 mask = shadowRays.wereNotHitSSE(i);

        if(_mm_movemask_ps(mask) == 0)
          // All hit points are in shadow
          continue;

        // Not in shadow, so compute the direct light contributions.

        // Vector normal = rays.getNormal(i)
        __m128 normalx = _mm_load_ps(&data->ffnormal[0][i]);
        __m128 normaly = _mm_load_ps(&data->ffnormal[1][i]);
        __m128 normalz = _mm_load_ps(&data->ffnormal[2][i]);
        // Vector shadowdir = shadowRays.getDirection(i);
        __m128 sdx = _mm_load_ps(&shadowData->direction[0][i]);
        __m128 sdy = _mm_load_ps(&shadowData->direction[1][i]);
        __m128 sdz = _mm_load_ps(&shadowData->direction[2][i]);
        // ColorComponent cos_theta = Dot(shadowdir, normal);
        __m128 cos_theta = _mm_add_ps(_mm_add_ps(_mm_mul_ps(sdx, normalx), _mm_mul_ps(sdy, normaly)), _mm_mul_ps(sdz, normalz));
        // Color light = shadowRays.getColor(i);
        __m128 lightr = _mm_load_ps(&shadowData->color[0][i]);
        __m128 lightg = _mm_load_ps(&shadowData->color[1][i]);
        __m128 lightb = _mm_load_ps(&shadowData->color[2][i]);
        // for(int k = 0; k < Color::NumComponents;k++)
        //   totalLight[k][i] += light[k]*cos_theta;
        if(_mm_movemask_ps(mask) == 0xf){
          // All the rays are not in shadow
          _mm_store_ps(&totalLight[0][i], _mm_add_ps(_mm_load_ps(&totalLight[0][i]), _mm_mul_ps(lightr, cos_theta)));
          _mm_store_ps(&totalLight[1][i], _mm_add_ps(_mm_load_ps(&totalLight[1][i]), _mm_mul_ps(lightg, cos_theta)));
          _mm_store_ps(&totalLight[2][i], _mm_add_ps(_mm_load_ps(&totalLight[2][i]), _mm_mul_ps(lightb, cos_theta)));
        } else {
          // Some rays are in shadow so only copy over data for those that are.
          _mm_maskmoveu_si128(_mm_castps_si128(_mm_add_ps(_mm_load_ps(&totalLight[0][i]), _mm_mul_ps(lightr, cos_theta))), _mm_castps_si128(mask), (char*)&totalLight[0][i]);
          _mm_maskmoveu_si128(_mm_castps_si128(_mm_add_ps(_mm_load_ps(&totalLight[1][i]), _mm_mul_ps(lightg, cos_theta))), _mm_castps_si128(mask), (char*)&totalLight[1][i]);
          _mm_maskmoveu_si128(_mm_castps_si128(_mm_add_ps(_mm_load_ps(&totalLight[2][i]), _mm_mul_ps(lightb, cos_theta))), _mm_castps_si128(mask), (char*)&totalLight[2][i]);
        }
      }
      // Pick up the trailing bits
      for(;i<shadowRays.rayEnd;i++){
        if(!shadowRays.wasHit(i)){
          // Not in shadow, so compute the direct lighting contributions.
          Vector normal = rays.getFFNormal(i);
          Vector shadowdir = shadowRays.getDirection(i);
          ColorComponent cos_theta = Dot(shadowdir, normal);
          Color light = shadowRays.getColor(i);
          for(int k = 0; k < Color::NumComponents;k++)
            totalLight[k][i] += light[k]*cos_theta;
        }
      }
    }
#else // ifdef MANTA_SSE
    for(int i=shadowRays.begin(); i < shadowRays.end(); i++){
      if(!shadowRays.wasHit(i)){
        // Not in shadow, so compute the direct lighting contributions.
        Vector normal = rays.getFFNormal(i);
        Vector shadowdir = shadowRays.getDirection(i);
        ColorComponent cos_theta = Dot(shadowdir, normal);
        Color light = shadowRays.getColor(i);
        for(int k = 0; k < Color::NumComponents;k++)
          totalLight[k][i] += light[k]*cos_theta;
      }
    }
#endif // ifdef MANTA_SSE
  } while(!shadowState.done());

#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = rays.rayEnd & (~3);
  if(b >= e){
    for(int i = rays.begin(); i < rays.end(); i++){
      Color result;
      for(int j=0;j<Color::NumComponents;j++)
        result[j] = totalLight[j][i] * diffuse.colordata[j][i];
      rays.setColor(i, result);
    }
  } else {
    int i = rays.rayBegin;
    for(;i<b;i++){
      Color result;
      for(int j=0;j<Color::NumComponents;j++)
        result[j] = totalLight[j][i] * diffuse.colordata[j][i];
      rays.setColor(i, result);
    }
    RayPacketData* data = rays.data;
    for(;i<e;i+=4){
      _mm_store_ps(&data->color[0][i], _mm_mul_ps(_mm_load_ps(&totalLight[0][i]),
                                                  _mm_load_ps(&diffuse.colordata[0][i])));
      _mm_store_ps(&data->color[1][i], _mm_mul_ps(_mm_load_ps(&totalLight[1][i]),
                                                  _mm_load_ps(&diffuse.colordata[1][i])));
      _mm_store_ps(&data->color[2][i], _mm_mul_ps(_mm_load_ps(&totalLight[2][i]),
                                                  _mm_load_ps(&diffuse.colordata[2][i])));
    }
    for(;i<rays.rayEnd;i++){
      Color result;
      for(int j=0;j<Color::NumComponents;j++)
        result[j] = totalLight[j][i] * diffuse.colordata[j][i];
      rays.setColor(i, result);
    }
  }
#else
  for(int i = rays.begin(); i < rays.end(); i++){
    Color result;
    for(int j=0;j<Color::NumComponents;j++)
      result[j] = totalLight[j][i] * diffuse.colordata[j][i];
    rays.setColor(i, result);
  }
#endif

}

void Lambertian::sampleBSDF(const RenderContext& context,
                          RayPacket& rays, Packet<Color>& reflectance) const
{
  rays.computeHitPositions();

  Packet<Real> r1;
  context.sample_generator->nextSeeds(context, r1, rays);
  Packet<Real> r2;
  context.sample_generator->nextSeeds(context, r2, rays);
  rays.computeFFNormals<true>(context);

#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = rays.rayEnd & (~3);
  if(b >= e){
    for(int i=rays.begin();i<rays.end();i++){
      Real sintheta2 = r1.get(i);
      Real sintheta=Sqrt(sintheta2);
      Real costheta=Sqrt(1-sintheta2);
      Real phi=2*M_PI*r2.get(i);
      Real cosphi=Cos(phi)*sintheta;
      Real sinphi=Sin(phi)*sintheta;
      Vector normal = rays.getFFNormal(i);
      Vector v1 = normal.findPerpendicular();
      v1.normalize();
      Vector v2 = Cross(v1, normal);
      rays.setDirection(i, normal * costheta + v1 * cosphi + v2 * sinphi);
      rays.setOrigin(i, rays.getHitPosition(i));
    }
  } else {
    int i = rays.rayBegin;
    for(;i<b;i++){
      Real sintheta2 = r1.get(i);
      Real sintheta=Sqrt(sintheta2);
      Real costheta=Sqrt(1-sintheta2);
      Real phi=2*M_PI*r2.get(i);
      Real cosphi=Cos(phi)*sintheta;
      Real sinphi=Sin(phi)*sintheta;
      Vector normal = rays.getFFNormal(i);
      Vector v1 = normal.findPerpendicular();
      v1.normalize();
      Vector v2 = Cross(v1, normal);
      rays.setDirection(i, normal * costheta + v1 * cosphi + v2 * sinphi);
      rays.setOrigin(i, rays.getHitPosition(i));
    }

    for(;i<e;i+=4){
      sse_t sintheta2 = load44(&r1.data[i]);
      sse_t sintheta=sqrt4(sintheta2);
      sse_union costheta;
      costheta.sse = sqrt4(sub4(_mm_one,sintheta2));
      sse_t phi= mul4(set4(2*M_PI), load44(&r2.data[i]));
      sse_union cosphi, sinphi;
      sincos4(phi, &sinphi.sse, &cosphi.sse);
      cosphi.sse = mul4(cosphi.sse, sintheta);
      sinphi.sse = mul4(sinphi.sse, sintheta);

      for (int k=0; k < 4; ++k) {
        //TODO: Turn this into SSE code.
        Vector normal = rays.getFFNormal(i+k);
        Vector v1 = normal.findPerpendicular();
        v1.normalize();
        Vector v2 = Cross(v1, normal);
        rays.setDirection(i+k, normal * costheta.f[k] + v1 * cosphi.f[k] + v2 * sinphi.f[k]);
        rays.setOrigin(i+k, rays.getHitPosition(i+k));
      }
    }
    for(;i<rays.rayEnd;i++){
      Real sintheta2 = r1.get(i);
      Real sintheta=Sqrt(sintheta2);
      Real costheta=Sqrt(1-sintheta2);
      Real phi=2*M_PI*r2.get(i);
      Real cosphi=Cos(phi)*sintheta;
      Real sinphi=Sin(phi)*sintheta;
      Vector normal = rays.getFFNormal(i);
      Vector v1 = normal.findPerpendicular();
      v1.normalize();
      Vector v2 = Cross(v1, normal);
      rays.setDirection(i, normal * costheta + v1 * cosphi + v2 * sinphi);
      rays.setOrigin(i, rays.getHitPosition(i));
    }
  }
#else
  for(int i=rays.begin();i<rays.end();i++){
    Real sintheta2 = r1.get(i);
    Real sintheta=Sqrt(sintheta2);
    Real costheta=Sqrt(1-sintheta2);
    Real phi=2*M_PI*r2.get(i);
    Real cosphi=Cos(phi)*sintheta;
    Real sinphi=Sin(phi)*sintheta;
    Vector normal = rays.getFFNormal(i);
    Vector v1 = normal.findPerpendicular();
    v1.normalize();
    Vector v2 = Cross(v1, normal);
    rays.setDirection(i, normal * costheta + v1 * cosphi + v2 * sinphi);
    rays.setOrigin(i, rays.getHitPosition(i));
  }
#endif
  colortex->mapValues(reflectance, context, rays);
}


namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(Lambertian, LitMaterial, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(Lambertian);
}

void Lambertian::readwrite(ArchiveElement* archive)
{
  MantaRTTI<LitMaterial>::readwrite(archive, *this);
  archive->readwrite("color", colortex);
}
