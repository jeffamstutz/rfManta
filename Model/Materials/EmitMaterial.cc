#include <Model/Materials/EmitMaterial.h>
#include <Model/Textures/Constant.h>

using namespace Manta;

EmitMaterial::EmitMaterial(const Color& color)
{
  colortex = new Constant<Color>(color);
}

EmitMaterial::EmitMaterial(const Texture<Color>* colortex)
  : colortex(colortex)
{
}

void EmitMaterial::preprocess(const PreprocessContext&)
{
}

void EmitMaterial::shade(const RenderContext& context,
                                RayPacket& rays) const
{
  // Compute colors
  Packet<Color> colors;
  colortex->mapValues(colors, context, rays);

#ifndef MANTA_SSE
  // Copy the colors into the ray packet.
  for(int i=rays.begin();i<rays.end();i++) {
    rays.setColor( i, colors.get(i) );
  }
#else
  int b = (rays.rayBegin + 3) & ~3;
  int e = (rays.rayEnd) & ~3;
  if (b >= e) {
    for (int i = rays.begin(); i < rays.end(); i++) {
      if (rays.getIgnoreEmittedLight(i))
        rays.setColor(i, Color::black());
      else
        rays.setColor(i, colors.get(i) );
    }
  } else {
    int i = rays.rayBegin;
    for (; i < b; i++) {
      if (rays.getIgnoreEmittedLight(i))
        rays.setColor(i, Color::black());
      else
        rays.setColor(i, colors.get(i) );
    }
    // SIMD now
    RayPacketData* data = rays.data;
    for (; i < e; i+= 4) {
      __m128 black_light =
        _mm_cmpneq_ps(_mm_load_ps((float*)&(data->ignoreEmittedLight[i])),
                      _mm_setzero_ps());
      for (unsigned int c = 0; c < 3; c++) {
        __m128 val = _mm_or_ps(_mm_and_ps(black_light, _mm_setzero_ps()),
                               _mm_andnot_ps(black_light, _mm_load_ps(&(colors.colordata[c][i]))));
        _mm_store_ps(&data->color[c][i], val);
      }
    }
    for (; i < rays.rayEnd; i++) {
      if (rays.getIgnoreEmittedLight(i))
        rays.setColor(i, Color::black());
      else
        rays.setColor(i, colors.get(i) );
    }
  }
#endif
}
