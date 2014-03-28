
#ifndef Manta_Interface_Material_h
#define Manta_Interface_Material_h

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

#include <Interface/Interpolable.h>
#include <Core/Color/Color.h>

namespace Manta {
  class PreprocessContext;
  class RayPacket;
  class RenderContext;
  class Vector;
  template<class T> class Packet;

  class Material : public Interpolable {
  public:
    Material();
    virtual ~Material();

    virtual void preprocess(const PreprocessContext&) {};
    virtual void shade(const RenderContext& context, RayPacket& rays) const = 0;

    // Multiplies the color in the ray packet (interpreted as the
    // light color) by the material's shadow attenuation.  In the case
    // of the material, it multiplies the light color by zero to
    // represent no light.
    virtual void attenuateShadows(const RenderContext& context,
                                  RayPacket& shadowRays) const = 0;
    // return false if the material is completely opaque.
    virtual bool canAttenuateShadows() const { return true; }

    virtual void sampleBSDF(const RenderContext& context,
                            RayPacket& rays, Packet<Color>& reflectance) const;

  private:
    // Material(const Material&);
    // Material& operator=(const Material&);
  };
}

#endif
