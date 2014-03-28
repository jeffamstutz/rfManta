
#include <Interface/Material.h>
#include <Core/Exceptions/InternalError.h>

using namespace Manta;

Material::Material()
{
}

Material::~Material()
{
}

void Material::sampleBSDF(const RenderContext& context,
                          RayPacket& rays, Packet<Color>& reflectance) const
{
  throw InternalError("computeBSDF not implemented for this material");
}

