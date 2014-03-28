
#ifndef Manta_Image_NullImage_h
#define Manta_Image_NullImage_h

#include <Interface/Image.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class NullImage : public Image {
  public:
    NullImage(bool stereo, int xres, int yres);
    virtual ~NullImage();
    virtual void set(const Fragment&);
    virtual void get(Fragment&) const;
    virtual bool isValid() const;
    virtual void setValid(bool to);
    virtual void getResolution(bool& stereo, int& xres, int& yres) const;
    static Image* create(const vector<string>& args,
			 bool stereo, int xres, int yres);
  private:
    NullImage(const Image&);
    NullImage& operator=(const NullImage&);

    bool valid;
    bool stereo;
    int xres, yres;
  };
}

#endif
