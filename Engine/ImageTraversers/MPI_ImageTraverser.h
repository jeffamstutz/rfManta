
#ifndef Manta_Engine_MPI_ImageTraverser_h
#define Manta_Engine_MPI_ImageTraverser_h

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
#include <Core/Thread/ConditionVariable.h>
#include <Core/Thread/Barrier.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>

namespace Manta {
  using namespace std;
  class MPI_ImageTraverser : public ImageTraverser {
  public:
    MPI_ImageTraverser(const vector<string>& args);
    MPI_ImageTraverser( const int xtilesize_, const int ytilesize );

    virtual ~MPI_ImageTraverser();
    virtual void setupBegin(SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderImage(const RenderContext& context, Image* image);

    static ImageTraverser* create(const vector<string>& args);

    // Accessors.
    int getXTileSize() const { return xtilesize; }
    int getYTileSize() const { return ytilesize; }

  private:
    MPI_ImageTraverser(const MPI_ImageTraverser&);
    MPI_ImageTraverser& operator=(const MPI_ImageTraverser&);

    Fragment::FragmentShape shape;

    int xtilesize;
    int ytilesize;

    int xtiles;
    int ytiles;

    class Fragment_Light { // lightweight fragment
    public:
      Fragment_Light() : size(0) { }
      void set(const Fragment &frag, const Image* image) {
        size = 0;
        for (int i = frag.begin(); i < frag.end(); ++i) {
          const int x = frag.getX(i);
          const int y = frag.getY(i);
          pixelID[size][0] = x;
          pixelID[size][1] = y;
          pixels[size] = static_cast<const SimpleImage<RGB8Pixel>*> // XXX: Hardcoded to require this type of Image!
            (image)->get(x, y, 0);
          ++size;
        }
      }
      RGB8Pixel pixels[Fragment::MaxSize];
      unsigned short pixelID[Fragment::MaxSize][2];
      unsigned char size; // Note using uchar places a size limit on fragments
                          // being 256 rays or less.
    };

    class FragmentBuffer {
    private:
      int currFrag;
      Fragment fragments[13];
    public:
      Fragment_Light frag_light[13];
      inline static int maxFrags() { return 13; }

      Fragment *frag;
      FragmentBuffer() : currFrag(0) {
        frag = &fragments[currFrag];

        for (int j=0; j < maxFrags(); ++j)
          for (int i=0; i < Fragment::MaxSize; ++i)
            fragments[j].whichEye[i] = 0;
      }

      void reset() {
        currFrag = 0;
        frag = &fragments[currFrag];
      }

      // Up to user to make sure this doesn't overflow!
      void nextFrag() {
        ++currFrag;
        frag = &fragments[currFrag];
      }

      inline int numFrags() { return currFrag; }

      Fragment* getFragmentList() {
        return fragments;
      }

      void initialize(Fragment::FragmentShape shape, int flags) {
        for (int i=0; i < maxFrags(); ++i) {
          fragments[i].shape = shape;
          fragments[i].flags = flags;
        }
      }

      void fillRemainingWithNullFrags() {
        for (int i=currFrag; i < maxFrags(); ++i)
          fragments[i].setSize(0);
        currFrag = maxFrags()-1;
      }
    };
    FragmentBuffer* fragBuffer;

    struct FragLightQueue {
      // Make the queue so huge that it's likely to always be big enough so
      // that if we wrap, it's unlikely that we'll overwrite data that we
      // haven't yet consumed.  Note that if that were to happen, then we would
      // get missing pixels in the image.
      const static int MAX_SIZE = 2048*2048/Fragment::MaxSize;
      Fragment_Light* data[MAX_SIZE];//[FragmentBuffer::maxFrags()];
      int nextFree; // for producer
      int currAvailable; // for consumer
      Mutex mutex;
      bool done;

      FragLightQueue() : nextFree(0), currAvailable(0), mutex(""), done(false) {
        for (int i=0; i < MAX_SIZE; ++i)
          data[i] = new Fragment_Light[FragmentBuffer::maxFrags()];
      }

      ~FragLightQueue() {
        for (int i=0; i < MAX_SIZE; ++i)
          delete[] data[i];
        delete[] data;
      }

      int getNextAvailable() {
        int which = -1;
        mutex.lock();
        if (currAvailable != nextFree) {
          which = currAvailable++;
          if (currAvailable >= MAX_SIZE)
            currAvailable = 0;
        }
        mutex.unlock();

        return which;
      }

      void producedOne() {
        nextFree = (nextFree+1) % MAX_SIZE;
      }

      void reset() {
        nextFree = currAvailable = 0;
        done = false;
      }

    };

    FragLightQueue* fraglQueue;

    Barrier barrier;

    int LB_master; // node that is the master for giving out work.

    void writeMPIFragment(const RenderContext& context, Image* image);
    void exit(const RenderContext& context);
  };
}

#endif
