
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

#include <Model/Materials/Transparent.h>
#include <Interface/AmbientLight.h>
#include <Interface/Context.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Primitive.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
#include <Interface/ShadowAlgorithm.h>
#include <Model/Textures/Constant.h>

using namespace Manta;

Transparent::Transparent(const Color& color_, ColorComponent alpha_ )
{
  color = new Constant<Color>(color_);
  alpha = new Constant<ColorComponent>(alpha_);
}

Transparent::Transparent(const Texture<Color>* color_,
                         const Texture<ColorComponent> *alpha_ )
  : color(color_), alpha(alpha_)
{
}

Transparent::Transparent(const Texture<Color>* color_, ColorComponent alpha_ )
  : color(color_)
{
  alpha = new Constant<ColorComponent>(alpha_);
}

Transparent::~Transparent()
{
}

void Transparent::shade(const RenderContext& context, RayPacket& rays) const {

  /////////////////////////////////////////////////////////////////////////////
  // Determine lambertian contribution.

  // Compute normals
  rays.computeFFNormals<true>(context);
  rays.computeHitPositions();

  // Compute colors
  Packet<Color> diffuse;
  color->mapValues(diffuse, context, rays);

  // Compute ambient contributions for all rays
  MANTA_ALIGN(16) ColorArray totalLight;
  activeLights->getAmbientLight()->computeAmbient(context, rays, totalLight);

  // We normalized directions for proper dot product computation.
  rays.normalizeDirections();

  ShadowAlgorithm::StateBuffer shadowState;
  do {
    RayPacketData shadowData;
    RayPacket shadowRays(shadowData, RayPacket::UnknownShape, 0, 0, rays.getDepth(), 0);

    // Call the shadowalgorithm(sa) to generate shadow rays.  We may not be
    // able to compute all of them, so we pass along a buffer for the sa
    // object to store it's state.  The firstTime flag tells the sa to fill
    // in the state rather than using anything in the state buffer.  Most
    // sas will only need to store an int or two in the statebuffer.
    context.shadowAlgorithm->computeShadows(context, shadowState, activeLights,
                                            rays, shadowRays);

    // We need normalized directions for proper dot product computation.
    shadowRays.normalizeDirections();

    for(int j=shadowRays.begin(); j < shadowRays.end(); j++){
      if(!shadowRays.wasHit(j)){
        // Not in shadow, so compute the direct and specular contributions.
        Vector normal = rays.getFFNormal(j);
        Vector shadowdir = shadowRays.getDirection(j);
        ColorComponent cos_theta = Dot(shadowdir, normal);
        Color light = shadowRays.getColor(j);
        for(int k = 0; k < Color::NumComponents;k++)
          totalLight[k][j] += light[k]*cos_theta;
      }
    }
  } while(!shadowState.done());

  // Sum up diffuse/specular contributions
  for(int i = rays.begin(); i < rays.end(); i++){
    Color result;
    for(int j=0;j<Color::NumComponents;j++) {
      result[j] = totalLight[j][i] * diffuse.colordata[j][i];
    }

    rays.setColor( i, result );
  }

  // Check to see what the alpha value is for this location.
  Packet<ColorComponent> alpha_values;
  alpha->mapValues( alpha_values, context, rays );

  RayPacketData secondaryData;
  RayPacket secondaryRays(secondaryData, RayPacket::UnknownShape, 0, 0, rays.getDepth(), 0);
  int map[RayPacket::MaxSize];

  // Shoot a secondary ray for all non 1.0 alpha values.
  int size = 0;
  for (int i=rays.begin();i<rays.end();++i) {
    if (alpha_values.data[i] < (ColorComponent)1.0) {
      secondaryRays.setOrigin   ( size, rays.getHitPosition( i ) );
      secondaryRays.setDirection( size, rays.getDirection  ( i ) );
      secondaryRays.setTime     ( size, rays.getTime       ( i ) );
      secondaryRays.setImportance(size, rays.getImportance ( i )*(1-alpha_values.data[i]));
      secondaryRays.data->ignoreEmittedLight[size] = rays.data->ignoreEmittedLight[i];
      map[size] = i;
      ++size;
    }
  }

  // Send the secondary rays.
  secondaryRays.resize( size );
  context.sample_generator->setupChildPacket(context, rays, secondaryRays);
  context.renderer->traceRays( context, secondaryRays );

  // Blend the result of each secondary ray.
  for (int i=secondaryRays.begin();i<secondaryRays.end();++i) {

    Color second = secondaryRays.getColor( i );
    Color first  = rays.getColor( map[i] );

    // Blend the secondary ray color based on the alpha.
    rays.setColor( map[i],
                   (second*((ColorComponent)1.0-alpha_values.data[map[i]])) +
                   (first*(alpha_values.data[map[i]])));
  }

}


void Transparent::attenuateShadows(const RenderContext& context,
                                   RayPacket& shadowRays) const
{
  // Check to see what the alpha and color value are for this location.
  Packet<ColorComponent> alpha_values;
  alpha->mapValues( alpha_values, context, shadowRays );

  Packet<Color> diffuse;
  color->mapValues(diffuse, context, shadowRays);

  for(int i = shadowRays.begin(); i < shadowRays.end(); ++i) {
    const ColorComponent alpha = alpha_values.get(i);
    shadowRays.setColor(i,
                        (ColorComponent(1)-alpha) * shadowRays.getColor(i) *
                        (alpha*diffuse.get(i) + (ColorComponent(1)-alpha)*Color::white()));
  }
}
