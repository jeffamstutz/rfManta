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

#include <Model/Materials/OpaqueShadower.h>
#include <Interface/InterfaceRTTI.h>
#include <MantaSSE.h>
#include <Interface/RayPacket.h>
#include <Interface/Context.h>
#include <Core/Color/Color.h>

using namespace Manta;

void OpaqueShadower::attenuateShadows(const RenderContext& context,
                                      RayPacket& shadowRays) const
{
#ifdef MANTA_SSE
  int b = (shadowRays.rayBegin + 3) & (~3);
  int e = shadowRays.rayEnd & (~3);
  if(b >= e){
    for(int i = shadowRays.begin(); i < shadowRays.end(); i++){
      shadowRays.setColor(i, Color::black());
    }
  } else {
    int i = shadowRays.rayBegin;
    for(;i<b;i++){
      shadowRays.setColor(i, Color::black());
    }
    RayPacketData* shadowData = shadowRays.data;
    for(;i<e;i+=4){
      _mm_store_ps(&shadowData->color[0][i], _mm_zero);
      _mm_store_ps(&shadowData->color[1][i], _mm_zero);
      _mm_store_ps(&shadowData->color[2][i], _mm_zero);
    }
    for(;i<shadowRays.rayEnd;i++){
      shadowRays.setColor(i, Color::black());
    }
  }
#else
  for(int i = shadowRays.begin(); i < shadowRays.end(); i++){
    shadowRays.setColor(i, Color::black());
  }
#endif
}

namespace Manta {
  MANTA_REGISTER_CLASS(OpaqueShadower);
}
