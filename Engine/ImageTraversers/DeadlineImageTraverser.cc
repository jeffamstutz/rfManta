// TODO
// Profile/optimize
// Better criteria
// Spatial criteria (1 iteration of lcong)
// Error criteria
// Should do pixel centers - directly to renderer, or alternate interface??
// Avoid false sharing in tiles - pad or make per-processor tile arrays?

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

#include <Engine/ImageTraversers/DeadlineImageTraverser.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/Expon.h>
#include <Core/Thread/Mutex.h>
#include <Core/Util/Args.h>
#include <Core/Util/NotFinished.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/Image.h>
#include <Interface/LoadBalancer.h>
#include <Interface/MantaInterface.h>
#include <Interface/PixelSampler.h>
#include <Engine/PixelSamplers/SingleSampler.h>
#include <Engine/PixelSamplers/JitterSampler.h>
#include <Engine/SampleGenerators/ConstantSampleGenerator.h>
#include <Engine/SampleGenerators/Stratified2D.h>
#include <Engine/SampleGenerators/UniformRandomGenerator.h>
#include <Interface/RayPacket.h> // for maxsize
#include <Core/Util/CPUTime.h>
#include <MantaSSE.h>
#include <set>
#include <sstream>

using namespace Manta;

#define BLEND_OLD_SAMPLES 0
#define USE_SAMPLE_GEN 1

ImageTraverser* DeadlineImageTraverser::create(const vector<string>& args)
{
  return new DeadlineImageTraverser(args);
}

DeadlineImageTraverser::DeadlineImageTraverser(const vector<string>& args)
  : qlock("DeadlineImageTraverser queue lock"), queue(16, 16)
{
  finished_coarse = false;
  reset_every_frame = false;
  converged = false;
  ShowTimeSupersample = false;
  FIFO_cutoff = 1.f;
  use_render_region = false;
  do_dart_benchmark = false;
  max_spp = 1024;

  float frameRate = 15;
  ypacketsize = 1;
  while(ypacketsize * ypacketsize * 2 < Fragment::MaxSize)
    ypacketsize *= 2;
  xpacketsize = Fragment::MaxSize / ypacketsize;
  xcoarsepixelsize = 8;
  ycoarsepixelsize = 8;
  xrefinementRatio = 2;
  yrefinementRatio = 2;
  priority = LuminanceVariance;
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-packetsize"){
      if(!getResolutionArg(i, args, xpacketsize, ypacketsize))
        throw IllegalArgument("DeadlineImageTraverser -packetsize", i, args);
      if(xpacketsize * ypacketsize > Fragment::MaxSize)
        throw IllegalArgument("DeadlineImageTraverser -packetsize too large", i, args);
    } else if(arg == "-framerate"){
      if(!getArg(i, args, frameRate))
        throw IllegalArgument("DeadlineImageTraverser -framerate", i, args);
    } else if(arg == "-magnification"){
      if(!getResolutionArg(i, args, xcoarsepixelsize, ycoarsepixelsize))
        throw IllegalArgument("DeadlineImageTraverser -magnification", i, args);
    } else if(arg == "-priority"){
      string priorityString;
      if(!getArg(i, args, priorityString))
        throw IllegalArgument("DeadlineImageTraverser -priority", i, args);
      if(priorityString == "FIFO")
        priority = FIFO;
      else if(priorityString == "luminancevariance")
        priority = LuminanceVariance;
      else if(priorityString == "contrast")
        priority = Contrast;
      else if(priorityString == "FIFOluminance")
        priority = FIFO_to_LumVar;
      else if(priorityString == "center")
        priority = Center;
      else
        throw IllegalArgument("DeadlineImageTraverser -priority, bad priority", i, args);
    } else if(arg == "-FIFOCutoff") {
      if(!getArg(i, args, FIFO_cutoff))
        throw IllegalArgument("DeadlineImageTraverser -FIFOCutoff", i, args);
    } else if(arg == "-refinmentRatio"){
      if(!getResolutionArg(i, args, xrefinementRatio, yrefinementRatio))
        throw IllegalArgument("DeadlineImageTraverser -refinement", i, args);
    } else if(arg == "-showtime"){
      ShowTimeSupersample = true;
    } else if(arg == "-dart_benchmark"){
      do_dart_benchmark = true;
    } else if(arg == "-max_spp"){
      if(!getArg(i, args, max_spp))
        throw IllegalArgument("DeadlineImageTraverser -max_spp", i, args);
    } else {
      throw IllegalArgument("DeadlineImageTraverser", i, args);
    }
  }
  frameTime = 1./frameRate;
  singleSampler = new SingleSampler(vector<string>());

  jitterSamplers = new JitterSampler*[kNumJitterLevels];
  sampleGenerators = new SampleGenerator*[kNumJitterLevels + 1];
  sampleGenerators[0] = new UniformRandomGenerator();
  int num_spp = 4;
  for (int i = 0; i < kNumJitterLevels; i++) {
    vector<string> args;
    args.push_back("-nocheap");
    args.push_back("-numberOfSamples");

    ostringstream r;
    r << num_spp;
    args.push_back(r.str());
    jitterSamplers[i] = new JitterSampler(args);
    sampleGenerators[i+1] = new Stratified2D(num_spp);
    num_spp *= 4;
  }
}

