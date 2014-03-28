
#ifndef Manta_Engine_NullImageTraverser_h
#define Manta_Engine_NullImageTraverser_h

#include <Interface/ImageTraverser.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class NullImageTraverser : public ImageTraverser {
  public:
    NullImageTraverser(const vector<string>& args);
    virtual ~NullImageTraverser();
    virtual void setupBegin(SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderImage(const RenderContext& context, Image* image);

    static ImageTraverser* create(const vector<string>& args);
  private:
    NullImageTraverser(const NullImageTraverser&);
    NullImageTraverser& operator=(const NullImageTraverser&);
  };
}

#endif
