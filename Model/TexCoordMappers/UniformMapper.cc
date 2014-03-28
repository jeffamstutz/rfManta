
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/RayPacket.h>

using namespace Manta;

UniformMapper::UniformMapper()
{
}

UniformMapper::~UniformMapper()
{
}

void UniformMapper::computeTexCoords2(const RenderContext&,
                                      RayPacket& rays) const
{
  rays.computeHitPositions();
#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        rays.setTexCoords(i, rays.getHitPosition(i));
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        rays.setTexCoords(i, rays.getHitPosition(i));
      }
      RayPacketData* data = rays.data;
      for(;i<e;i+=4){
        _mm_store_ps(&data->texCoords[0][i], _mm_load_ps(&data->hitPosition[0][i]));
        _mm_store_ps(&data->texCoords[1][i], _mm_load_ps(&data->hitPosition[1][i]));
        _mm_store_ps(&data->texCoords[2][i], _mm_load_ps(&data->hitPosition[2][i]));
      }
      for(;i<rays.rayEnd;i++){
        rays.setTexCoords(i, rays.getHitPosition(i));
      }
    }
#else
    for(int i = rays.begin(); i < rays.end(); i++){
      rays.setTexCoords(i, rays.getHitPosition(i));
    }
#endif
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void UniformMapper::computeTexCoords3(const RenderContext&,
                                      RayPacket& rays) const
{
  rays.computeHitPositions();
#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        rays.setTexCoords(i, rays.getHitPosition(i));
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        rays.setTexCoords(i, rays.getHitPosition(i));
      }
      RayPacketData* data = rays.data;
      for(;i<e;i+=4){
        _mm_store_ps(&data->texCoords[0][i], _mm_load_ps(&data->hitPosition[0][i]));
        _mm_store_ps(&data->texCoords[1][i], _mm_load_ps(&data->hitPosition[1][i]));
        _mm_store_ps(&data->texCoords[2][i], _mm_load_ps(&data->hitPosition[2][i]));
      }
      for(;i<rays.rayEnd;i++){
        rays.setTexCoords(i, rays.getHitPosition(i));
      }
    }
#else
    for(int i = rays.begin(); i < rays.end(); i++){
      rays.setTexCoords(i, rays.getHitPosition(i));
    }
#endif
  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(UniformMapper, TexCoordMapper, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(UniformMapper);
};

void UniformMapper::readwrite(ArchiveElement* archive)
{
  MantaRTTI<TexCoordMapper>::readwrite(archive, *this);
}