DeadlineImageTraverser::~DeadlineImageTraverser() {
  delete[] jitterSamplers;
  delete[] sampleGenerators;
}

void DeadlineImageTraverser::setupBegin(SetupContext& context, int numChannels)
{
  singleSampler->setupBegin(context, numChannels);
  for (int i = 0; i < kNumJitterLevels; i++) {
    jitterSamplers[i]->setupBegin(context, numChannels);
  }
  for (int i = 0; i < kNumJitterLevels + 1; i++) {
    sampleGenerators[i]->setupBegin(context, numChannels);
  }
  context.loadBalancer->setupBegin(context, numChannels);
  context.pixelSampler->setupBegin(context, numChannels);

  if (context.proc == 0)
    queue.clear();

  ASSERT(context.proc < DeadlineImageTraverser::kMaxThreads);
  tile_pools[context.proc].clear();

  context.rtrt_int->addIdleMode(this);
}

void DeadlineImageTraverser::changeIdleMode(const SetupContext& context,
                                            bool changed,
                                            bool firstFrame,
                                            bool& pipelineNeedsSetup) {
  if (context.proc == 0) {
    if (firstFrame)
      reset_every_frame = true;
    // NOTE(boulos): Someone probably moved the camera or
    // something... Time to restart.  The setupFrame call for the next
    // frame will clean up the other data (queue and next_tile)
    //cerr << "changeIdleMode called with changed = " << changed << endl;
    if (changed) {
      reset_every_frame = true;
    } else {
      // Nothing is changing... we are idle
      reset_every_frame = false;
    }
  }
}

void DeadlineImageTraverser::setupDisplayChannel(SetupContext& context)
{
  //if (context.proc == 0)
  //  cerr << __func__ << endl;
  StartFrameTime = CPUTime::currentSeconds();
  finished_coarse = false;
  converged = false;
  // Determine the resolution.
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);

  // render region defaults to entire image
  // TODO(arobison): this means that on resizes we will lose
  // a user selected region, so we need to save it and scale it
  setRenderRegion(0,0,xres-1,yres-1);

  // Determine how many coarse tiles are needed.
  int coarse_xres = (xres + xcoarsepixelsize-1)/xcoarsepixelsize;
  int coarse_yres = (yres + ycoarsepixelsize-1)/ycoarsepixelsize;

  int xtiles = (coarse_xres + xpacketsize-1)/xpacketsize;
  int ytiles = (coarse_yres + ypacketsize-1)/ypacketsize;

  // Tell the load balancer how much work to assign.
  int numAssignments = xtiles * ytiles;
  context.loadBalancer->setupDisplayChannel(context, numAssignments);

  // Single bufferd, please.
  context.constrainPipelineDepth(1,1);

  // Continue setting up the rendering stack.
  context.pixelSampler->setupDisplayChannel(context);

  singleSampler->setupDisplayChannel(context);
  // NOTE(boulos): Having the sampler tell the renderer to setup is a
  // bit confusing and redundant but oh well...
  for (int i = 0; i < kNumJitterLevels; i++) {
    jitterSamplers[i]->setupDisplayChannel(context);
  }

  for (int i = 0; i < kNumJitterLevels + 1; i++) {
    sampleGenerators[i]->setupDisplayChannel(context);
  }
}

