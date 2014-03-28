
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

#include <Engine/ImageTraversers/TiledImageTraverser.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Math/MinMax.h>
#include <Core/Math/Expon.h>
#include <Core/Thread/Mutex.h>
#include <Core/Util/Args.h>
#include <Core/Util/NotFinished.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/Image.h>
#include <Interface/LoadBalancer.h>
#include <Interface/PixelSampler.h>
#include <Interface/RayPacket.h> // for maxsize
#include <Interface/RandomNumberGenerator.h>
#include <MantaSSE.h>

using namespace Manta;

ImageTraverser* TiledImageTraverser::create(const vector<string>& args)
{
  return new TiledImageTraverser(args);
}

TiledImageTraverser::TiledImageTraverser( const int xtilesize_, const int ytilesize_ )
  : xtilesize( xtilesize_ ), ytilesize( ytilesize_ )
{
  if (xtilesize == ytilesize)
    shape = Fragment::SquareShape;
  else
    shape = Fragment::LineShape;

}

TiledImageTraverser::TiledImageTraverser(const vector<string>& args)
{
#if 0
  xtilesize = Fragment::MaxSize;
  ytilesize = Fragment::MaxSize;
#else
  // NOTE(boulos): Personally, I think our tiles should be smaller
  // than this. But this maintains the current performance for a
  // RayPacket::MaxSize = 64, and improves performance when using even
  // larger packets.
  xtilesize = Min(64, 8 * static_cast<int>(Sqrt(static_cast<Real>(Fragment::MaxSize))));
  ytilesize = Min(64, 8 * static_cast<int>(Sqrt(static_cast<Real>(Fragment::MaxSize))));
#endif
  shape = Fragment::LineShape;
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-tilesize"){
      if(!getResolutionArg(i, args, xtilesize, ytilesize))
        throw IllegalArgument("TiledImageTraverser -tilesize", i, args);
    } else if (arg == "-square") {
/*
        int sqrt_size = static_cast<int>(Sqrt(RayPacket::MaxSize));
        if (sqrt_size * sqrt_size != RayPacket::MaxSize)
            throw IllegalArgument("TiledImageTraverser -square (RayPacket::MaxSize is not square",
                                  i, args);
*/
        shape = Fragment::SquareShape;
    } else {
      throw IllegalArgument("TiledImageTraverser", i, args);
    }
  }
}

TiledImageTraverser::~TiledImageTraverser()
{
}

void TiledImageTraverser::setupBegin(SetupContext& context, int numChannels)
{
  context.loadBalancer->setupBegin(context, numChannels);
  context.pixelSampler->setupBegin(context, numChannels);
}

void TiledImageTraverser::setupDisplayChannel(SetupContext& context)
{
  // Determine the resolution.
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);

  // Determine how many tiles are needed.
  xtiles = (xres + xtilesize-1)/xtilesize;
  ytiles = (yres + ytilesize-1)/ytilesize;

  // Tell the load balancer how much work to assign.
  int numAssignments = xtiles * ytiles;
  context.loadBalancer->setupDisplayChannel(context, numAssignments);

  // Continue setting up the rendering stack.
  context.pixelSampler->setupDisplayChannel(context);
}

void TiledImageTraverser::setupFrame(const RenderContext& context)
{
  context.loadBalancer->setupFrame(context);
  context.pixelSampler->setupFrame(context);
}

