/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

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

#include <Model/Backgrounds/TextureBackground.h>
#include <Interface/RayPacket.h>
#include <MantaTypes.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>
#include <Core/Geometry/Vector.h>

using namespace Manta;

TextureBackground::TextureBackground(const Texture<Color>* colortex,
                                     const TexCoordMapper* mapper_in)
  : colortex(colortex)
{
  if (mapper_in)
    mapper = mapper_in;
  else
    mapper = this;
}

TextureBackground::~TextureBackground()
{
}

void TextureBackground::preprocess(const PreprocessContext&)
{
}

// Globe mapping
void TextureBackground::computeTexCoords2(const RenderContext&,
                                          RayPacket& rays) const
{
  rays.computeHitPositions();
  for(int i = rays.begin();i<rays.end();i++){
    Vector n = rays.getHitPosition(i);
    Real angle = Clamp(n.z(), (Real)-1, (Real)1);
    Real theta = Acos(angle);
    Real phi = Atan2(n.x(), n.y());
    Real x = (phi+(Real)M_PI)*((Real)0.5*(Real)M_1_PI);
    Real y = theta*(Real)M_1_PI;
    rays.setTexCoords(i, VectorT<Real, 2>(x, y));
  }
}

void TextureBackground::shade(const RenderContext& context,
                              RayPacket& rays) const
{
  rays.normalizeDirections();

  // Copy the ray directions and set the TexCoordMapper pointer back
  // to this class for each ray.
  for(int i=rays.begin();i<rays.end();i++){
    // Set the TexCoordMapper pointer.
    rays.setHitTexCoordMapper(i, mapper);
    // Copy the direction from old ray to the new one.
    rays.setHitPosition(i, rays.getDirection(i));
  }
  // Tell the RayPacket that we have the hit positions since we just
  // fed them in there.
  rays.setFlag(RayPacket::HaveHitPositions);

  // Now get the colors from the texture.
  Packet<Color> bg_color;
  colortex->mapValues(bg_color, context, rays);

  // Copy the colors over.
  for(int i=rays.begin();i<rays.end();i++){
    rays.setColor(i, bg_color.get(i));
  }
}

