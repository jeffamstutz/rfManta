/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef Manta_SimpleImage_h
#define Manta_SimpleImage_h

#include <Image/SimpleImageBase.h>
#include <Core/Color/Color.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Util/AlignedAllocator.h>
#include <Image/Pixel.h>
#include <Parameters.h>
#include <string>
#include <vector>

namespace Manta {
  template<class Pixel> class SimpleImage : public SimpleImageBase {
  public:
    SimpleImage(bool stereo, int xres, int yres);
    virtual ~SimpleImage();
    static Image* create(const std::vector<std::string>& args, bool stereo,
                         int xres, int yres);
    virtual void set(const Fragment& fragment);
    virtual void get(Fragment& fragment) const;
    virtual void set(const Pixel& pixel, int x, int y, int eye);
    virtual Pixel get(int x, int y, int eye) const;

    Pixel* getRawPixels(int which_eye) const;
    void* getRawData(int which_eye) const;

    virtual size_t pixelSize() const { return sizeof(Pixel); }
  private:
    SimpleImage(const Image&);
    SimpleImage& operator=(const SimpleImage&);

    // This splats
    void splat(const Fragment& fragment);

    Pixel** eyeStart[2];
  };

  template<class Pixel>
  SimpleImage<Pixel>::~SimpleImage()
  {
    deallocateAligned(eyeStart[0][0]);
    delete[] eyeStart[0];
  }

  template<class Pixel>
  Image* SimpleImage<Pixel>::create(const std::vector<std::string>& args,
                                    bool stereo, int xres, int yres)
  {
    if(args.size() != 0)
      throw IllegalValue<std::string>
        ("Illegal argument to SimpleImage create", args[0]);
    return new SimpleImage<Pixel>(stereo, xres, yres);
  }

  template<class Pixel>
  SimpleImage<Pixel>::SimpleImage(bool stereo, int xres, int yres)
    : SimpleImageBase(stereo, xres, yres)
  {
    valid = false;
    int numEyes = stereo?2:1;
    int pad = IMAGE_ROW_BYTES/sizeof(Pixel);
    if(pad < 1)
      pad = 1;
    xpad = (xres+pad-1)&~(pad-1);
    unsigned long totalSize = (xpad * yres * numEyes * sizeof(Pixel));
    eyeStart[0] = new Pixel*[stereo?2*yres:yres];
    eyeStart[0][0] = (Pixel*)allocateAligned(totalSize, MAXCACHELINESIZE);
    for(int i=1;i<yres;i++)
      eyeStart[0][i] = eyeStart[0][i-1]+xpad;
    if(stereo){
      eyeStart[1] = eyeStart[0]+yres;
      eyeStart[1][0] = eyeStart[0][0]+xpad*yres;
      for(int i=1;i<yres;i++)
        eyeStart[1][i] = eyeStart[1][i-1]+xpad;
    } else {
      eyeStart[1] = 0;
    }
  }

  template<class Pixel>
  void SimpleImage<Pixel>::splat(const Fragment& fragment)
  {
    // NOTE(boulos): This branch tries to copy fragments where a
    // single sample splats onto several pixels
    for(int i=fragment.begin();i<fragment.end();i++){
      Pixel pix;
      convertToPixel(pix, fragment.getColor(i).convertRGB());

      // start splatting
      int y_index = fragment.getY(i);
      int last_x = std::min(xres - fragment.getX(i), fragment.xPixelSize);
      for(int y=0;y<fragment.yPixelSize;y++){

        if (y_index >= yres) break;

        Pixel* row = eyeStart[fragment.getWhichEye(i)][y_index];
        for(int x=0;x<last_x;x++)
          row[fragment.getX(i) + x] = pix;

        y_index++;
      }
    }
  }
  template<>
  void SimpleImage<RGBZfloatPixel>::splat(const Fragment& fragment);
  template<>
  void SimpleImage<RGBA8ZfloatPixel>::splat(const Fragment& fragment);

  template<class Pixel>
  void SimpleImage<Pixel>::set(const Fragment& fragment)
  {
    if(fragment.xPixelSize != 1 || fragment.yPixelSize != 1){
      splat(fragment);
    } else if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)){
      int b = fragment.begin();
      Pixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+fragment.getX(b);
      for(int i=fragment.begin(); i< fragment.end();i++)
        convertToPixel(*pix++, fragment.getColor(i).convertRGB());
    } else {
      for(int i=fragment.begin();i<fragment.end();i++){
        convertToPixel(eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)], fragment.getColor(i).convertRGB());
      }
    }
  }

  template<>
    void SimpleImage<ARGB8Pixel>::set(const Fragment& fragment);

  template<>
    void SimpleImage<BGRA8Pixel>::set(const Fragment& fragment);

  template<>
    void SimpleImage<RGBA8Pixel>::set(const Fragment& fragment);

  template<>
    void SimpleImage<RGBAfloatPixel>::set(const Fragment& fragment);

  template<>
    void SimpleImage<RGBZfloatPixel>::set(const Fragment& fragment);
  template<>
    void SimpleImage<RGBA8ZfloatPixel>::set(const Fragment& fragment);

  template<class Pixel>
  void SimpleImage<Pixel>::get(Fragment& fragment) const
  {
    if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)){
      int b = fragment.begin();
      Pixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+fragment.getX(b);
      for(int i=fragment.begin(); i < fragment.end();i++) {
        RGBColor color;
        convertToRGBColor(color, *pix++);
        // Convert from RGBColor to Color
        fragment.setColor(i, Color(color));
      }
    } else {
      for(int i=fragment.begin(); i < fragment.end(); i++){
        RGBColor color;
        convertToRGBColor(color, eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)]);
        // Convert from RGBColor to Color
        fragment.setColor(i, Color(color));
      }
    }
  }

  template<class Pixel>
  void SimpleImage<Pixel>::set(const Pixel& pixel, int x, int y, int eye)
  {
    *(eyeStart[eye][y]+x) = pixel;
  }

  template<class Pixel>
  Pixel SimpleImage<Pixel>::get(int x, int y, int eye) const
  {
    return *(eyeStart[eye][y]+x);
  }

  template<class Pixel>
    Pixel* SimpleImage<Pixel>::getRawPixels(int which_eye) const
  {
    return eyeStart[which_eye][0];
  }

  template<class Pixel>
    void* SimpleImage<Pixel>::getRawData(int which_eye) const
  {
    return eyeStart[which_eye][0];
  }

}

#endif
