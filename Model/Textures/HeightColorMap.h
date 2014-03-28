
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

#ifndef Manta_Model_HeightColorMap_h
#define Manta_Model_HeightColorMap_h

#include <Interface/Texture.h>
#include <Interface/RayPacket.h>
#include <Model/Textures/Constant.h>

namespace Manta {

  class RayPacket;
  class RenderContext;

  class HeightColorMap : public Texture<Color> {
  public:
    HeightColorMap(Real min_height, Real max_height)
    {
      setRange(min_height, max_height);
    }
    virtual ~HeightColorMap() { }

    void setRange(Real min, Real max, int axis=0) {
      min_height = min;
      max_height = max;
      this->axis = axis;
      inv_range = 1.0 / (max_height - min_height);
    }

    static void hsv_to_rgb(float &R, float &G, float &B,
                           float H, float S, float V);

    virtual void mapValues(Packet<Color>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
  private:
    Real min_height, max_height;
    Real inv_range;
    int axis; // 0, 1, or 2.
  };
};

#endif
