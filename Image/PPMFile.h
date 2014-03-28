#ifndef Manta_Image_PPMFile_h
#define Manta_Image_PPMFile_h

#include <string>

namespace Manta
{
  class Image;

  extern "C" void writePPM(Image const *image, std::string const &filename,
                           int which=0);
  extern "C" Image* readPPM(const std::string &filename);
}

#endif // Manta_Image_PPMFile_h
