
#ifndef Manta_Model_Dielectric_h
#define Manta_Model_Dielectric_h

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

#include <Model/Materials/LitMaterial.h>
#include <Core/Color/Color.h>
#include <Interface/Texture.h>
#include <Model/Textures/Constant.h>

namespace Manta
{
  class LightSet;

  class Dielectric : public LitMaterial
  {
  public:
    Dielectric(const Real n, const Real nt, const Color &sigma_a,
               Real localCutoffScale = 1)
      : n(new Constant<Real>(n)),
        nt(new Constant<Real>(nt)),
        sigma_a(new Constant<Color>(sigma_a)),
        localCutoffScale(localCutoffScale),
        doSchlick(false)
        { }

    Dielectric(const Texture<Real>* n, const Texture<Real>* nt,
               const Texture<Color>* sigma_a,
               Real localCutoffScale = 1);
    ~Dielectric();

    void shade(const RenderContext& context, RayPacket& rays) const;
    virtual void attenuateShadows(const RenderContext& context,
                                  RayPacket& shadowRays) const;
    virtual bool canAttenuateShadows() const { return true; }

    void setDoSchlick(bool new_val) { doSchlick = new_val; }
  private:
    const Texture<Real>* n;
    const Texture<Real>* nt;
    const Texture<Color>* sigma_a;
    Real localCutoffScale;
    bool doSchlick;
  };
}

#endif
