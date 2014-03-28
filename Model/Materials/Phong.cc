
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

// TODO
// SSE reflection code
// SSE run length in computeNormal

#include <Model/Materials/Phong.h>
#include <Core/Math/ipow.h>
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
#include <Model/Textures/Constant.h>
#include <Core/Util/NotFinished.h>
#include <Core/Color/ColorSpace_fancy.h>
#include <MantaSSE.h>

using namespace Manta;

#define USE_MASKEDSTORE 0
#define USE_INTMASKEDSTORE 0

#if 0
static inline void maskedStore_si128(__m128i mask, __m128i* oldD, __m128i newD)
{
  _mm_store_si128(oldD,
                  _mm_or_si128(_mm_and_si128(mask, newD),
                               _mm_andnot_si128(mask, _mm_load_si128(oldD))));
}
static inline void maskedStore_ps(__m128 mask, float* oldD, __m128 newD) {
  _mm_store_ps(oldD,
               _mm_or_ps(_mm_and_ps(mask, newD),
                         _mm_andnot_ps(mask, _mm_load_ps(oldD))));
}
#else

#define maskedStore_si128(mask, oldD, newD)                         \
  _mm_store_si128(oldD,                                             \
                  _mm_or_si128(_mm_and_si128(mask, newD),           \
                               _mm_andnot_si128(mask, _mm_load_si128(oldD))))

#define maskedStore_ps(mask, oldD, newD)                            \
  _mm_store_ps(oldD,                                                \
               _mm_or_ps(_mm_and_ps(mask, newD),                    \
                         _mm_andnot_ps(mask, _mm_load_ps(oldD))))

#endif

Phong::Phong(const Color& diffuse, const Color& specular,
             int specpow, ColorComponent refl)
  : specpow(specpow)
{
  diffusetex = new Constant<Color>(diffuse);
  speculartex = new Constant<Color>(specular);
  refltex = new Constant<ColorComponent>(refl);
  do_refl = (refl != 0);
  //highlight_threshold = Pow(COLOR_EPSILON, (Real)1.f/specpow);
  highlight_threshold = 0;
}

Phong::Phong(const Texture<Color>* diffusetex,
             const Texture<Color>* speculartex,
             int specpow, const Texture<ColorComponent>* refltex)
  : diffusetex(diffusetex), speculartex(speculartex), refltex(refltex),
    specpow(specpow)
{
  do_refl=true;
  if (refltex) {
    const Constant<ColorComponent>* rtest =
      dynamic_cast<const Constant<ColorComponent>*>(refltex);
    if(rtest && rtest->getValue() == 0)
      do_refl =false;
  } else {
    do_refl = false;
  }
  //highlight_threshold = Pow(COLOR_EPSILON, (Real)1.f/specpow);
  highlight_threshold = 0;
}

Phong::~Phong()
{
}

void Phong::setReflective(const Texture<ColorComponent>* tex) {
  refltex = tex;
  do_refl=true;
  if (refltex) {
    const Constant<ColorComponent>* rtest =
      dynamic_cast<const Constant<ColorComponent>*>(refltex);
    if(rtest && rtest->getValue() == 0)
      do_refl =false;
  } else {
    do_refl = false;
  }
}

