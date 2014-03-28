#include <Model/Materials/CopyTextureMaterial.h>
#include <Model/Textures/Constant.h>

using namespace Manta;

CopyTextureMaterial::CopyTextureMaterial(const Color& color)
{
  colortex = new Constant<Color>(color);
}

CopyTextureMaterial::CopyTextureMaterial(const Texture<Color>* colortex)
  : colortex(colortex)
{
}

void CopyTextureMaterial::preprocess(const PreprocessContext&)
{
}

void CopyTextureMaterial::shade(const RenderContext& context,
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
      rays.setColor( i, colors.get(i) );
    }
  } else {
    int i = rays.rayBegin;
    for (; i < b; i++) {
      rays.setColor( i, colors.get(i) );
    }
    // SIMD now
    RayPacketData* data = rays.data;
    for (; i < e; i+= 4) {
      _mm_store_ps(&data->color[0][i], _mm_load_ps(&colors.colordata[0][i]));
      _mm_store_ps(&data->color[1][i], _mm_load_ps(&colors.colordata[1][i]));
      _mm_store_ps(&data->color[2][i], _mm_load_ps(&colors.colordata[2][i]));
    }
    for (; i < rays.rayEnd; i++) {
      rays.setColor( i, colors.get(i) );
    }
  }
#endif
}
