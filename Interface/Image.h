
#ifndef Manta_Interface_Image_h
#define Manta_Interface_Image_h

#include <Interface/Fragment.h>

namespace Manta {
  class Image {
  public:
    Image();
    virtual ~Image();
    virtual void getResolution(bool& stereo, int& xres, int& yres) const = 0;
    virtual bool isValid() const = 0;
    virtual void setValid(bool to) = 0;
    virtual void set(const Fragment&) = 0;
    virtual void get(Fragment&) const = 0;
  private:
    Image(const Image&);
    Image& operator=(const Image&);
  };
}

#endif
