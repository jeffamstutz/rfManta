/*
  DissolveTiledImageTraverser

  A frameless rendering image traverser.  It attemps to use a pseudo
  random number generator with variable period to eventually fill the
  screen.  The screen is filled tile by tile.  The size of the tiles
  can be anything from 1x1 to 32x2 (default for TiledImageTraverser)
  or anything else you can imagine.  The tile orders are not
  precomputed but rather computed when needed.  The technique is based
  on the article "A Digital "Dissolve" Effect" by Mike Morton found in
  Graphics Gems I (pg. 221-232).

  Every thread computes every Nth tile (where N is the number of
  threads).
  
  Author: James Bigler
  Date: Aug. 2005

*/

#ifndef Manta_Engine_DissolveTiledImageTraverser_h
#define Manta_Engine_DissolveTiledImageTraverser_h

#include <Interface/ImageTraverser.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class DissolveTiledImageTraverser : public ImageTraverser {
  public:
    DissolveTiledImageTraverser(const vector<string>& args);
    virtual ~DissolveTiledImageTraverser();
    virtual void setupBegin(SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderImage(const RenderContext& context, Image* image);

    static ImageTraverser* create(const vector<string>& args);
  private:
    DissolveTiledImageTraverser(const DissolveTiledImageTraverser&);
    DissolveTiledImageTraverser& operator=(const DissolveTiledImageTraverser&);

    int xtilesize;
    int ytilesize;

    // Since this is a "framed" version of a frameless method, we
    // attempt to only do a subset of the work required for a frame.
    // This tells us how many frames are needed to fill a frame.
    unsigned int iters_per_frame;

    struct ChannelContext {
      // This is the mask for the random number generator.
      unsigned int rng_mask;
      // Which tile to start on next (since this is a framed
      // implementation).  Remember this number is in the range of
      // [1,numTiles], so you should subtract one when you want to use
      // it.  The reason is that the computeNextSample doesn't work
      // for 0 (and you want that value).
      unsigned int next_tile;
      // This tells us how many tiles to work on for a given thread/context
      unsigned int numTiles;
      // This is how many tiles to render on a call to renderImage.  It
      // is based on the number of iters_per_pass, threads, and pixels
      // contained in the channels.
      unsigned int tiles_per_pass;
    };
    
    // Each thread stores where it left off for each channel.
    struct ThreadContext {
      vector<ChannelContext> channel_data; // size = numChannels
    };

    vector<ThreadContext> per_thread_data; // size = numProcs

    inline void computeNextSample(unsigned int& val, unsigned int mask) {
      if (val & 1)
        val = (val >> 1) ^ mask;
      else
        val = val >> 1;
    }

    // Returns the number of binary digits needed to represent val.
    unsigned int numBinaryDigits(unsigned int val) {
      unsigned int num_digits = 0;
      while (val) {
        num_digits++;
        val>>=1;
      }
      return num_digits;
    }

    
  };
}

#endif
