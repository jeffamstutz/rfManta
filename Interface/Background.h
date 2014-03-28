
#ifndef Manta_Interface_Background_h
#define Manta_Interface_Background_h

namespace Manta {
  class PreprocessContext;
  class RayPacket;
  class RenderContext;
  class Background {
  public:
    Background();
    virtual ~Background();

    virtual void preprocess(const PreprocessContext& context) {}
    virtual void shade(const RenderContext& context, RayPacket& rays) const = 0;
  private:
    Background(const Background&);
    Background& operator=(const Background&);
  };
}

#endif
