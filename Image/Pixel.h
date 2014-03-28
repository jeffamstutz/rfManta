
#ifndef Manta_Image_Pixel_h
#define Manta_Image_Pixel_h

#include <MantaTypes.h>
#include <Core/Color/RGBColor.h>

namespace Manta {

  /////////////////////////////////////////////////////////////////
  // RGBA8Pixel

  class RGBA8Pixel {
  public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
  };

  inline void convertToPixel(RGBA8Pixel& p, const RGBColor& c) {
    float r = c.r();
    r = r<0.f?0.f:r;
    r = r>1.f?1.f:r;
    p.r = (unsigned char)(r*255.f);
    float g = c.g();
    g = g<0.f?0.f:g;
    g = g>1.f?1.f:g;
    p.g = (unsigned char)(g*255.f);
    float b = c.b();
    b = b<0.f?0.f:b;
    b = b>1.f?1.f:b;
    p.b = (unsigned char)(b*255.f);
    p.a = 255;
  }

  inline void convertToRGBColor(RGBColor& c, const RGBA8Pixel& p) {
    ColorComponent norm = (ColorComponent)1/(ColorComponent)255;
    c.setR(p.r*norm);
    c.setG(p.g*norm);
    c.setB(p.b*norm);
  }

  /////////////////////////////////////////////////////////////////
  // ABGR8Pixel

  class ABGR8Pixel {
  public:
    unsigned char a;
    unsigned char b;
    unsigned char g;
    unsigned char r;
  };

  inline void convertToPixel(ABGR8Pixel& p, const RGBColor& c) {
    p.a = 255;
    float b = c.b();
    b = b<0.f?0.f:b;
    b = b>1.f?1.f:b;
    p.b = (unsigned char)(b*255.f);
    float g = c.g();
    g = g<0.f?0.f:g;
    g = g>1.f?1.f:g;
    p.g = (unsigned char)(g*255.f);
    float r = c.r();
    r = r<0.f?0.f:r;
    r = r>1.f?1.f:r;
    p.r = (unsigned char)(r*255.f);
  }

  inline void convertToRGBColor(RGBColor& c, const ABGR8Pixel& p) {
    ColorComponent norm = (ColorComponent)1/(ColorComponent)255;
    c.setR(p.r*norm);
    c.setG(p.g*norm);
    c.setB(p.b*norm);
  }

  /////////////////////////////////////////////////////////////////
  // ARGB8Pixel

  class ARGB8Pixel {
  public:
    unsigned char a;
    unsigned char r;
    unsigned char g;
    unsigned char b;
  };

  inline void convertToPixel(ARGB8Pixel& p, const RGBColor& c) {
    p.a = 255;
    float r = c.r();
    r = r<0.f?0.f:r;
    r = r>1.f?1.f:r;
    p.r = (unsigned char)(r*255.f);
    float g = c.g();
    g = g<0.f?0.f:g;
    g = g>1.f?1.f:g;
    p.g = (unsigned char)(g*255.f);
    float b = c.b();
    b = b<0.f?0.f:b;
    b = b>1.f?1.f:b;
    p.b = (unsigned char)(b*255.f);
  }

  inline void convertToRGBColor(RGBColor& c, const ARGB8Pixel& p) {
    ColorComponent norm = (ColorComponent)1/(ColorComponent)255;
    c.setR(p.r*norm);
    c.setG(p.g*norm);
    c.setB(p.b*norm);
  }

  /////////////////////////////////////////////////////////////////
  // BGRA8Pixel

  class BGRA8Pixel {
  public:
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
  };

  inline void convertToPixel(BGRA8Pixel& p, const RGBColor& c) {
    float b = c.b();
    b = b<0.f?0.f:b;
    b = b>1.f?1.f:b;
    p.b = (unsigned char)(b*255.f);
    float g = c.g();
    g = g<0.f?0.f:g;
    g = g>1.f?1.f:g;
    p.g = (unsigned char)(g*255.f);
    float r = c.r();
    r = r<0.f?0.f:r;
    r = r>1.f?1.f:r;
    p.r = (unsigned char)(r*255.f);
    p.a = 255;
  }