void DeadlineImageTraverser::setupFrame(const RenderContext& context)
{
  // TODO(boulos): Determine what stuff only proc0 should do...
  //if (context.proc == 0)
  //  cerr << __func__ << endl;
  // TODO(boulos): What other stuff shouldn't get reset?
  if (reset_every_frame && context.proc == 0) {
    //if (context.proc == 0) cerr << "We are resetting every frame"<< endl;
    finished_coarse = false;
  }
  if (!finished_coarse) {
    context.loadBalancer->setupFrame(context);
    if (context.proc == 0) {
      queue.clear();
      converged = false;
      StartFrameTime = CPUTime::currentSeconds();
    }

    ASSERT(context.proc < DeadlineImageTraverser::kMaxThreads);
    tile_pools[context.proc].clear();
  }

  context.pixelSampler->setupFrame(context);

  singleSampler->setupFrame(context);
  for (int i = 0; i < kNumJitterLevels; i++) {
    jitterSamplers[i]->setupFrame(context);
  }
  for (int i = 0; i < kNumJitterLevels + 1; i++) {
    sampleGenerators[i]->setupFrame(context);
  }
  // Determine how long we should render this frame.
  frameEnd = CPUTime::currentSeconds() + frameTime;
  if (context.proc == 0) {
    if (finished_coarse && queue.empty() && !converged) {
      converged = true;
      double EndTime = CPUTime::currentSeconds();
      double total_time = EndTime - StartFrameTime;
      if (do_dart_benchmark) {
        //This is an ugly hack to get benchmarking information out of
        //this constant frame rate image traverser. Really don't like
        //having this hardcoded exit though.
        std::cout << "<DartMeasurement name=\"total_time\" type=\"numeric/double\">"<<total_time<<"</DartMeasurement>\n";
        context.rtrt_int->finish();
      } else {
        cerr << "Took " << total_time << " seconds to refine to "<< max_spp <<" spp" << endl;
      }
    }
  }
}

void DeadlineImageTraverser::insertIntoQueue(Tile* tile, size_t thread_id)
{
  queue.push(tile, tile->priority, thread_id);
}

void DeadlineImageTraverser::insertIntoQueue(vector<Tile*>& tiles, size_t thread_id)
{
  for(size_t i=0; i < tiles.size(); i++) {
    Tile* tile = tiles[i];
    queue.push(tile, tile->priority, thread_id);
  }
}

DeadlineImageTraverser::Tile* DeadlineImageTraverser::popFromQueue(size_t thread_id) {
  Tile* result = queue.pop(thread_id);
  return result;
}

void DeadlineImageTraverser::renderImage(const RenderContext& context, Image* image)
{
  // Determine number of coarse level
  if (converged) return;
  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);
  int numEyes = stereo?2:1;

#if USE_SAMPLE_GEN
  RenderContext& mutable_context = const_cast<RenderContext&>(context);
  SampleGenerator* original_samplegen = context.sample_generator;
