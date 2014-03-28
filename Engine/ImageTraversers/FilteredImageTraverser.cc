
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

#include <Engine/ImageTraversers/FilteredImageTraverser.h>

#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/Image.h>
#include <Interface/LoadBalancer.h>
#include <Interface/PixelSampler.h>
#include <Core/Thread/Mutex.h>
#include <Core/Util/Assert.h>

using namespace Manta;

ImageTraverser* FilteredImageTraverser::create(const vector<string>& args)
{
  return new FilteredImageTraverser(args);
}

FilteredImageTraverser::FilteredImageTraverser(const vector<string>& args)
{
  xtilesize = 32;
  ytilesize = 2;
  for(size_t i = 0; i<args.size();i++){
    string arg = args[i];
    if(arg == "-tilesize"){
      if(!getResolutionArg(i, args, xtilesize, ytilesize))
        throw IllegalArgument("FilteredImageTraverser -tilesize", i, args);
    } else {
      throw IllegalArgument("FilteredImageTraverser", i, args);
    }
  }
}

FilteredImageTraverser::~FilteredImageTraverser()
{
}

void FilteredImageTraverser::setupBegin(SetupContext& context, int numChannels)
{
  context.loadBalancer->setupBegin(context, numChannels);
  context.pixelSampler->setupBegin(context, numChannels);
}

void FilteredImageTraverser::setupDisplayChannel(SetupContext& context)
{
  /////////////////////////////////////////////////////////////////////////////
  // Setup the image traverser.
  
  // Determine the resolution.
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);

  // Determine how many tiles are needed.
  xtiles = (xres + xtilesize-1)/xtilesize;
  ytiles = (yres + ytilesize-1)/ytilesize;

  // Determine how much storage is necessary for one tile's worth of
  // fragments.
  int total_pixels = xtilesize * ytilesize;
  total_allocated = ((total_pixels/Fragment::MaxSize)+1) * (stereo?2:1);

  size_t bytes = sizeof( Fragment ) * total_allocated;
    

  // Request storage for persistent fragments.
  storage_token = context.storage_allocator->requestStorage( bytes, 128 );

  /////////////////////////////////////////////////////////////////////////////
  // Setup the rest of the stack.
  
  // Tell the load balancer how much work to assign.
  int numAssignments = xtiles * ytiles;
  context.loadBalancer->setupDisplayChannel(context, numAssignments);

  // Continue setting up the rendering stack.
  context.pixelSampler->setupDisplayChannel(context);
}

void FilteredImageTraverser::setupFrame(const RenderContext& context)
{
  /////////////////////////////////////////////////////////////////////////////
  // Setup the image traverser.
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Setup the rest of the stack.
  context.loadBalancer->setupFrame(context);
  context.pixelSampler->setupFrame(context);
}

void FilteredImageTraverser::renderImage(const RenderContext& context, Image* image)
{
  // Obtain a pointer to persistant storage.
  Fragment *fragment_array =
    (Fragment *)context.storage_allocator->get( context.proc, storage_token );

  // Determine resolution
  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);

  // Call the load balancer to get work assignments.
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

      // Keep track of which persistent fragment to copy into.
      int total_fragments = 0;

      /////////////////////////////////////////////////////////////////////////
      // Create the fragments.
      for(int y = ystart; y<yend; y++){
        for(int x = xstart; x<xend; x+= Fragment::MaxSize){

          // Set the fragment elements.
          ASSERT( total_fragments < total_allocated );
          
          Fragment &frag = fragment_array[total_fragments++];
          frag.setConsecutiveX( x, xend, y, 0 );
          
        }
      }

      // Check to see if we need to render another copy in setero.
      if(stereo){
        for(int y = ystart; y<yend; y++){
          for(int x = xstart; x<xend; x+= Fragment::MaxSize){
            
            // Create a Fragment that is consecutive in X pixels
            // Fragment frag(0, x, xend, y);
            
            // Set the fragment elements.
            ASSERT( total_fragments < total_allocated );
            
            Fragment &frag = fragment_array[total_fragments++];
            frag.setConsecutiveX( x, xend, y, 1);
          }
        }
      }

      /////////////////////////////////////////////////////////////////////////
      // Render the fragments.
      for (int i=0;i<total_fragments; ++i)
        context.pixelSampler->renderFragment( context, fragment_array[i] );

      // Filter the fragments.
      for (int i=0;i<total_fragments; ++i)
        fragment_array[i].scaleColors(0.5);

      // Set image values.
      for (int i=0;i<total_fragments; ++i)
        image->set( fragment_array[i] );
    }
  }
  
  // Specify that the image is valid.
  if(context.proc == 0)
    image->setValid(true);
}