void Phong::shade(const RenderContext& context, RayPacket& rays) const
{
  int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;
  if (debugFlag) {
    cerr << "Phong::shade called\n";
    //    cerr << getStackTrace();
  }
  // Shade a bunch of rays.  We know that they all have the same intersected
  // object and are all of the same material

  // We need normalized directions for proper dot product computation.
  rays.normalizeDirections();

  // Compute colors
  Packet<Color> diffuse;
  diffusetex->mapValues(diffuse, context, rays);
  Packet<Color> specular;
  speculartex->mapValues(specular, context, rays);

  // Compute normals
  rays.computeFFNormals<true>(context);

  // Compute ambient contributions for all rays
  MANTA_ALIGN(16) ColorArray ambientAndDiffuseLight;
  activeLights->getAmbientLight()->computeAmbient(context, rays, ambientAndDiffuseLight);

  // Initialize specular to zero
  MANTA_ALIGN(16) ColorArray specularLight;
  for(int i = rays.begin(); i < rays.end(); i++)
    for(int j=0;j<Color::NumComponents;j++)
      specularLight[j][i] = 0;

  ShadowAlgorithm::StateBuffer shadowState;
  do {
    RayPacketData shadowData;
    RayPacket shadowRays(shadowData, RayPacket::UnknownShape, 0, 0, rays.getDepth(), debugFlag);

    // Call the shadowalgorithm(sa) to generate shadow rays.  We may not be
    // able to compute all of them, so we pass along a buffer for the sa
    // object to store it's state.  The firstTime flag tells the sa to fill
    // in the state rather than using anything in the state buffer.  Most
    // sas will only need to store an int or two in the statebuffer.
    context.shadowAlgorithm->computeShadows(context, shadowState, activeLights,
                                            rays, shadowRays);

    // We need normalized directions for proper dot product computation.
    shadowRays.normalizeDirections();

#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        if(!shadowRays.wasHit(i)){
          // Not in shadow, so compute the direct and specular contributions.
          Vector normal = rays.getFFNormal(i);
          Vector shadowdir = shadowRays.getDirection(i);
          ColorComponent cos_theta = Dot(shadowdir, normal);
          Color light = shadowRays.getColor(i);
          for(int k = 0; k < Color::NumComponents;k++)
            ambientAndDiffuseLight[k][i] += light[k]*cos_theta;
          Vector dir = rays.getDirection(i);
          Vector H = shadowdir-dir;
          ColorComponent cos_alpha = Dot(H, normal);
          if(cos_alpha > highlight_threshold){
            Color::ComponentType length2 = H.length2();
            Color::ComponentType inv_length;
            _mm_store_ss(&inv_length, _mm_rsqrt_ss(_mm_set_ss(length2)));
            Color::ComponentType scale = ipow(cos_alpha*inv_length, specpow);
            for(int k=0;k<Color::NumComponents;k++)
              specularLight[k][i] += light[k] * scale;
          }
        }
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        if(!shadowRays.wasHit(i)){
          // Not in shadow, so compute the direct and specular contributions.
          Vector normal = rays.getFFNormal(i);
          Vector shadowdir = shadowRays.getDirection(i);
          ColorComponent cos_theta = Dot(shadowdir, normal);
          Color light = shadowRays.getColor(i);
          for(int k = 0; k < Color::NumComponents;k++)
            ambientAndDiffuseLight[k][i] += light[k]*cos_theta;
          Vector dir = rays.getDirection(i);
          Vector H = shadowdir-dir;
          ColorComponent cos_alpha = Dot(H, normal);
          if(cos_alpha > highlight_threshold){
            Color::ComponentType length2 = H.length2();
            Color::ComponentType inv_length;
            _mm_store_ss(&inv_length, _mm_rsqrt_ss(_mm_set_ss(length2)));
            Color::ComponentType scale = ipow(cos_alpha*inv_length, specpow);
            for(int k=0;k<Color::NumComponents;k++)
              specularLight[k][i] += light[k] * scale;
          }
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

        // Not in shadow, so compute the direct and specular contributions.

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
          _mm_store_ps(&ambientAndDiffuseLight[0][i], _mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[0][i]), _mm_mul_ps(lightr, cos_theta)));
          _mm_store_ps(&ambientAndDiffuseLight[1][i], _mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[1][i]), _mm_mul_ps(lightg, cos_theta)));
          _mm_store_ps(&ambientAndDiffuseLight[2][i], _mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[2][i]), _mm_mul_ps(lightb, cos_theta)));
        } else {
          // Some rays are in shadow so only copy over data for those that are.
#if USE_MASKEDSTORE
#if USE_INTMASKEDSTORE
          maskedStore_si128(_mm_castps_si128(mask),
                            (__m128i*)&ambientAndDiffuseLight[0][i],
                            _mm_castps_si128(_mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[0][i]), _mm_mul_ps(lightr, cos_theta))));
          maskedStore_si128(_mm_castps_si128(mask),
                            (__m128i*)&ambientAndDiffuseLight[1][i],
                            _mm_castps_si128(_mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[1][i]), _mm_mul_ps(lightg, cos_theta))));
          maskedStore_si128(_mm_castps_si128(mask),
                            (__m128i*)&ambientAndDiffuseLight[2][i],
                            _mm_castps_si128(_mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[2][i]), _mm_mul_ps(lightb, cos_theta))));