#endif

  // NOTE(boulos): Only the coarse_yres and ytiles variables are being
  // used to determine the xtile and ytile of the assignment.

  //int coarse_xres = (xres + xcoarsepixelsize - 1) / xcoarsepixelsize;
  int coarse_yres = (yres + ycoarsepixelsize - 1) / ycoarsepixelsize;

  //int xtiles = (coarse_xres+ xpacketsize-1)/xpacketsize;
  int ytiles = (coarse_yres + ypacketsize-1)/ypacketsize;
  int xcoarsetilesize = xpacketsize * xcoarsepixelsize;
  int ycoarsetilesize = ypacketsize * ycoarsepixelsize;

  // First pass - do all of the coarse level tiles with the normal
  // load balancer
  int s,e;
  // NOTE(boulos): If we've finished the coarse tiles, we have no
  // assignments left
  while(context.loadBalancer->getNextAssignment(context, s, e)){
    for(int assignment = s; assignment < e; assignment++){
      int xtile = assignment/ytiles;
      int ytile = assignment%ytiles;
      int xstart = xtile * xcoarsetilesize;
      int xend = (xtile+1) * xcoarsetilesize;
      bool isSquare = true;
      if(xend > xres) {
        xend = xres;
        isSquare = false;
      }

      int ystart = ytile * ycoarsetilesize;
      int yend = (ytile+1) * ycoarsetilesize;

      if(yend > yres) {
        yend = yres;
        isSquare = false;
      }

      Fragment frag((isSquare) ? Fragment::SquareShape : Fragment::UnknownShape,
                    Fragment::ConstantEye);
      frag.setPixelSize(xcoarsepixelsize, ycoarsepixelsize);

      for(int eye = 0; eye < numEyes; eye++){
        int idx = 0;
        for (int j = ystart; j < yend; j += ycoarsepixelsize) {
          for (int i = xstart; i < xend; i += xcoarsepixelsize) {
            frag.pixel[0][idx] = i;
            frag.pixel[1][idx] = j;
            frag.whichEye[idx] = eye;
            idx++;
          }
        }
        frag.setSize(idx);
        if((xcoarsepixelsize | ycoarsepixelsize) > 1){
#if USE_SAMPLE_GEN
          mutable_context.sample_generator = sampleGenerators[0];
#endif
          singleSampler->renderFragment(context, frag);

          //Tile* tile = new Tile;
          ASSERT(context.proc < DeadlineImageTraverser::kMaxThreads);
          Tile* tile = tile_pools[context.proc].getItem();

          tile->xstart = xstart;
          tile->ystart = ystart;
          tile->xend = xend;
          tile->yend = yend;
          tile->xmag = xcoarsepixelsize;
          tile->ymag = ycoarsepixelsize;
          computePriority(tile, frag);
          insertIntoQueue(tile, static_cast<size_t>(context.proc));
        } else {
#if USE_SAMPLE_GEN
          mutable_context.sample_generator = sampleGenerators[0];
#endif
          context.pixelSampler->renderFragment(context, frag);
        }
        image->set(frag);
      }
    }
  }

  // NOTE(boulos): I don't care about this, but James thinks we should
  // check ;)
  if (!finished_coarse)
    finished_coarse = true;

  vector<Tile*> childtiles;
  childtiles.reserve(xrefinementRatio * yrefinementRatio);

  while(CPUTime::currentSeconds() < frameEnd){
    Tile* tile = popFromQueue(static_cast<size_t>(context.proc));

    if(!tile)
      continue;

    //cerr << "tile->priority = " << tile->priority << endl;

    childtiles.clear();

    float newxmag = tile->xmag/xrefinementRatio;
    float newymag = tile->ymag/yrefinementRatio;

    if(newxmag < 1 || newymag < 1) {
      // Super sample the fragments. We want our fragment to exactly
      // place 1./newxmag samples into each pixel, so we need to
      // determine how many pixels we can render per packet.
      int x_samples = static_cast<int>(1./newxmag);
      int y_samples = static_cast<int>(1./newymag);

      // TODO(boulos): Can we do some other number of samples?
      int samples_per_pixel = x_samples * y_samples;

      if (samples_per_pixel > max_spp)
        continue;
      // NOTE(boulos): We know that we don't just stop anymore due
      // to super sampling
      JitterSampler* jitter_sampler = NULL;
      switch (samples_per_pixel) {
      case 4:
        //cerr << "Using a 4 spp sampler" << endl;
        jitter_sampler = jitterSamplers[0];
#if USE_SAMPLE_GEN
        mutable_context.sample_generator = sampleGenerators[1];
#endif
        break;
      case 16:
        //cerr << "Using a 16 spp sampler" << endl;
        jitter_sampler = jitterSamplers[1];
#if USE_SAMPLE_GEN
        mutable_context.sample_generator = sampleGenerators[2];
#endif
        break;
      case 64:
        //cerr << "Using a 64 spp sampler" << endl;
        jitter_sampler = jitterSamplers[2];
#if USE_SAMPLE_GEN
        mutable_context.sample_generator = sampleGenerators[3];
#endif
        break;
      case 256:
        //cerr << "Using a 256 spp sampler" << endl;
        jitter_sampler = jitterSamplers[3];
#if USE_SAMPLE_GEN
        mutable_context.sample_generator = sampleGenerators[4];
#endif
        break;
      case 1024:
        jitter_sampler = jitterSamplers[4];
#if USE_SAMPLE_GEN
        mutable_context.sample_generator = sampleGenerators[5];
#endif
        break;
      case 4096:
        jitter_sampler = jitterSamplers[5];
#if USE_SAMPLE_GEN
        mutable_context.sample_generator = sampleGenerators[6];
#endif
        break;
      case 16384:
        jitter_sampler = jitterSamplers[6];
#if USE_SAMPLE_GEN
        mutable_context.sample_generator = sampleGenerators[7];
#endif
        break;

      default:
        throw InternalError("Expected either 4, 16, 64, 256, 1024, 4096, or 16384 spp");
        break;
      }

      int xs = static_cast<int>(xpacketsize * newxmag);
      int ys = static_cast<int>(ypacketsize * newymag);
      // We might be asking for more samples than we can fit in a
      // packet.
      if (xs < 1 || ys < 1) {
        // Handle multiple packets per pixel.
        //cerr << "Multiple packets per pixel (spp = " << samples_per_pixel << ", newxmag = " << newxmag << ")\n";
        if (xs != ys) {
          throw InternalError("Don't handle non-square refinement yet");
        }

        Fragment frag(Fragment::SquareShape, Fragment::ConstantEye);
        frag.setPixelSize(1, 1);
        frag.pixel[0][0] = tile->xstart;
        frag.pixel[1][0] = tile->ystart;
        frag.whichEye[0] = 0;
        frag.setSize(1);
        jitter_sampler->renderFragment(context, frag);

        //Tile* childtile = new Tile;
        ASSERT(context.proc < DeadlineImageTraverser::kMaxThreads);
        Tile* childtile = tile_pools[context.proc].getItem();
        childtile->xstart = tile->xstart;
        childtile->xend   = tile->xstart;
        childtile->ystart = tile->ystart;
        childtile->yend   = tile->ystart;
        childtile->xmag = newxmag;
        childtile->ymag = newymag;
        computePriority(childtile, frag);
        childtiles.push_back(childtile);

#if BLEND_OLD_SAMPLES
        int prev_xs = static_cast<int>(xpacketsize * xrefinementRatio * newxmag);
        int prev_ys = static_cast<int>(ypacketsize * yrefinementRatio * newymag);

        if (prev_xs < 1 && prev_ys < 1) {
          Fragment prev_frag(Fragment::SquareShape, Fragment::ConstantEye);
          prev_frag.setPixelSize(1, 1);
          prev_frag.pixel[0][0] = tile->xstart;
          prev_frag.pixel[1][0] = tile->ystart;
          prev_frag.whichEye[0] = 0;
          prev_frag.setSize(1);
          image->get(prev_frag);
          // TODO(boulos): Determine kOldAlpha according to refinement...
          const float kOldAlpha = 0.2f; // 1/5 and 4/5
          prev_frag.scaleColor(0, kOldAlpha);

          frag.scaleColor(0, 1.f-kOldAlpha);
          frag.addColor(0, prev_frag.getColor(0));
        }
#endif


        image->set(frag);
      } else {
        for(int y = tile->ystart; y < tile->yend; y += ys){
          for(int x = tile->xstart; x < tile->xend; x += xs){
            bool isSquare = true;
            int xend = x + xs;
            if(xend > tile->xend) {
              xend = tile->xend;
              isSquare = false;
            }
            int yend = y + ys;
            if(yend > tile->yend) {
              yend = tile->yend;
              isSquare = false;
            }

            // NOTE(boulos): This logic is now slightly different as we
            // are going to make smaller fragments.  This makes it so we
            // don't need to change the render fragment logic (a single
            // fragment will now just have many samples) so the fragment
            // size will be smaller than before
            Fragment frag((isSquare) ? Fragment::SquareShape : Fragment::UnknownShape,
                          Fragment::ConstantEye);
            frag.setPixelSize(1, 1);
            int idx = 0;
            for (int j = y; j < yend; j++) {
              for (int i = x; i < xend; i++) {
                frag.pixel[0][idx] = i;
                frag.pixel[1][idx] = j;
                frag.whichEye[idx] = 0;
                idx++;
              }
            }
            frag.setSize(idx);

            //cout << "Rendering a super sampled fragment" << endl;
            jitter_sampler->renderFragment(context, frag);

            if (ShowTimeSupersample) {
              ColorComponent color = CPUTime::currentSeconds()/60.;
              for (int i = frag.begin(); i < frag.end(); i++) {
                for ( int c = 0; c < Color::NumComponents; c++ )
                  frag.color[c][i] = color;
              }
            }

            //Tile* childtile = new Tile;
            ASSERT(context.proc < DeadlineImageTraverser::kMaxThreads);
            Tile* childtile = tile_pools[context.proc].getItem();
            childtile->xstart = x;
            childtile->xend = xend;
            childtile->ystart = y;
            childtile->yend = yend;
            childtile->xmag = newxmag;
            childtile->ymag = newymag;
            computePriority(childtile, frag);
            childtiles.push_back(childtile);
            image->set(frag);
          }
        }
      }
    } else {
      int newxmag_int = static_cast<int>(newxmag);
      int newymag_int = static_cast<int>(newymag);

      int xs = xpacketsize * newxmag_int;
      int ys = ypacketsize * newymag_int;

      for(int y = tile->ystart; y < tile->yend; y += ys){
        for(int x = tile->xstart; x < tile->xend; x += xs){
          bool isSquare = true;
          int xend = x + xs;
          if(xend > tile->xend) {
            xend = tile->xend;
            isSquare = false;
          }
          int yend = y + ys;
          if(yend > tile->yend) {
            yend = tile->yend;
            isSquare = false;
          }

          Fragment frag((isSquare) ? Fragment::SquareShape : Fragment::UnknownShape, Fragment::ConstantEye);
          frag.setPixelSize(newxmag_int, newymag_int);
          int idx = 0;
          for (int j = y; j < yend; j+=newymag_int) {
            for (int i = x; i < xend; i+=newxmag_int) {
              frag.pixel[0][idx] = i;
              frag.pixel[1][idx] = j;
              frag.whichEye[idx] = 0;
              idx++;
            }
          }
          frag.setSize(idx);

          // NOTE(boulos): We know that we don't just stop anymore due
          // to super sampling
#if USE_SAMPLE_GEN
          mutable_context.sample_generator = sampleGenerators[0];
#endif
          singleSampler->renderFragment(context, frag);
          //Tile* childtile = new Tile;
          ASSERT(context.proc < DeadlineImageTraverser::kMaxThreads);
          Tile* childtile = tile_pools[context.proc].getItem();
          childtile->xstart = x;
          childtile->xend = xend;
          childtile->ystart = y;
          childtile->yend = yend;
          childtile->xmag = newxmag;
          childtile->ymag = newymag;
          computePriority(childtile, frag);
          childtiles.push_back(childtile);

          image->set(frag);
        }
      }
    }

    // Check size before inserting to avoid lock contention
    if(childtiles.size())
      insertIntoQueue(childtiles, static_cast<size_t>(context.proc));
  }

  // This can potentially happen before the other procesors are finished
  // rendering, but that is okay because it won't get displayed until
  // everyone enters the barrier anyway
