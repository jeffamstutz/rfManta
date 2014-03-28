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

#include <Model/Instances/InstanceMaterial.h>
#include <Model/Instances/MPT.h>
#include <Interface/RayPacket.h>

using namespace Manta;

InstanceMaterial::InstanceMaterial()
{
  material = this;
}

void InstanceMaterial::shade(const RenderContext& context,
                             RayPacket& rays) const
{
  for(int i=rays.begin();i<rays.end();){
    const Material* matl = rays.scratchpad<MPT>(i).material;
    int end = i+1;
    while(end < rays.end() && rays.scratchpad<MPT>(end).material == matl)
      end++;
    RayPacket subPacket(rays, i, end);
    matl->shade(context, subPacket);
    i = end;
  }
}

void InstanceMaterial::attenuateShadows(const RenderContext& context,
                                        RayPacket& shadowRays) const
{
  for(int i=shadowRays.begin();i<shadowRays.end();){
    const Material* matl = shadowRays.scratchpad<MPT>(i).material;
    int end = i+1;
    while(end < shadowRays.end() && shadowRays.scratchpad<MPT>(end).material == matl)
      end++;
    RayPacket subPacket(shadowRays, i, end);
    matl->attenuateShadows(context, subPacket);
    i = end;
  }
}

void InstanceMaterial::overrideMaterial(Material* in_material)
{
  material = in_material;
}