#else
          maskedStore_ps(mask,(float*)&ambientAndDiffuseLight[0][i],
                         _mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[0][i]),
                                    _mm_mul_ps(lightr, cos_theta)));
          maskedStore_ps(mask,(float*)&ambientAndDiffuseLight[1][i],
                         _mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[1][i]),
                                    _mm_mul_ps(lightg, cos_theta)));
          maskedStore_ps(mask,(float*)&ambientAndDiffuseLight[2][i],
                         _mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[2][i]),
                                    _mm_mul_ps(lightb, cos_theta)));
#endif
#else // USE_MASKEDSTORE
          _mm_maskmoveu_si128((__m128i) _mm_castps_si128(_mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[0][i]), _mm_mul_ps(lightr, cos_theta))), (__m128i) _mm_castps_si128(mask), (char*)&ambientAndDiffuseLight[0][i]);
          _mm_maskmoveu_si128((__m128i) _mm_castps_si128(_mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[1][i]), _mm_mul_ps(lightg, cos_theta))), (__m128i) _mm_castps_si128(mask), (char*)&ambientAndDiffuseLight[1][i]);
          _mm_maskmoveu_si128((__m128i) _mm_castps_si128(_mm_add_ps(_mm_load_ps(&ambientAndDiffuseLight[2][i]), _mm_mul_ps(lightb, cos_theta))), (__m128i) _mm_castps_si128(mask), (char*)&ambientAndDiffuseLight[2][i]);
#endif
        }

        __m128 Hx = _mm_sub_ps(sdx, _mm_load_ps(&data->direction[0][i]));
        __m128 Hy = _mm_sub_ps(sdy, _mm_load_ps(&data->direction[1][i]));
        __m128 Hz = _mm_sub_ps(sdz, _mm_load_ps(&data->direction[2][i]));

        __m128 cos_alpha = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Hx, normalx), _mm_mul_ps(Hy, normaly)), _mm_mul_ps(Hz, normalz));
        mask = _mm_and_ps(mask, _mm_cmpge_ps(cos_alpha, _mm_set1_ps(highlight_threshold)));
        if(_mm_movemask_ps(mask) == 0)
          continue;

        __m128 length2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Hx, Hx), _mm_mul_ps(Hy, Hy)), _mm_mul_ps(Hz, Hz));
        __m128 inv_length = _mm_rsqrt_ps(length2);
        __m128 scale = _mm_set1_ps(1.0f);
        __m128 x = _mm_mul_ps(cos_alpha, inv_length);
        int p = specpow;
        while(p){
          if(p&1){
            scale = _mm_mul_ps(scale, x);
          }
          x = _mm_mul_ps(x, x);
          p>>=1;
        }

        if(_mm_movemask_ps(mask) == 0xf){
          _mm_store_ps(&specularLight[0][i], _mm_add_ps(_mm_load_ps(&specularLight[0][i]), _mm_mul_ps(lightr, scale)));
          _mm_store_ps(&specularLight[1][i], _mm_add_ps(_mm_load_ps(&specularLight[1][i]), _mm_mul_ps(lightg, scale)));
          _mm_store_ps(&specularLight[2][i], _mm_add_ps(_mm_load_ps(&specularLight[2][i]), _mm_mul_ps(lightb, scale)));
        } else {
#if USE_MASKEDSTORE
#if USE_INTMASKEDSTORE
          maskedStore_si128(_mm_castps_si128(mask),
                            (__m128i*)&specularLight[0][i],
                            _mm_castps_si128(_mm_add_ps(_mm_load_ps(&specularLight[0][i]), _mm_mul_ps(lightr, scale))));
          maskedStore_si128(_mm_castps_si128(mask),
                            (__m128i*)&specularLight[1][i],
                            _mm_castps_si128(_mm_add_ps(_mm_load_ps(&specularLight[1][i]), _mm_mul_ps(lightg, scale))));
          maskedStore_si128(_mm_castps_si128(mask),
                            (__m128i*)&specularLight[2][i],
                            _mm_castps_si128(_mm_add_ps(_mm_load_ps(&specularLight[2][i]), _mm_mul_ps(lightb, scale))));
#else // USE_INTMASKEDSTORE
          maskedStore_ps(mask,(float*)&specularLight[0][i],
                         _mm_add_ps(_mm_load_ps(&specularLight[0][i]),
                                    _mm_mul_ps(lightr, scale)));
          maskedStore_ps(mask,(float*)&specularLight[1][i],
                         _mm_add_ps(_mm_load_ps(&specularLight[1][i]),
                                    _mm_mul_ps(lightg, scale)));
          maskedStore_ps(mask,(float*)&specularLight[2][i],
                         _mm_add_ps(_mm_load_ps(&specularLight[2][i]),
                                    _mm_mul_ps(lightb, scale)));
#endif // USE_INTMASKEDSTORE
#else // USE_MASKEDSTORE
          _mm_maskmoveu_si128((__m128i) _mm_castps_si128(_mm_add_ps(_mm_load_ps(&specularLight[0][i]), _mm_mul_ps(lightr, scale))), (__m128i) _mm_castps_si128(mask), (char*)&specularLight[0][i]);
          _mm_maskmoveu_si128((__m128i) _mm_castps_si128(_mm_add_ps(_mm_load_ps(&specularLight[1][i]), _mm_mul_ps(lightg, scale))), (__m128i) _mm_castps_si128(mask), (char*)&specularLight[1][i]);
          _mm_maskmoveu_si128((__m128i) _mm_castps_si128(_mm_add_ps(_mm_load_ps(&specularLight[2][i]), _mm_mul_ps(lightb, scale))), (__m128i) _mm_castps_si128(mask), (char*)&specularLight[2][i]);
#endif // USE_MASKEDSTORE
        }
      }
      // Pick up the trailing bits
      for(;i<rays.rayEnd;i++){
        if(!shadowRays.wasHit(i)){
          // Not in shadow, so compute the direct and specular contributions.
          Vector normal = rays.getFFNormal(i);
          Vector shadowdir = shadowRays.getDirection(i);
          ColorComponent cos_theta = Dot(shadowdir, normal);
          Color light = shadowRays.getColor(i);
          for(int k = 0; k < Color::NumComponents;k++)
            ambientAndDiffuseLight[k][i] += light[k]*cos_theta;
          Vector dir = rays.getDirection(i);
          Vector H = shadowdir-dir;
          ColorComponent cos_alpha = Dot(H, normal);
          if(cos_alpha > highlight_threshold){
            Color::ComponentType length2 = H.length2();
            Color::ComponentType inv_length;
            _mm_store_ss(&inv_length, _mm_rsqrt_ss(_mm_set_ss(length2)));
            Color::ComponentType scale = ipow(cos_alpha*inv_length, specpow);
            for(int k=0;k<Color::NumComponents;k++)
              specularLight[k][i] += light[k] * scale;
          }
        }
      }
    }
