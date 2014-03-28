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

#include <Model/Materials/OrenNayar.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Primitive.h>
#include <Interface/RayPacket.h>
#include <Interface/AmbientLight.h>
#include <Interface/Context.h>
#include <Interface/ShadowAlgorithm.h>
#include <Model/Textures/Constant.h>
#include <iostream>
#include <math.h>
#include <Core/Math/MinMax.h>
using namespace Manta;
using std::cerr;

OrenNayar::OrenNayar(const Color& color, const Real sigma)
{
  Real sigma_sqr=sigma*sigma;
  colortex = new Constant<Color>(color);
  a=1-sigma_sqr/(2*(sigma_sqr+0.33));
  b=(0.45*sigma_sqr)/(sigma_sqr+0.09);
}

OrenNayar::OrenNayar(const Texture<Color>* colortex)
  : colortex(colortex)
{
}

OrenNayar::~OrenNayar()
{
}

void OrenNayar::shade(const RenderContext& context, RayPacket& rays) const
{
  int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;
  if (debugFlag) {
    cerr << "OrenNayar::shade called (rays["<<rays.begin()<<", "<<rays.end()<<"])\n";
    //    cerr << getStackTrace();
  }
  // Shade a bunch of rays.  We know that they all have the same intersected
  // object and are all of the same material

  // We normalized directions for proper dot product computation.
  rays.normalizeDirections();

  // Compute colors
  Packet<Color> diffuse;
  colortex->mapValues(diffuse, context, rays);

  // Compute normals
  
  rays.computeFFNormals<true>(context);
  
  // Compute ambient contributions for all rays
  MANTA_ALIGN(16) ColorArray totalLight;
  activeLights->getAmbientLight()->computeAmbient(context, rays, totalLight);

  ShadowAlgorithm::StateBuffer shadowState;
  do { 
    RayPacketData shadowData;
    RayPacket shadowRays(shadowData, RayPacket::UnknownShape, 0, 0, rays.getDepth(), debugFlag);

    // Call the shadowalgorithm(sa) to generate shadow rays.  We may not be
    // able to compute all of them, so we pass along a buffer for the sa
    // object to store it's state.
    context.shadowAlgorithm->computeShadows(context, shadowState, activeLights,
                                            rays, shadowRays);

    // We need normalized directions for proper dot product computation.
    shadowRays.normalizeDirections();

    for(int i=shadowRays.begin(); i < shadowRays.end(); i++){
      if(!shadowRays.wasHit(i)){
        // Not in shadow, so compute the direct lighting contributions.
        Vector normal = rays.getFFNormal(i);
        Vector shadowdir = shadowRays.getDirection(i);
        ColorComponent cos_phi = Dot(shadowdir, normal);
        Color light = shadowRays.getColor(i);
        Real theta=acos(Dot(rays.getNormal(i), rays.getDirection(i)));
        Real phi=acos(cos_phi);
    
        Real alpha, beta;
        (theta > phi)? alpha = theta : alpha = phi;
        (theta < phi)? beta = theta : beta = phi;
    
        Real max_value;
        (0 < cos(theta-phi))? max_value =  cos(theta-phi) : max_value = 0;
    
        for(int k = 0; k < Color::NumComponents;k++)
          totalLight[k][i] += light[k]*(0.6*cos_phi)*(a+b*max_value*sin(alpha)*tan(beta));
      }
    }
  } while(!shadowState.done());


  for(int i = rays.begin(); i < rays.end(); i++){
    Color result;
    for(int j=0;j<Color::NumComponents;j++)
      result[j] = totalLight[j][i] * diffuse.colordata[j][i];
    rays.setColor(i, result);
  }


}
