#include <Model/Primitives/TextureCoordinateCylinder.h>
#include <Interface/RayPacket.h>

using namespace Manta;
using namespace std;

TextureCoordinateCylinder::TextureCoordinateCylinder
  (Material* mat, const Vector& bottom, const Vector& top,
   Real radius,
   Vector tc1, Vector tc2)
    : Cylinder(mat, bottom, top, radius), TexCoord1(tc1), TexCoord2(tc2)
{
  this->setTexCoordMapper(this);
}

TextureCoordinateCylinder::~TextureCoordinateCylinder()
{
}

void TextureCoordinateCylinder::computeTexCoords2(const RenderContext&,
			     RayPacket& rays) const
{
  //TODO: interpolate between TC1, and TC2
  for(int i=rays.begin();i<rays.end();i++)
    rays.setTexCoords(i, TexCoord1);
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void TextureCoordinateCylinder::computeTexCoords3(const RenderContext&,
				      RayPacket& rays) const
{
  //TODO: interpolate between TC1, and TC2
  for(int i=rays.begin();i<rays.end();i++)
    rays.setTexCoords(i, TexCoord1);
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}
