
#include <Engine/PixelSamplers/SingleSampler.h>
#include <Interface/Context.h>
#include <Interface/Fragment.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/SampleGenerator.h>
#include <MantaSSE.h>
using namespace Manta;

PixelSampler* SingleSampler::create(const vector<string>& args)
{
  return new SingleSampler(args);
}

SingleSampler::SingleSampler(const vector<string>& /* args */)
{
}

SingleSampler::~SingleSampler()
{
}

void SingleSampler::setupBegin(const SetupContext& context, int numChannels)
{
  channelInfo.resize(numChannels);
  context.renderer->setupBegin(context, numChannels);
  context.sample_generator->setupBegin(context, numChannels);
}

void SingleSampler::setupDisplayChannel(SetupContext& context)
{
  ChannelInfo& ci = channelInfo[context.channelIndex];
  bool stereo;
  context.getResolution(stereo, ci.xres, ci.yres);

  // Set up the scale from -1 to 1
  ci.xscale = (Real)2/ci.xres;
  ci.yscale = (Real)2/ci.yres;
  ci.xoffset = (-ci.xres/(Real)2+(Real)0.5)*ci.xscale; // Offset to pixel center
  ci.yoffset = (-ci.yres/(Real)2+(Real)0.5)*ci.yscale;
  context.renderer->setupDisplayChannel(context);
  context.sample_generator->setupDisplayChannel(context);
}

void SingleSampler::setupFrame(const RenderContext& context)
{
  context.renderer->setupFrame(context);
  context.sample_generator->setupFrame(context);
}