  inline void convertToRGBColor(RGBColor& c, const BGRA8Pixel& p) {
    ColorComponent norm = (ColorComponent)1/(ColorComponent)255;
    c.setR(p.r*norm);
    c.setG(p.g*norm);
    c.setB(p.b*norm);
  }

  /////////////////////////////////////////////////////////////////
  // RGB8Pixel

  class RGB8Pixel {
  public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
  };

  inline void convertToPixel(RGB8Pixel& p, const RGBColor& c) {
    float r = c.r();
    r = r<0.f?0.f:r;
    r = r>1.f?1.f:r;
    p.r = (unsigned char)(r*255.f);
    float g = c.g();
    g = g<0.f?0.f:g;
    g = g>1.f?1.f:g;
    p.g = (unsigned char)(g*255.f);
    float b = c.b();
    b = b<0.f?0.f:b;
    b = b>1.f?1.f:b;
    p.b = (unsigned char)(b*255.f);
  }

  inline void convertToRGBColor(RGBColor& c, const RGB8Pixel& p) {
    ColorComponent norm = (ColorComponent)1/255;
    c.setR(p.r*norm);
    c.setG(p.g*norm);
    c.setB(p.b*norm);
  }

  /////////////////////////////////////////////////////////////////
  // RGBfloatPixel

  class RGBfloatPixel {
  public:
    float r;
    float g;
    float b;
  };

  inline void convertToPixel(RGBfloatPixel& p, const RGBColor& c){
    p.r = c.r();
    p.g = c.g();
    p.b = c.b();
  }

  inline void convertToRGBColor(RGBColor& c, const RGBfloatPixel& p) {
    c.setR(p.r);
    c.setG(p.g);
    c.setB(p.b);
  }

  /////////////////////////////////////////////////////////////////
  // RGBAfloatPixel

  class RGBAfloatPixel {
  public:
    float r;
    float g;
    float b;
    float a;
  };

  inline void convertToPixel(RGBAfloatPixel& p, const RGBColor& c){
    p.r = c.r();
    p.g = c.g();
    p.b = c.b();
    p.a = 1;
  }

  inline void convertToRGBColor(RGBColor& c, const RGBAfloatPixel& p) {
    c.setR(p.r);
    c.setG(p.g);
    c.setB(p.b);
  }

  class BGRAfloatPixel {
  public:
    float b;
    float g;
    float r;
    float a;
  };

  inline void convertToPixel(BGRAfloatPixel& p, const RGBColor& c){
    p.r = c.r();
    p.g = c.g();
    p.b = c.b();
    p.a = 1;
  }

  inline void convertToRGBColor(RGBColor& c, const BGRAfloatPixel& p) {
    c.setR(p.r);
    c.setG(p.g);
    c.setB(p.b);
  }

  class RGBZfloatPixel {
  public:
    float r;
    float g;
    float b;
    float z;
  };

  inline void convertToPixel(RGBZfloatPixel& p, const RGBColor& c, Real z){
    p.r = c.r();
    p.g = c.g();
    p.b = c.b();
    p.z = z;
  }

  inline void convertToRGBColor(RGBColor& c, const RGBZfloatPixel& p) {
    c.setR(p.r);
    c.setG(p.g);
    c.setB(p.b);
  }

  class RGBA8ZfloatPixel {
  public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    float z;
  };

  inline void convertToPixel(RGBA8ZfloatPixel& p, const RGBColor& c, Real z) {
    float r = c.r();
    r = r<0.f?0.f:r;
    r = r>1.f?1.f:r;
    p.r = (unsigned char)(r*255.f);
    float g = c.g();
    g = g<0.f?0.f:g;
    g = g>1.f?1.f:g;
    p.g = (unsigned char)(g*255.f);
    float b = c.b();
    b = b<0.f?0.f:b;
    b = b>1.f?1.f:b;
    p.b = (unsigned char)(b*255.f);
    p.a = 255;

    p.z = z;
    //printf("here\n");
  }

  inline void convertToRGBColor(RGBColor& c, const RGBA8ZfloatPixel& p) {
    ColorComponent norm = (ColorComponent)1/(ColorComponent)255;
    c.setR(p.r*norm);
    c.setG(p.g*norm);
    c.setB(p.b*norm);
  }
}

#endif
