#ifndef _MANTA_IMAGE_RGBEFILE_H_
#define _MANTA_IMAGE_RGBEFILE_H_

#include <string>

namespace Manta {
  class Image;

  extern "C" void writeRGBE(Image const *image, std::string const &filename,
                            int which=0);

  extern "C" Image* readRGBE(const std::string &filename);
};

#endif
