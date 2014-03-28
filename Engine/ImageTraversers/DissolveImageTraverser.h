/*
  DissolveImageTraverser

  A frameless rendering image traverser.  It attemps to use a pseudo
  random number generator with variable period to eventually fill the
  screen.  The pixel locations are not precomputed but rather computed
  when needed.  The technique is based on the article "A Digital
  "Dissolve" Effect" by Mike Morton found in Graphics Gems I
  (pg. 221-232).

  There are some interesting things about this method.  First for N
  processors, ever Nth pixel is computed by each thread.  Once the
  period of the random number generator has been reached it starts
  over (but not necessarily from the same point).  The net effect is
  that a single processor can do a different set of pixels the next
  full frame.  I'm not entirely sure what the implications are for
  over drawing of pixels, but it looks OK for now.
  
  Author: James Bigler
  Date: Aug. 2005

*/

#ifndef Manta_Engine_DissolveImageTraverser_h
#define Manta_Engine_DissolveImageTraverser_h

#include <Interface/ImageTraverser.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class DissolveImageTraverser : public ImageTraverser {
  public:
    DissolveImageTraverser(const vector<string>& args);
    DissolveImageTraverser(const unsigned iters_per_frame_) :
      iters_per_frame( iters_per_frame_ ) { }
    
    virtual ~DissolveImageTraverser();
    virtual void setupBegin(SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderImage(const RenderContext& context, Image* image);

    static ImageTraverser* create(const vector<string>& args);

    // Accessors.
    unsigned getItersPerFrame() const { return iters_per_frame; };
    
  private:
    DissolveImageTraverser(const DissolveImageTraverser&);
    DissolveImageTraverser& operator=(const DissolveImageTraverser&);

    // Each thread stores where it left off for each channel.
    struct ThreadContext {
      // next_pixel.size() == numChannels.  We'll store both x and y
      // in this single variable, where x uses the higher order bits
      // and y uses the lower order.
      vector<unsigned int> next_pixel;
    };

    vector<ThreadContext> per_thread_data;

    // This is the information needed to produce the next random
    // number.  Since the masks are based on the size of the channel,
    // each channel must maintain its own set of masks to allow for
    // varying sizes of channels.
    struct ChannelContext {
      unsigned int mask;
      unsigned int num_y_bits;
      unsigned int y_bit_mask;
    };
    // channel_masks.size() == numChannels
    vector<ChannelContext> channel_data;
    
    // This is how many pixels to render on a call to renderImage.
    unsigned int pixels_per_pass;
    // Since this is a "framed" version of a frameless method, we
    // attempt to only do a subset of the work required for a frame.
    // This tells us how many frames are needed to fill a frame.
    unsigned int iters_per_frame;

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
