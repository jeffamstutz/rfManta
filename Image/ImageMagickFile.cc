
/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

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


#include <Image/ImageMagickFile.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>

#include <Core/Exceptions/UnknownPixelFormat.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>

#include <Magick++.h>

#include <iostream>

using namespace Manta;

#if MagickLibVersion < 0x642
using MagickLib::Quantum;
using MagickLib::MagickRealType;
#else
using MagickCore::Quantum;
using MagickCore::MagickRealType;
#endif

using std::cerr;

// This is bacward compatibility with previous versions of ImageMagick
#ifndef QuantumRange
#define QuantumRange MaxRGB
#endif

extern "C" void writeImageMagick( Image const *image,
                                  std::string const &file_name,
                                  int which )
{
  Magick::Image magick_image;

  // Copy image data over
  throw OutputError( "writeImageMagick currently not implemented");
//   magick_image.read(image->width, image->height, pixel_type,
//                     storage_type, image

  try {
    // Read a file into image object
    magick_image.write( file_name );
  } catch ( Magick::Exception& error_ ) {
    throw OutputError( "writeImageMagick::Error writing file:" + string(error_.what()) );
  }

}

extern "C" Image *readImageMagick( const std::string &file_name )
{
  Magick::Image magick_image;

  try {
    // Read a file into image object
    magick_image.read( file_name );
  } catch ( Magick::Warning& warning_ ) {
    cerr << "readImageMagick::Warning reading file:" + string(warning_.what()) << "\n";
  } catch ( Magick::Error& error_ ) {
    throw InputError( "readImageMagick::Error reading file:" + string(error_.what()) );
  }

  Magick::Geometry size = magick_image.size();
  int width = size.width();
  int height = size.height();

  // Copy image data over
  if (magick_image.matte()) {
    // RGBA pixels
    SimpleImage<RGBA8Pixel>* si = new SimpleImage<RGBA8Pixel>( false, width, height );
    for ( int y = 0; y < height; ++y )
      for ( int x = 0; x < width; ++x ) {
        RGBA8Pixel pixel;
        Magick::Color color = magick_image.pixelColor(x,y);
        if (QuantumDepth == 8) {
          pixel.r = color.redQuantum();
          pixel.g = color.greenQuantum();
          pixel.b = color.blueQuantum();
          pixel.a = color.alphaQuantum();
        } else {
          // Pixels need to be rescaled
          MagickRealType scale = (MagickRealType)(255)/QuantumRange;
          pixel.r = static_cast<unsigned char>(color.redQuantum()*scale);
          pixel.g = static_cast<unsigned char>(color.greenQuantum()*scale);
          pixel.b = static_cast<unsigned char>(color.blueQuantum()*scale);
          pixel.a = static_cast<unsigned char>(color.alphaQuantum()*scale);
        }
        si->set(pixel, x, (height-1-y), 0);
      }
    return si;
  } else {
    // RGB Pixels
    SimpleImage<RGB8Pixel>* si = new SimpleImage<RGB8Pixel>( false, width, height );
    for ( int y = 0; y < height; ++y )
      for ( int x = 0; x < width; ++x ) {
        RGB8Pixel pixel;
        Magick::Color color = magick_image.pixelColor(x,y);
        if (QuantumDepth == 8) {
          pixel.r = color.redQuantum();
          pixel.g = color.greenQuantum();
          pixel.b = color.blueQuantum();
        } else {
          // Pixels need to be rescaled
          MagickRealType scale = (MagickRealType)(255)/QuantumRange;
          pixel.r = static_cast<unsigned char>(color.redQuantum()*scale);
          pixel.g = static_cast<unsigned char>(color.greenQuantum()*scale);
          pixel.b = static_cast<unsigned char>(color.blueQuantum()*scale);
        }
        si->set(pixel, x, (height-1-y), 0);
      }
    return si;
  }
}

// Returns true if this reader is supported
extern "C" bool ImageMagickSupported()
{
  return true;
}
