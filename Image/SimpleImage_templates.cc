#include <Image/SimpleImage.h>
#include <Image/Pixel.h>

using namespace Manta;

template class SimpleImage<RGB8Pixel>;
template class SimpleImage<RGBA8Pixel>;
template class SimpleImage<ABGR8Pixel>;
template class SimpleImage<ARGB8Pixel>;
template class SimpleImage<RGBfloatPixel>;
template class SimpleImage<RGBAfloatPixel>;

