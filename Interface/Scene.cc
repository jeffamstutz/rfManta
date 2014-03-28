
#include <Interface/Scene.h>
#include <Core/Persistent/stdRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Interface/Background.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/LightSet.h>

using namespace Manta;

int Scene::addBookmark(const std::string& name, const Vector& eye, const Vector& lookat,
                       const Vector& up, double hfov, double vfov)
{
  return addBookmark(name, BasicCameraData(eye, lookat, up, hfov, vfov));
}

int Scene::addBookmark(const std::string& name, const BasicCameraData& cam)
{
  int idx = bookmarks.size();
  Bookmark* bookmark = new Bookmark;
  bookmark->name = name;
  bookmark->cameradata = cam;
  bookmarks.push_back(bookmark);
  return idx;
}

void Scene::selectBookmark(unsigned int bookmark)
{
  currentBookmark_id = bookmark;
  if(bookmarks.size() > 0 && currentBookmark_id >= bookmarks.size())
    currentBookmark_id = bookmarks.size()-1;
}

const BasicCameraData* Scene::nextBookmark()
{
  if(bookmarks.size() == 0)
    return 0;

  ++currentBookmark_id;
  if(currentBookmark_id >= bookmarks.size())
    currentBookmark_id = 0;

  return currentBookmark();
}

const BasicCameraData* Scene::currentBookmark()
{
  if(bookmarks.size() == 0)
    return NULL;

  return &bookmarks[currentBookmark_id]->cameradata;
}

void Scene::readwrite(ArchiveElement* archive)
{
  archive->readwrite("object", object);
  archive->readwrite("bg", bg);
  archive->readwrite("lights", lights);
  archive->readwrite("renderParameters", renderParameters);
  archive->readwrite("bookmarks", bookmarks);
  archive->readwrite("currentBookmark", currentBookmark_id);
}

namespace Manta {
  MANTA_DECLARE_RTTI_BASECLASS(Scene::Bookmark, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(Scene::Bookmark);
}

void Scene::Bookmark::readwrite(ArchiveElement* archive)
{
  archive->readwrite("name", name);
  archive->readwrite("cameradata", cameradata);
}
