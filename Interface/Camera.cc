
#include <Interface/Camera.h>
#include <Core/Persistent/ArchiveElement.h>

using namespace Manta;

Camera::Camera()
  : aspectRatioMode(Camera::KeepVertical)
{
}

Camera::~Camera()
{
}

BasicCameraData::BasicCameraData(const Vector& eye, const Vector& lookat,
                                 const Vector& up, double hfov, double vfov)
  : eye(eye), lookat(lookat), up(up), hfov(hfov), vfov(vfov)
{
}

MANTA_REGISTER_CLASS(BasicCameraData);

void BasicCameraData::readwrite(ArchiveElement* archive)
{
  archive->readwrite("eye", eye);
  archive->readwrite("lookat", lookat);
  archive->readwrite("up", up);
  archive->readwrite("hfov", hfov);
  if(archive->reading() && !archive->hasField("vfov"))
    vfov = hfov;
  else
    archive->readwrite("vfov", vfov);
}