#if USE_SAMPLE_GEN
  mutable_context.sample_generator = original_samplegen;
#endif
  if(context.proc == 0)
    image->setValid(true);
}

float DeadlineImageTraverser::computeLuminancePriority(Tile* tile, const Fragment& frag) const
{
  double sum = 0;
  double sum2 = 0;
  for(int i=frag.begin();i<frag.end();i++){
    Color::ComponentType luminance = frag.getColor(i).luminance();
    sum += luminance;
    sum2 += luminance * luminance;
  }
  int n = frag.end()-frag.begin();
  Color::ComponentType variance = (sum2 - sum * sum/n)/(n);
  return variance * tile->xmag * tile->ymag;
}

float DeadlineImageTraverser::computeContrastPriority(Tile* tile, const Fragment& frag) const
{
  Color::ComponentType min, max;
  min = max = frag.getColor(frag.begin()).luminance();
  for(int i=frag.begin()+1;i<frag.end();i++){
    Color::ComponentType luminance = frag.getColor(i).luminance();
    if(luminance < min)
      min = luminance;
    if(luminance > max)
      max = luminance;
  }
  return (max-min)/(max+min) * tile->xmag  * tile->ymag ;
}

float DeadlineImageTraverser::computeFIFOPriority(Tile* tile, const Fragment& frag) const
{
  return tile->xmag * tile->ymag;
}

