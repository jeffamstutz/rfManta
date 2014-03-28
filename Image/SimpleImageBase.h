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

#ifndef Manta_SimpleImageBase_h
#define Manta_SimpleImageBase_h

#include <Interface/Image.h>

namespace Manta {
  class SimpleImageBase : public Image {
  public:
    SimpleImageBase(bool stereo, int xres, int yres);
    virtual ~SimpleImageBase();
    virtual bool isValid() const;
    virtual void setValid(bool to);
    virtual void getResolution(bool& stereo, int& xres, int& yres) const;
    virtual void* getRawData(int which_eye) const = 0;
    virtual size_t pixelSize() const = 0;

    int getRowLength() const {
      return xpad;
    }

  protected:
    bool valid;
    bool stereo;
    int xres, yres;
    int xpad;

  private:
    SimpleImageBase(const Image&);
    SimpleImageBase& operator=(const SimpleImageBase&);

  };
}

#endif
