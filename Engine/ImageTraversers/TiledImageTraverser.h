
#ifndef Manta_Engine_TiledImageTraverser_h
#define Manta_Engine_TiledImageTraverser_h

/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
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

#include <Interface/ImageTraverser.h>
#include <Interface/Fragment.h>
#include <string>
#include <vector>
#include <MantaSSE.h>

namespace Manta {
  using namespace std;
  class TiledImageTraverser : public ImageTraverser {
  public:
    TiledImageTraverser(const vector<string>& args);
    TiledImageTraverser( const int xtilesize_, const int ytilesize );
    
    virtual ~TiledImageTraverser();
    virtual void setupBegin(SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderImage(const RenderContext& context, Image* image);

    static ImageTraverser* create(const vector<string>& args);

    // Accessors.
    int getXTileSize() const { return xtilesize; }
    int getYTileSize() const { return ytilesize; }
    
  private:
    TiledImageTraverser(const TiledImageTraverser&);
    TiledImageTraverser& operator=(const TiledImageTraverser&);

      Fragment::FragmentShape shape;

    int xtilesize;
    int ytilesize;

    int xtiles;
    int ytiles;
  };
}

#endif
