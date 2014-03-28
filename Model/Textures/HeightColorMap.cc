
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

#include <Model/Textures/HeightColorMap.h>
#include <Interface/RayPacket.h>
#include <Core/Color/Color.h>

using namespace Manta;


void HeightColorMap::hsv_to_rgb(float &R, float &G, float &B,
                                float H, float S, float V)
{
  H *= 6.0;
  float i = floor(H);
  float f = H - i;

  float p = V * (1.0 - S);
  float q = V * (1.0 - S * f);
  float t = V * (1.0 - S * (1 - f));

  if (i == 0.0)
  {
    R = V;
    G = t;
    B = p;
  }
  else if (i == 1.0)
  {
    R = q;
    G = V;
    B = p;
  }
  else if (i == 2.0)
  {
    R = p;
    G = V;
    B = t;
  }
  else if (i == 3.0)
  {
    R = p;
    G = q;
    B = V;
  }
  else if (i == 4.0)
  {
    R = t;
    G = p;
    B = V;
  }
  else
  {
    // i == 5.0
    R = V;
    G = p;
    B = q;
  }
}


void HeightColorMap::mapValues(Packet<Color>& results,
                               const RenderContext& context,
                               RayPacket& rays) const
{

  for (int i=rays.begin();i<rays.end();++i) {
    rays.computeHitPositions();
    Real height = rays.getHitPosition(i)[axis] - min_height;
    Real normalized_height = height * inv_range;
    float r, g, b;
    hsv_to_rgb(r, g, b, normalized_height, 1, 1);

    results.set(i, Color( RGB( (ColorComponent)r,
                               (ColorComponent)g,
                               (ColorComponent)b )));

  }
}
