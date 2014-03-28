// Color: Yellow, 9/4/03, see doc/codingstandard.txt for more information

#ifndef Manta_Interface_PixelSampler_h
#define Manta_Interface_PixelSampler_h

/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2003-2005
  Scientific Computing and Imaging Institute, University of Utah

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
 *   Copyright (C) 2003 SCI Institute
 *
 *   Reviewed: 9/4/03 - Manta Meeting
 */

namespace Manta {

  class SetupContext;
  class RenderContext;
  class Fragment;

  /**
   *  This is the interface to a PixelSampler component. A PixelSampler
   *  computes colors for a set of pixels, typically provided from the
   *  ImageTraverser component.
   */

  class PixelSampler {
  public:
    virtual ~PixelSampler();

    /**
     * Called once per major context change, see doc/setup.txt for
     * more information on the setup sequence.
     */
    virtual void setupBegin(const SetupContext&, int numChannels) = 0;

    /**
     * Called once per display chhanel (e.g. once each for left and
     * right screens).  See doc/setup.txt for more information on the
     * setup sequence.
     */
    virtual void setupDisplayChannel(SetupContext&) = 0;

    /**
     * Called once at the beginning of the frame by *all* threads.  Be
     * careful inserting code into this call because it can have a
     * profound impact on scalability.  In particular, calls that
     * allocate memory are better placed in setupBegin or
     * setupDisplayChannel.  See doc/setup.txt for more information on
     * the setup sequence.
     */
    virtual void setupFrame(const RenderContext& context) = 0;

    /**
     * Called during each frame to render a set of pixels.  The
     * fragment object contains both the input and the output of the
     * rendering.
     *
     *  Fragment inputs:
     *    int x, y;      // Pixel position in image space
     *    int which_eye; // monocular: always 0, stereo: 0=left, 1=right
     *
     *  Fragment outputs:
     *    Color color;   // Final result
     */
    virtual void renderFragment(const RenderContext& context,
				Fragment& fragment) = 0;

  protected:
    PixelSampler();

  private:
    PixelSampler(const PixelSampler&);
    PixelSampler& operator=(const PixelSampler&);
  };

}

#endif
