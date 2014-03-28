
#ifndef Manta_Interface_Scene_h
#define Manta_Interface_Scene_h

#include <Interface/Camera.h>
#include <Interface/RenderParameters.h>
#include <string>
#include <vector>

namespace Manta {
  class Archive;
  class Background;
  class LightSet;
  class Object;
  class ReadContext;
  class Scene {
  public:
    Scene()
    {
      object = 0;
      bg = 0;
      currentBookmark_id = 0;
      animationStartTime = 0;
      animationEndTime = 100;
    }

    const Object* getObject() const
    {
      return object;
    }
    Object* getObject()
    {
      return object;
    }
    void setObject(Object* newobject)
    {
      object = newobject;
    }

    const Background* getBackground() const
    {
      return bg;
    }
    Background* getBackground()
    {
      return bg;
    }
    void setBackground(Background* newbg)
    {
      bg = newbg;
    }

    const LightSet* getLights() const
    {
      return lights;
    }
    LightSet* getLights()
    {
      return lights;
    }
    void setLights(LightSet* newlights)
    {
      lights = newlights;
    }

    const RenderParameters& getRenderParameters() const {
      return renderParameters;
    }
    RenderParameters& getRenderParameters() {
      return renderParameters;
    }

    int addBookmark(const std::string& name, const Vector& eye, const Vector& lookat,
                    const Vector& up, double hfov, double vfov);
    int addBookmark(const std::string& name, const BasicCameraData& cam);
    void selectBookmark(unsigned int bookmark);
    const BasicCameraData* nextBookmark();
    const BasicCameraData* currentBookmark();
    BasicCameraData* getBookmark(int i) {
	return &bookmarks[i]->cameradata;
    }

    void readwrite(ArchiveElement*);
    struct Bookmark {
      void readwrite(ArchiveElement* archive);
    private:
      friend class Scene;
      std::string name;
      BasicCameraData cameradata;
    };

    void setAnimationStartTime(double start) {
      animationStartTime = start;
    }
    void setAnimationEndTime(double end) {
      animationEndTime = end;
    }
  private:
    Object* object;
    Background* bg;
    LightSet* lights;
    RenderParameters renderParameters;
    std::vector<Bookmark*> bookmarks;
    unsigned int currentBookmark_id;
    double animationStartTime;
    double animationEndTime;
  };
}

#endif
