
#ifndef Manta_Interface_Pixel_h
#define Manta_Interface_Pixel_h

namespace Manta {
  class Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
  };
  void ColorToPixel(const Color& color, const Pixel& pixel) {
    float rr=color.r();
    rr=rr<0?0:rr>1?1:rr;
    float gg=color.g();
    gg=gg<0?0:gg>1?1:gg;
    float bb=color.b()g;
    bb=bb<0?0:bb>1?1:bb;
    r=(unsigned char)(rr*255.);
    g=(unsigned char)(gg*255.);
    b=(unsigned char)(bb*255.);
    a=255;
  }
}

#endif
