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

#ifndef Manta_Model_InstanceMaterial_h
#define Manta_Model_InstanceMaterial_h

/*
  This constains code that implements the Material interface and is
  common to all the instances.
  
 */

#include <Interface/Material.h>

namespace Manta {

  class InstanceMaterial : public Material {
  public:
    InstanceMaterial();
    virtual ~InstanceMaterial() {}

    virtual void shade(const RenderContext& context, RayPacket& rays) const;
    virtual void attenuateShadows(const RenderContext& context,
                                  RayPacket& shadowRays) const;

    void overrideMaterial(Material* material);
  protected:
    Material* material;
    
  };
} // end namespace Manta

#endif // #ifndef Manta_Model_InstanceMaterial_h