void SingleSampler::renderFragment(const RenderContext& context,
                                   Fragment& fragment)
{
  ChannelInfo& ci = channelInfo[context.channelIndex];
  int flags = RayPacket::HaveImageCoordinates;
  if(fragment.getFlag(Fragment::ConstantEye))
    flags |= RayPacket::ConstantEye;
  for(int f=fragment.begin();f<fragment.end();f+=RayPacket::MaxSize){
    // We want to fill our RayPacket with as many as
    // RayPacket::MaxSize rays.
    int size = RayPacket::MaxSize;
    if(size >= fragment.end()-f)
      // We don't have enough fragments left to fill a ray packet, so
      // set the size of the RayPacket to the number of fragments we
      // have left.
      size = fragment.end()-f;
    // Create a ray packet
    int depth = 0;
    RayPacketData raydata;
    RayPacket rays(raydata, RayPacket::UnknownShape, 0, size, depth, flags);

    // Check to see if the fragment is consecutive in x.
    if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)){

      // If so place each pixel in the ray packet relative to the first
      // fragment.
      Real px = fragment.getX(f)*ci.xscale+ci.xoffset;
      Real py = fragment.getY(f)*ci.yscale+ci.yoffset;
      int eye = fragment.getWhichEye(f);
      int pixel_id = fragment.getY(f) * ci.xres + fragment.getX(f);

#if MANTA_SSE
      // The code used to load in the values using _mm_set_ps, but
      // that seemed to be broken for certain gcc compilers.  Stuffing
      // the values into cascade4 first and then loading it, seems to
      // work.
      MANTA_ALIGN(16) float cascade4[4] = {0.0f,
                                           ci.xscale,
                                           ci.xscale+ci.xscale,
                                           ci.xscale+ci.xscale+ci.xscale};
      MANTA_ALIGN(16) int pixel_addition[4] = {0, 1, 2, 3};
      __m128 vec_xscale_cascade = _mm_load_ps(cascade4);
      __m128 vec_px = _mm_add_ps(_mm_set1_ps(px), vec_xscale_cascade);
      __m128 vec_py = _mm_set1_ps(py);
      __m128i vec_eye = _mm_set1_epi32(eye);
      __m128i vec_pixel_id = _mm_add_epi32(_mm_set1_epi32(pixel_id),
                                           _mm_load_si128((__m128i*)&pixel_addition));
      RayPacketData* data = rays.data;
      int e = size&(~3);
      __m128 vec_xscale4 = _mm_set1_ps(ci.xscale*4);
      for(int i=0;i<e;i+=4){
        _mm_store_si128((__m128i*)&data->whichEye[i], vec_eye);
        _mm_store_si128((__m128i*)&data->sample_id[i], _mm_set1_epi32(0));
        _mm_store_si128((__m128i*)&data->region_id[i], vec_pixel_id);
        _mm_store_ps(&data->image[0][i], vec_px);
        _mm_store_ps(&data->image[1][i], vec_py);
        vec_px = _mm_add_ps(vec_px, vec_xscale4);
        vec_pixel_id = _mm_add_epi32(vec_pixel_id, _mm_set1_epi32(4));
      }
      px += e*ci.xscale;
#else
      int e = 0;
#endif
      for(int i=e;i<size;i++){
        rays.setPixel(i, eye, px, py);
        rays.data->sample_id[i] = 0;
        rays.data->region_id[i] = pixel_id + i;
        px += ci.xscale;
      }
    }
    else if (fragment.xPixelSize != 1 || fragment.yPixelSize != 1){
      // Need to do centered setting of the pixels
      Real x_splat_offset = .5 * fragment.xPixelSize;
      Real y_splat_offset = .5 * fragment.yPixelSize;

      for(int i=0;i<size;i++){
        Real px = (fragment.getX(f+i)+x_splat_offset)*ci.xscale+ci.xoffset;
        Real py = (fragment.getY(f+i)+y_splat_offset)*ci.yscale+ci.yoffset;

        rays.setPixel(i, fragment.getWhichEye(f+i), px, py);
        rays.data->sample_id[i] = 0;
        rays.data->region_id[i] = fragment.getY(f+i) * ci.xres + fragment.getX(f+i);
      }
    }
    // Otherwise, set each pixel individually.
    else {
#if MANTA_SSE
      RayPacketData* data = rays.data;
      int e = size&(~3);
      __m128 vec_xoffset = _mm_set1_ps(ci.xoffset);
      __m128 vec_yoffset = _mm_set1_ps(ci.yoffset);
      __m128 vec_xscale = _mm_set1_ps(ci.xscale);
      __m128 vec_yscale = _mm_set1_ps(ci.yscale);
      for(int i=0;i<e;i+=4) {
        _mm_store_si128((__m128i*)&data->whichEye[i], _mm_load_si128((__m128i*)&fragment.whichEye[f+i]));
        _mm_store_si128((__m128i*)&data->sample_id[i], _mm_set1_epi32(0));
        __m128i x_pixel = _mm_load_si128((__m128i*)&fragment.pixel[0][f+i]);
        __m128 fx = _mm_cvtepi32_ps(x_pixel);
        _mm_store_ps(&data->image[0][i], _mm_add_ps(_mm_mul_ps(fx, vec_xscale), vec_xoffset));
        __m128i y_pixel = _mm_load_si128((__m128i*)&fragment.pixel[1][f+i]);
        __m128 fy = _mm_cvtepi32_ps(y_pixel);
        _mm_store_ps(&data->image[1][i], _mm_add_ps(_mm_mul_ps(fy, vec_yscale), vec_yoffset));
        __m128i pixel_id = _mm_add_epi32(_mm_mullo_epi32(y_pixel, _mm_set1_epi32(ci.xres)), x_pixel);
        _mm_store_si128((__m128i*)&data->region_id[i], pixel_id);
      }
#else
      int e = 0;
#endif
      for(int i=e;i<size;i++){
        Real px = fragment.getX(f+i)*ci.xscale+ci.xoffset;
        Real py = fragment.getY(f+i)*ci.yscale+ci.yoffset;
        rays.setPixel(i, fragment.getWhichEye(f+i), px, py);
        rays.data->sample_id[i] = 0;
        rays.data->region_id[i] = fragment.getY(f+i) * ci.xres + fragment.getX(f+i);
      }
    }

    // Trace the rays.  The results will automatically go into the fragment
    context.sample_generator->setupPacket(context, rays);
    context.renderer->traceEyeRays(context, rays);

#if MANTA_SSE
    int e = size&(~3);
    for(int i=0;i<e;i+=4){
      _mm_store_ps(&fragment.color[0][f+i], _mm_load_ps(&raydata.color[0][i]));
      _mm_store_ps(&fragment.color[1][f+i], _mm_load_ps(&raydata.color[1][i]));
      _mm_store_ps(&fragment.color[2][f+i], _mm_load_ps(&raydata.color[2][i]));
      _mm_store_ps(&fragment.depth[f+i], _mm_load_ps(&raydata.minT[i]));
    }
#else
    int e = 0;
#endif
    for(int i=e;i<size;i++) {
      for ( int c = 0; c < Color::NumComponents; c++ )
        fragment.color[c][f+i] = raydata.color[c][i];
      fragment.depth[f+i] = raydata.minT[i];
    }
  }
}