#else // ifdef MANTA_SSE
    for(int i=shadowRays.begin(); i < shadowRays.end(); i++){
      if(!shadowRays.wasHit(i)){
        // Not in shadow, so compute the direct and specular contributions.
        Vector normal = rays.getFFNormal(i);
        Vector shadowdir = shadowRays.getDirection(i);
        ColorComponent cos_theta = Dot(shadowdir, normal);
        Color light = shadowRays.getColor(i);
        for(int k = 0; k < Color::NumComponents;k++)
          ambientAndDiffuseLight[k][i] += light[k]*cos_theta;
        Vector dir = rays.getDirection(i);
        Vector H = shadowdir-dir;
        ColorComponent cos_alpha = Dot(H, normal);
        if(cos_alpha > highlight_threshold){
          Color::ComponentType length = H.length();
          Color::ComponentType scale = ipow(cos_alpha/length, specpow);
          for(int k=0;k<Color::NumComponents;k++)
            specularLight[k][i] += light[k] * scale;
        }
      }
    }
#endif // ifdef MANTA_SSE
  } while(!shadowState.done());

  // Sum up diffuse/specular contributions
#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = rays.rayEnd & (~3);
  if(b >= e){
    for(int i = rays.begin(); i < rays.end(); i++){
      Color result;
      for(int j=0;j<Color::NumComponents;j++)
        result[j] = specularLight[j][i] * specular.colordata[j][i] + ambientAndDiffuseLight[j][i] * diffuse.colordata[j][i];
      rays.setColor(i, result);
    }
  } else {
    int i = rays.rayBegin;
    for(;i<b;i++){
      Color result;
      for(int j=0;j<Color::NumComponents;j++)
        result[j] = specularLight[j][i] * specular.colordata[j][i] + ambientAndDiffuseLight[j][i] * diffuse.colordata[j][i];
      rays.setColor(i, result);
    }
    RayPacketData* data = rays.data;
    for(;i<e;i+=4){
      _mm_store_ps(&data->color[0][i], _mm_add_ps(_mm_mul_ps(_mm_load_ps(&specularLight[0][i]), _mm_load_ps(&specular.colordata[0][i])), _mm_mul_ps(_mm_load_ps(&ambientAndDiffuseLight[0][i]), _mm_load_ps(&diffuse.colordata[0][i]))));
      _mm_store_ps(&data->color[1][i], _mm_add_ps(_mm_mul_ps(_mm_load_ps(&specularLight[1][i]), _mm_load_ps(&specular.colordata[1][i])), _mm_mul_ps(_mm_load_ps(&ambientAndDiffuseLight[1][i]), _mm_load_ps(&diffuse.colordata[1][i]))));
      _mm_store_ps(&data->color[2][i], _mm_add_ps(_mm_mul_ps(_mm_load_ps(&specularLight[2][i]), _mm_load_ps(&specular.colordata[2][i])), _mm_mul_ps(_mm_load_ps(&ambientAndDiffuseLight[2][i]), _mm_load_ps(&diffuse.colordata[2][i]))));
    }
    for(;i<rays.rayEnd;i++){
      Color result;
      for(int j=0;j<Color::NumComponents;j++)
        result[j] = specularLight[j][i] * specular.colordata[j][i] + ambientAndDiffuseLight[j][i] * diffuse.colordata[j][i];
      rays.setColor(i, result);
    }
  }
