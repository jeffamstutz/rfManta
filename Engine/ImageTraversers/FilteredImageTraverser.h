
#ifndef Manta_Engine_FilteredImageTraverser_h
#define Manta_Engine_FilteredImageTraverser_h

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
#include <Core/Util/ThreadStorage.h>
#include <string>
#include <vector>

namespace Manta {
  
  using namespace std;

  class Fragment;
  
  class FilteredImageTraverser : public ImageTraverser {
  public:
    FilteredImageTraverser( int xtilesize_, int ytilesize_ ) :
      xtilesize( xtilesize_ ), ytilesize( ytilesize_ ) { };

    FilteredImageTraverser(const vector<string>& args);
    
    ///////////////////////////////////////////////////////////////////////////
    // ImageTraverser interface implemented by this class.
    virtual ~FilteredImageTraverser();
    virtual void setupBegin(SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderImage(const RenderContext& context, Image* image);

    static ImageTraverser* create(const vector<string>& args);
    
  private:
    FilteredImageTraverser(const FilteredImageTraverser&);
    FilteredImageTraverser& operator=(const FilteredImageTraverser&);

    // Tile size.
    int xtilesize;
    int ytilesize;

    // Token to access per thread storage.
    ThreadStorage::Token storage_token;
    
    // Buffer of persistant fragments.
    int total_allocated;
    Fragment *fragment_buffer;
    
    int xtiles;
    int ytiles;
  };
}

#endif
