
#ifndef Manta_Engine_DeadlineImageTraverser_h
#define Manta_Engine_DeadlineImageTraverser_h

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

#include <Interface/IdleMode.h>
#include <Interface/ImageTraverser.h>
#include <Interface/Fragment.h>
#include <Core/Thread/Mutex.h>
#include <Core/Util/ApproximatePriorityQueue.h>
#include <Core/Util/MemoryPool.h>

#include <string>
#include <vector>
#include <MantaSSE.h>

namespace Manta {
  using namespace std;
  class SampleGenerator;
  class SingleSampler;
  class JitterSampler;

  class DeadlineImageTraverser : public ImageTraverser, public IdleMode {
  public:
    DeadlineImageTraverser(const vector<string>& args);
    virtual ~DeadlineImageTraverser();

    virtual void changeIdleMode(const SetupContext&,
                                bool changed,
                                bool firstFrame,
                                bool& pipelineNeedsSetup);

    virtual void setupBegin(SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderImage(const RenderContext& context, Image* image);

    static ImageTraverser* create(const vector<string>& args);
    static const int kNumJitterLevels = 7; // This provides 4, 16, ..., 2^(kNumJitterLevels*2) spp

    void setRenderRegion(int xstart, int ystart, int xend, int yend) {
      render_region.xstart = xstart;
      render_region.ystart = ystart;
      render_region.xend = xend;
      render_region.yend = yend;
      render_region.xcenter = (xend-xstart)/2 + xstart;
      render_region.ycenter = (yend-ystart)/2 + ystart;
    }
    void useRenderRegion(bool use) { use_render_region = use; }

  private:
    DeadlineImageTraverser(const DeadlineImageTraverser&);
    DeadlineImageTraverser& operator=(const DeadlineImageTraverser&);

    SingleSampler* singleSampler;
    JitterSampler** jitterSamplers; // An array of different jittered samplers

    SampleGenerator** sampleGenerators; // An array of sample generators for each refinement level

    int xpacketsize;
    int ypacketsize;

    int xcoarsepixelsize;
    int ycoarsepixelsize;
    int xrefinementRatio;
    int yrefinementRatio;
    double frameTime;
    double frameEnd;

    struct Tile {
      float priority;
      int xstart;
      int ystart;
      int xend;
      int yend;
      float xmag;
      float ymag;
      int padding; // unfortunately we can't make these 128 bytes wide
    };
    Mutex qlock;
    ApproximatePriorityQueue<Tile*, float> queue;
    static const int kMaxThreads = 32;
    MemoryPool<Tile> tile_pools[kMaxThreads];
    bool reset_every_frame;
    bool finished_coarse;
    bool ShowTimeSupersample;
    bool converged;
    double StartFrameTime;
    bool do_dart_benchmark;
    int max_spp; //max samples per pixel
    enum PriorityScheme {
      FIFO, LuminanceVariance, Contrast, FIFO_to_LumVar, Center
    };
    PriorityScheme priority;
    float FIFO_cutoff;

    struct Region
    {
      int xstart;
      int ystart;
      int xend;
      int yend;
      int xcenter;
      int ycenter;
    };
    bool use_render_region;
    Region render_region;

    void insertIntoQueue(Tile* tile, size_t thread_id);
    void insertIntoQueue(vector<Tile*>& tiles, size_t thread_id);
    Tile* popFromQueue(size_t thread_id);

    float computeLuminancePriority(Tile* tile, const Fragment& frag) const;
    float computeContrastPriority(Tile* tile, const Fragment& frag) const;
    float computeFIFOPriority(Tile* tile, const Fragment& frag) const;
    float computeCenterPriority(Tile* tile, const Fragment& frag) const;
    void computePriority(Tile* tile, const Fragment& frag) const;
  };
}

#endif