#else // ifdef MANTA_SSE
  for(int i = rays.begin(); i < rays.end(); i++){
    Color result;
    for(int j=0;j<Color::NumComponents;j++)
      result[j] = specularLight[j][i] * specular.colordata[j][i] + ambientAndDiffuseLight[j][i] * diffuse.colordata[j][i];
    rays.setColor(i, result);
  }
#endif // ifdef MANTA_SSE

  // Compute reflections
  if(do_refl && rays.getDepth() < context.scene->getRenderParameters().maxDepth){
    Packet<ColorComponent> refl;
    refltex->mapValues(refl, context, rays);

    rays.computeHitPositions();
    RayPacketData rdata;
    RayPacket refl_rays(rdata, RayPacket::UnknownShape,
                        rays.begin(), rays.end(),
                        rays.getDepth()+1,
                        RayPacket::NormalizedDirections | debugFlag);
#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        Vector refl_dir = (rays.getDirection(i) -
                           rays.getFFNormal(i)*(2*Dot(rays.getFFNormal(i), rays.getDirection(i) )));
        refl_rays.setRay(i, rays.getHitPosition(i), refl_dir);
        refl_rays.data->ignoreEmittedLight[i] = 0;
        refl_rays.setImportance(i, rays.getImportance(i) * refl.data[i]);
        refl_rays.setTime(i, rays.getTime(i));
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        Vector refl_dir = (rays.getDirection(i) -
                           rays.getFFNormal(i)*(2*Dot(rays.getFFNormal(i), rays.getDirection(i) )));
        refl_rays.setRay(i, rays.getHitPosition(i), refl_dir);
        refl_rays.data->ignoreEmittedLight[i] = 0;
        refl_rays.setImportance(i, rays.getImportance(i) * refl.data[i]);
        refl_rays.setTime(i, rays.getTime(i));
      }
      RayPacketData* data = rays.data;
      RayPacketData* refldata = refl_rays.data;
      for(;i<e;i+=4){
        __m128 dx = _mm_load_ps(&data->direction[0][i]);
        __m128 dy = _mm_load_ps(&data->direction[1][i]);
        __m128 dz = _mm_load_ps(&data->direction[2][i]);
        __m128 normalx = _mm_load_ps(&data->ffnormal[0][i]);
        __m128 normaly = _mm_load_ps(&data->ffnormal[1][i]);
        __m128 normalz = _mm_load_ps(&data->ffnormal[2][i]);
        __m128 cos_theta = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, normalx), _mm_mul_ps(dy, normaly)), _mm_mul_ps(dz, normalz));
        __m128 scale = _mm_mul_ps(_mm_set1_ps(2.0f), cos_theta);
        __m128 rx = _mm_sub_ps(dx, _mm_mul_ps(normalx, scale));
        __m128 ry = _mm_sub_ps(dy, _mm_mul_ps(normaly, scale));
        __m128 rz = _mm_sub_ps(dz, _mm_mul_ps(normalz, scale));
        _mm_store_ps(&refldata->direction[0][i], rx);
        _mm_store_ps(&refldata->direction[1][i], ry);
        _mm_store_ps(&refldata->direction[2][i], rz);

        _mm_store_ps(&refldata->origin[0][i], _mm_load_ps(&data->hitPosition[0][i]));
        _mm_store_ps(&refldata->origin[1][i], _mm_load_ps(&data->hitPosition[1][i]));
        _mm_store_ps(&refldata->origin[2][i], _mm_load_ps(&data->hitPosition[2][i]));

        _mm_store_si128((__m128i*)&(refl_rays.data->ignoreEmittedLight[i]),
                        _mm_setzero_si128());
        __m128 r = _mm_load_ps(&refl.data[i]);
        _mm_store_ps(&refldata->importance[0][i], _mm_mul_ps(_mm_load_ps(&data->importance[0][i]), r));
        _mm_store_ps(&refldata->importance[1][i], _mm_mul_ps(_mm_load_ps(&data->importance[1][i]), r));
        _mm_store_ps(&refldata->importance[2][i], _mm_mul_ps(_mm_load_ps(&data->importance[2][i]), r));

        _mm_store_ps(&refldata->time[i], _mm_load_ps(&data->time[i]));
      }
      for(;i<rays.rayEnd;i++){
        Vector refl_dir = (rays.getDirection(i) -
                           rays.getFFNormal(i)*(2*Dot(rays.getFFNormal(i), rays.getDirection(i) )));
        refl_rays.setRay(i, rays.getHitPosition(i), refl_dir);
        refl_rays.data->ignoreEmittedLight[i] = 0;
        refl_rays.setImportance(i, rays.getImportance(i) * refl.data[i]);
        refl_rays.setTime(i, rays.getTime(i));
      }
    }