float DeadlineImageTraverser::computeCenterPriority(Tile* tile, const Fragment& frag) const
{
  int tile_x_center = (tile->xend - tile->xstart)/2 + tile->xstart;
  int tile_y_center = (tile->yend - tile->ystart)/2 + tile->ystart;

#if 0
  // manhattan distance
  int dist = Abs(tile_x_center - render_region.xcenter) +
    Abs(tile_y_center - render_region.ycenter) ;
#else
  // euclidean distance
  int dx = Abs(tile_x_center - render_region.xcenter);
  int dy = Abs(tile_y_center - render_region.ycenter);
  int dist = Sqrt(dx*dx + dy*dy);
#endif

  return (1/static_cast<Real>(dist)) * tile->xmag * tile->ymag;
}

void DeadlineImageTraverser::computePriority(Tile* tile, const Fragment& frag) const
{
  // set the priority of tiles outside the render region to zero
  if(use_render_region) {
    if( tile->xstart < render_region.xstart ||
        tile->ystart < render_region.ystart ||
        tile->xend   > render_region.xend   ||
        tile->yend   > render_region.yend ) {
      tile->priority = 0;
      return;
    }
  }

  switch(priority) {
  case LuminanceVariance:
    tile->priority = computeLuminancePriority(tile, frag);
    break;

  case Contrast:
    tile->priority = computeLuminancePriority(tile, frag);
    break;

  case FIFO:
    tile->priority = computeFIFOPriority(tile, frag);
    break;

  case FIFO_to_LumVar:
    if(tile->xmag < FIFO_cutoff || tile->ymag < FIFO_cutoff)
      tile->priority = computeLuminancePriority(tile, frag);
    else
      tile->priority = computeFIFOPriority(tile, frag);
    break;

  case Center:
    tile->priority = computeCenterPriority(tile, frag);
    break;

  default:
    throw InternalError("Bad priority in computePriority");
    break;
  }
}

