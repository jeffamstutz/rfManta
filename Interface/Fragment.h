// Color: Red <url.to.info>

#ifndef Manta_Interface_Fragment_h
#define Manta_Interface_Fragment_h

/*
 *  Fragment.h:
 *
 *  Written by:
 *   Author: Steve Parker
 *   Additional Authors:
 *   School of Computing
 *   University of Utah
 *   Date: 9/2003
 *
 *  Copyright (C) 2003 SCI Group
 */

#include <Core/Color/Color.h>
#include <Core/Util/FancyAssert.h>
#include <Core/Util/Assert.h>

#include <FragmentParameters.h>
#include <Core/Util/Align.h>

// TODO:
//
//   add in:  static const int ConsecutiveY = 0x01;
//   add other constructors to allow other fragment shapes (eg: rectangles, sparse)
//
// CAVEATS:
//

namespace Manta {
  class MANTA_ALIGN(16) Fragment {
  public:

    static const int ConsecutiveX    = 0x01;  // Implies a constant Y:
    static const int ConstantEye     = 0x02;
    static const int UniformSpacing  = 0x04;

    enum FragmentShape {
      LineShape, SquareShape, UnknownShape
    };

    ///////////////////////////////////////////////////////////////////////////
    // Empty Constructor
    Fragment()
      : shape(UnknownShape), flags(0), pixelBegin(0), pixelEnd(0)
      {
        xPixelSize = yPixelSize = 1;
      }
    Fragment(FragmentShape shape)
      : shape(shape), flags(0), pixelBegin(0), pixelEnd(0)
      {
        xPixelSize = yPixelSize = 1;
      }
    Fragment(FragmentShape shape, int flags)
      : shape(shape), flags(flags), pixelBegin(0), pixelEnd(0)
      {
        xPixelSize = yPixelSize = 1;
      }

    // Creates a "Scan-line" fragment.
    Fragment(int xstart, int xend, int y, int eye)
    {
      setConsecutiveX(xstart, xend, y, eye);
    }

    ~Fragment() {}

    void addElement(int x, int y, int eye) {
      ASSERT(pixelEnd < MaxSize);
      pixel[0][pixelEnd] = x;
      pixel[1][pixelEnd] = y;
      whichEye[pixelEnd] = eye;
      pixelEnd++;
    }

    void setElement( int i, int x, int y, int eye ) {
      ASSERTRANGE(i, 0, MaxSize);
      pixel[0][i] = x;
      pixel[1][i] = y;
      whichEye[i] = eye;
    }

    void setConsecutiveX(int xstart, int xend, int y, int eye) {
      ASSERTRANGE(xend-xstart, 0, MaxSize+1);
      int nx = xend-xstart;
      for(int i=0; i< nx;i++){
        pixel[0][i] = i+xstart;
        pixel[1][i] = y;
        whichEye[i] = eye;
      }
      shape = LineShape;
      flags = ConsecutiveX|ConstantEye|UniformSpacing;
      pixelBegin = 0;
      pixelEnd = nx;
    }

    void resetSize() {
      pixelBegin = 0;
      pixelEnd = 0;
    }
    void setSize(int size) {
      pixelBegin = 0;
      pixelEnd = size;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Accessors.
    // Fragment flags
    int getAllFlags() const
    {
      return flags;
    }

    bool getFlag( int flag ) const
    {
      return (flags & flag) == flag;
    }

    void setAllFlags(int new_flags)
    {
      flags = new_flags;
    }
    void setFlag(int flag) {
      flags |= flag;
    }
    void resetFlag(int flag) {
      flags &= ~flag;
    }

    int begin() const {
      return pixelBegin;
    }
    int end() const {
      return pixelEnd;
    }
    void resetAll() {
      pixelBegin = 0;
      pixelEnd = 0;
      flags = 0;
    }

    // You should really only call these functions if size > 0.
    // Otherwise you will set the flag to be true.
    void testSetWhichEye() {
      // This will be true if size is <= 1.
      int test_passed = ConstantEye;
      for(int i = pixelBegin+1; i < pixelEnd; ++i)
        if (whichEye[i-1] != whichEye[i]) {
          test_passed = 0;
          break;
        }
      // This will turn off the flag and then turn it back on if
      // test_passed is "true".
      flags = test_passed | (flags & ~ConstantEye);
    }
    void testSetConsecutiveX() {
      int test_passed = ConsecutiveX;
      for(int i = pixelBegin+1; i < pixelEnd; ++i)
        // Test to see if the y's are the same as well as X being
        // consecutive.
        if (pixel[1][i] != pixel[1][i-1] ||
            pixel[0][i] - pixel[0][i-1] != 1){
          test_passed = 0;
          break;
        }
      flags = test_passed | (flags & ~ConsecutiveX);
    }

    // Pixel
    int getX(int which) const
    {
      return pixel[0][which];
    }
    int getY(int which) const
    {
      return pixel[1][which];
    }
    int getWhichEye(int which) const
    {
      return whichEye[which];
    }

    // Color
    void setColor(int which, const Color& newcolor)
    {
      for(int i=0;i<Color::NumComponents;i++)
        color[i][which] = newcolor[i];
    }
    Color getColor(int which) const
    {
      Color result;
      for(int i=0;i<Color::NumComponents;i++)
        result[i] = color[i][which];
      return result;
    }

    void scaleColors(Color::ComponentType scale)
    {
      for(int j=pixelBegin;j<pixelEnd;j++){
        for(int i = 0 ; i < Color::NumComponents; i++)
          color[i][j] *= scale;
      }
    }
    void scaleColor(int which, Color::ComponentType scale)
    {
      for(int i = 0 ; i < Color::NumComponents; i++)
        color[i][which] *= scale;
    }
    void addColor(int which, const Color& add)
    {
      for(int i = 0 ; i < Color::NumComponents; i++)
        color[i][which] += add[i];
    }

    void setDepth(int which, const Real z)
    {
      depth[which] = z;
    }
    Real getDepth(int which) const
    {
      return depth[which];
    }

    void setPixelSize(int xps, int yps)
    {
      xPixelSize = xps;
      yPixelSize = yps;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Fragment Element Structure.

    // Constant number of Elements so that we do not have to
    // dynamically allocate them.
    static const int MaxSize = FRAGMENT_MAXSIZE;

  public:
    // Fragment data
    MANTA_ALIGN(16) Color::ComponentType color[Manta::Color::NumComponents][MaxSize];
    MANTA_ALIGN(16) Real depth[MaxSize];
    MANTA_ALIGN(16) int pixel[2][MaxSize];
    MANTA_ALIGN(16) int whichEye[MaxSize];

    // Properties of this packet
    FragmentShape shape;
    int flags;

    int xPixelSize, yPixelSize;

    // Range of Elements that are currently being used.
    int pixelBegin, pixelEnd;

  private:
    // Prevent accidental copying of Fragments
    Fragment(const Fragment&);
    Fragment& operator=(const Fragment&);

  };
}

#endif