#else // ifdef MANTA_SSE
    for(int i = rays.begin(); i < rays.end(); i++){
      Vector refl_dir = (rays.getDirection(i) -
                         rays.getFFNormal(i)*(2*Dot(rays.getFFNormal(i), rays.getDirection(i) )));
      refl_rays.setRay(i, rays.getHitPosition(i), refl_dir);
      refl_rays.data->ignoreEmittedLight[i] = 0;
      refl_rays.setImportance(i, rays.getImportance(i) * refl.data[i]);
      refl_rays.setTime(i, rays.getTime(i));
    }
#endif // ifdef MANTA_SSE
    context.sample_generator->setupChildPacket(context, rays, refl_rays);
    refl_rays.resetHits();
    // Look for runs where the ray importance is significant
    Real cutoff = context.scene->getRenderParameters().importanceCutoff;
    for(int i = refl_rays.begin(); i < refl_rays.end(); ++i) {
      if (refl_rays.getImportance(i).luminance() > cutoff) {
        int end = i+1;
        while(end < refl_rays.end() &&
              refl_rays.getImportance(i).luminance() > cutoff)
          ++end;
        RayPacket subPacket(refl_rays, i, end);
        context.renderer->traceRays(context, subPacket);
        i = end;
      } else {
        refl_rays.setColor(i, Color::black());
      }
    }
#ifdef MANTA_SSE
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        rays.setColor(i, rays.getColor(i) + refl_rays.getColor(i) * refl.data[i]);
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        rays.setColor(i, rays.getColor(i) + refl_rays.getColor(i) * refl.data[i]);
      }
      RayPacketData* data = rays.data;
      RayPacketData* refldata = refl_rays.data;
      for(;i<e;i+=4){
        __m128 r = _mm_load_ps(&refl.data[i]);
        _mm_store_ps(&data->color[0][i], _mm_add_ps(_mm_load_ps(&data->color[0][i]), _mm_mul_ps(_mm_load_ps(&refldata->color[0][i]), r)));
        _mm_store_ps(&data->color[1][i], _mm_add_ps(_mm_load_ps(&data->color[1][i]), _mm_mul_ps(_mm_load_ps(&refldata->color[1][i]), r)));
        _mm_store_ps(&data->color[2][i], _mm_add_ps(_mm_load_ps(&data->color[2][i]), _mm_mul_ps(_mm_load_ps(&refldata->color[2][i]), r)));
      }
      for(;i<rays.rayEnd;i++){
        rays.setColor(i, rays.getColor(i) + refl_rays.getColor(i) * refl.data[i]);
      }
    }
#else // ifdef MANTA_SSE
    for(int i = rays.begin(); i < rays.end(); i++){
      rays.setColor(i, rays.getColor(i) + refl_rays.getColor(i) * refl.data[i]);
    }
#endif // ifdef MANTA_SSE
  }
}
