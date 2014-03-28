
#ifndef Manta_Interface_ImageDisplay_h
#define Manta_Interface_ImageDisplay_h

namespace Manta {
  class SetupContext;
  class DisplayContext;
  class Image;
  class ImageDisplay {
  public:
    virtual ~ImageDisplay();
    virtual void setupDisplayChannel(SetupContext&) = 0;
    virtual void displayImage(const DisplayContext& context,
			      const Image* image) = 0;
  protected:
    ImageDisplay();
  private:
    ImageDisplay(const ImageDisplay&);
    ImageDisplay& operator=(const ImageDisplay&);
  };
}

#endif

