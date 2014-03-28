

/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
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

#include <Model/Materials/Flat.h>

#include <Interface/RayPacket.h>
#include <Interface/Context.h>

#include <Model/Textures/Constant.h>

using namespace Manta;


Flat::Flat(const Color& color)
{
  colortex = new Constant<Color>(color);
}

Flat::Flat(const Texture<Color>* colortex) : colortex(colortex)
{
}

Flat::~Flat()
{
}

void Flat::preprocess(const PreprocessContext&)
{
}

void Flat::shade(const RenderContext& context, RayPacket& rays) const
{
  rays.normalizeDirections();
  rays.computeFFNormals<true>(context);
  // Compute colors
  Packet<Color> colors;
  colortex->mapValues(colors, context, rays);

#ifndef MANTA_SSE
  // Copy the colors into the ray packet.
  for(int i=rays.begin();i<rays.end();i++) {
    ColorComponent cosine = fabs(Dot(rays.getFFNormal(i), rays.getDirection(i)));
    rays.setColor( i, colors.get(i) * cosine );
  }
#else
  int b = (rays.rayBegin + 3) & ~3;
  int e = (rays.rayEnd) & ~3;
  if (b >= e) {
    for (int i = rays.begin(); i < rays.end(); i++) {
      ColorComponent cosine = fabs(Dot(rays.getFFNormal(i), rays.getDirection(i)));
      rays.setColor(i, colors.get(i) * cosine);
    }
  } else {
    int i = rays.rayBegin;
    for (; i < b; i++) {
      ColorComponent cosine = fabs(Dot(rays.getFFNormal(i), rays.getDirection(i)));
      rays.setColor(i, colors.get(i) * cosine);
    }
    // SIMD now
    RayPacketData* data = rays.data;
    for (; i < e; i+= 4) {
      __m128 dot = _mm_mul_ps(_mm_set_ps1(-1.f),
                              _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_load_ps(&data->direction[0][i]),
                                                               _mm_load_ps(&data->ffnormal[0][i])),
                                                    _mm_mul_ps(_mm_load_ps(&data->direction[1][i]),
                                                               _mm_load_ps(&data->ffnormal[1][i]))),
                                         _mm_mul_ps(_mm_load_ps(&data->direction[2][i]),
                                                    _mm_load_ps(&data->ffnormal[2][i]))));

      _mm_store_ps(&data->color[0][i], _mm_mul_ps(dot, _mm_load_ps(&colors.colordata[0][i])));
      _mm_store_ps(&data->color[1][i], _mm_mul_ps(dot, _mm_load_ps(&colors.colordata[1][i])));
      _mm_store_ps(&data->color[2][i], _mm_mul_ps(dot, _mm_load_ps(&colors.colordata[2][i])));
    }
    for (; i < rays.rayEnd; i++) {
      ColorComponent cosine = fabs(Dot(rays.getFFNormal(i), rays.getDirection(i)));
      rays.setColor(i, colors.get(i) * cosine);
    }
  }
#endif
}