void TiledImageTraverser::renderImage(const RenderContext& context, Image* image)
{

  // Determine number of tiles.
  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);
  int numEyes = stereo?2:1;

  int s,e;
  while(context.loadBalancer->getNextAssignment(context, s, e)){

    for(int assignment = s; assignment < e; assignment++){

      int xtile = assignment/ytiles;
      int ytile = assignment%ytiles;
      int xstart = xtile * xtilesize;
      int xend = (xtile+1) * xtilesize;

      if(xend > xres)
        xend = xres;

      int ystart = ytile * ytilesize;
      int yend = (ytile+1) * ytilesize;

      if(yend > yres)
        yend = yres;

#ifdef MANTA_SSE
      __m128i vec_4 = _mm_set1_epi32(4);
      __m128i vec_cascade = _mm_set_epi32(3, 2, 1, 0);
#endif
      // Create a Fragment that is consecutive in X pixels
      switch (shape) {
      case Fragment::LineShape:
      {
          Fragment frag(Fragment::LineShape, Fragment::ConsecutiveX|Fragment::ConstantEye);

          int fsize = Min(Fragment::MaxSize, xend-xstart);
          for(int eye = 0; eye < numEyes; eye++){
#ifdef MANTA_SSE
              // This finds the upper bound of the groups of 4.  Even if you
              // write more than you need (say 8 for a fragment size of 6),
              // you won't blow the top of the array, because
              // (Fragment::MaxSize+3)&(~3) == Fragment::MaxSize.
              int e = (fsize+3)&(~3);
              __m128i vec_eye = _mm_set1_epi32(eye);
              for(int i=0;i<e;i+=4)
                  _mm_store_si128((__m128i*)&frag.whichEye[i], vec_eye);
#else
              for(int i=0;i<fsize;i++)
                  frag.whichEye[i] = eye;
#endif

              // Two versions.  If the assignment is narrower than a fragment, we
              // can enable a few optimizations
              if(xend-xstart <= Fragment::MaxSize){
                  // Common case - one packet in X direction
                  int size = xend-xstart;
#ifdef MANTA_SSE
                  __m128i vec_x = _mm_add_epi32(_mm_set1_epi32(xstart), vec_cascade);
                  for(int i=0;i<size;i+=4){
                      // This will spill over by up to 3 pixels
                      _mm_store_si128((__m128i*)&frag.pixel[0][i], vec_x);
                      vec_x = _mm_add_epi32(vec_x, vec_4);
                  }
#else
                  for(int i=0;i<size;i++)
                      frag.pixel[0][i] = i+xstart;
#endif
                  frag.setSize(size);
                  for(int y = ystart; y<yend; y++){
#ifdef MANTA_SSE
                      int e = (fsize+3)&(~3);
                      __m128i vec_y = _mm_set1_epi32(y);
                      for(int i=0;i<e;i+=4)
                          _mm_store_si128((__m128i*)&frag.pixel[1][i], vec_y);
#else
                      for(int i=0;i<fsize;i++)
                          frag.pixel[1][i] = y;
#endif
                      context.rng->seed(xstart*xres+y);
                      context.pixelSampler->renderFragment(context, frag);
                      image->set(frag);
                  }
              } else {
                  // General case (multiple packets in X direction)
                  for(int y = ystart; y<yend; y++){
#ifdef MANTA_SSE
                      int e = (fsize+3)&(~3);
                      __m128i vec_y = _mm_set1_epi32(y);
                      for(int i=0;i<e;i+=4)
                          _mm_store_si128((__m128i*)&frag.pixel[1][i], vec_y);
#else
                      for(int i=0;i<fsize;i++)
                          frag.pixel[1][i] = y;
#endif
                      for(int x = xstart; x<xend; x+= Fragment::MaxSize){
                          // This catches cases where xend-xstart is larger than
                          // Fragment::MaxSize.
                          int xnarf = x+Fragment::MaxSize;
                          if (xnarf > xend) xnarf = xend;
                          int size = xnarf-x;
#ifdef MANTA_SSE
                          __m128i vec_x = _mm_add_epi32(_mm_set1_epi32(x), vec_cascade);
                          for(int i=0;i<size;i+=4){
                              // This will spill over by up to 3 pixels
                              _mm_store_si128((__m128i*)&frag.pixel[0][i], vec_x);
                              vec_x = _mm_add_epi32(vec_x, vec_4);
                          }
#else
                          for(int i=0;i<size;i++)
                              frag.pixel[0][i] = i+x;
#endif
                          frag.setSize(size);
                          context.rng->seed(x*xres+y);
                          context.pixelSampler->renderFragment(context, frag);
                          image->set(frag);
                      }
                  }
              }
          }
      }
      break;
      case Fragment::SquareShape:
      {
          Fragment frag(Fragment::SquareShape, Fragment::ConstantEye);

          // Square Shaped fragments of about RayPacket::MaxSize each
          static const int sqrt_size = static_cast<int>(Sqrt(RayPacket::MaxSize));
          static const int full_size = sqrt_size * sqrt_size;

          for(int eye = 0; eye < numEyes; eye++){

              for(int i=0;i<full_size;i++)
                  frag.whichEye[i] = eye;

              for (int y = ystart; y < yend; y += sqrt_size) {
                  for (int x = xstart; x < xend; x += sqrt_size) {

                      int j_end = Min(yend - y, sqrt_size);
                      int i_end = Min(xend - x, sqrt_size);

                      for (int j = 0; j < j_end; ++j) {
                          for (int i = 0; i < i_end; ++i) {
                              frag.pixel[0][j*i_end + i] = x + i;
                              frag.pixel[1][j*i_end + i] = y + j;
                          }
                      }
                      // NOTE(boulos): If these get clipped, it's not
                      // actually Square (but it is a
                      // Rectangle... maybe we should add that?)
                      if (i_end != sqrt_size ||
                          j_end != sqrt_size) {
                        frag.shape = Fragment::UnknownShape;
                      }
                      frag.setSize(j_end * i_end);
                      context.rng->seed(x*xres+y);
                      context.pixelSampler->renderFragment(context, frag);

                      image->set(frag);
                  }
              }
          }
      }
      break;
      default:
          break;
      }

    }
  }
  // This can potentially happen before the other procesors are finished
  // rendering, but that is okay because it won't get displayed until
  // everyone enters the barrier anyway
  if(context.proc == 0)
    image->setValid(true);
}
