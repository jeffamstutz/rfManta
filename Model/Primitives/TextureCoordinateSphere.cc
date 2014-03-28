
#include <Model/Primitives/TextureCoordinateSphere.h>
#include <Interface/RayPacket.h>
#include <Interface/Context.h>

using namespace Manta;
using namespace std;

#define USE_RTSL_OUTPUT 1

TextureCoordinateSphere::TextureCoordinateSphere()
{
}

TextureCoordinateSphere::TextureCoordinateSphere
  (Material* material, const Vector& center, Real radius, Vector tc)
 : Sphere(material, center, radius), TexCoords(tc)
{
  this->setTexCoordMapper(this);
}

TextureCoordinateSphere::~TextureCoordinateSphere()
{
}

void TextureCoordinateSphere::computeTexCoords2(const RenderContext&,
                               RayPacket& rays) const
{
  for(int i = rays.begin();i<rays.end();i++){
    rays.setTexCoords(i, TexCoords);
  }
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void TextureCoordinateSphere::computeTexCoords3(const RenderContext&,
                               RayPacket& rays) const
{
  for(int i = rays.begin();i<rays.end();i++){
    rays.setTexCoords(i, TexCoords);
  }
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

