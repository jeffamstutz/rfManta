
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
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Util/NotFinished.h>
#include <Image/NullImage.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>
#include <Image/TGAFile.h>
#include <fstream>

using namespace std;
using namespace Manta;

static void writePixel(ofstream& out, unsigned char r, unsigned char g,
                       unsigned char b)
{
  out.put( b );
  out.put( g );
  out.put( r );
}

static void writePixel(ofstream& out, unsigned char r, unsigned char g,
                       unsigned char b, unsigned char a)
{
  out.put( b );
  out.put( g );
  out.put( r );
  out.put( a );
}

static void writePixel(ofstream& out, float r, float g, float b)
{
  b = b < 0.0f ? 0.0f : b;
  b = b > 1.0f ? 1.0f : b;
  out.put( static_cast< unsigned char >( b * 255.0f ) );
  g = g < 0.0f ? 0.0f : g;
  g = g > 1.0f ? 1.0f : g;
  out.put( static_cast< unsigned char >( g * 255.0f ) );
  r = r < 0.0f ? 0.0f : r;
  r = r > 1.0f ? 1.0f : r;
  out.put( static_cast< unsigned char >( r * 255.0f ) );
}

static void writePixel(ofstream& out, float r, float g, float b, float a)
{
  b = b < 0.0f ? 0.0f : b;
  b = b > 1.0f ? 1.0f : b;
  out.put( static_cast< unsigned char >( b * 255.0f ) );
  g = g < 0.0f ? 0.0f : g;
  g = g > 1.0f ? 1.0f : g;
  out.put( static_cast< unsigned char >( g * 255.0f ) );
  r = r < 0.0f ? 0.0f : r;
  r = r > 1.0f ? 1.0f : r;
  out.put( static_cast< unsigned char >( r * 255.0f ) );
  a = a < 0.0f ? 0.0f : a;
  a = a > 1.0f ? 1.0f : a;
  out.put( static_cast< unsigned char >( a * 255.0f ) );
}

template<class PType>
void writeAllPixels(ofstream& out, Image const* image, int eye)
{
  bool stereo;
  int xres, yres;
  image->getResolution( stereo, xres, yres );

  const SimpleImage< PType >* si =
    dynamic_cast< const SimpleImage< PType > * >( image );
  PType const *buffer = si->getRawPixels( eye );
  for ( int y = 0; y < yres; ++y ) {
    PType const* row = buffer + (y * si->getRowLength());
    for ( int x = 0; x < xres; ++x ) {
      writePixel(out, row->r, row->g, row->b);
      ++row;
    }
  }
}

template<class PType>
void writeAllPixelsAlpha(ofstream& out, Image const* image, int eye)
{
  bool stereo;
  int xres, yres;
  image->getResolution( stereo, xres, yres );

  const SimpleImage< PType >* si =
    dynamic_cast< const SimpleImage< PType > * >( image );
  PType const *buffer = si->getRawPixels( eye );
  for ( int y = 0; y < yres; ++y ) {
    PType const* row = buffer + (y * si->getRowLength());
    for ( int x = 0; x < xres; ++x ) {
      writePixel(out, row->r, row->g, row->b, row->a);
      ++row;
    }
  }
}

void writePixelDescription(ofstream& out) {
  out.put( 24 );  // Pixel depth: 24bpp
  out.put( 0 );   // Image descriptor: 0 bits of alpha, bottom-left origin
}

void writePixelDescriptionAlpha(ofstream& out) {
  out.put( 32 );  // Pixel depth: 32bpp
  out.put( 8 );   // Image descriptor: 8 bits of alpha, bottom-left origin
}

  static inline void write16bit(
    ofstream &out,
    int const value )
  {
    out.put( value & 0xFF );
    out.put( ( value >> 8 ) & 0xFF );
  }

  // This function should be removed, because it isn't used.
  static inline void write32bit(
    ofstream &out,
    int const value )
  {
    out.put( value & 0xFF );
    out.put( ( value >> 8 ) & 0xFF );
    out.put( ( value >> 16 ) & 0xFF );
    out.put( ( value >> 24 ) & 0xFF );
  }

  void Manta::writeTGA(
    Image const *image,
    string const &filename,
    int const eye )
  {
    ofstream out( filename.c_str(), ios::out | ios::binary );
    if ( !out )
      throw InternalError( "Couldn't open TGA file for writing: " + filename);
    out.put( 0 );                 // ID length: 0
    out.put( 0 );                 // Color map type: None
    out.put( 2 );                 // Image type: uncompressed, true-color image
    write16bit( out, 0 );         // Color map first entry index
    write16bit( out, 0 );         // Color map length
    out.put( 0 );                 // Color map entry size
    write16bit( out, 0 );         // X-origin of image (left)
    write16bit( out, 0 );         // Y-origin of image (bottom)
    bool stereo;
    int xres, yres;
    image->getResolution( stereo, xres, yres );
    write16bit( out, xres );      // Image width
    write16bit( out, yres );      // Image height

    if ( typeid( *image ) == typeid( SimpleImage< RGBA8Pixel > ) ) {
      // RGBA8
      writePixelDescriptionAlpha(out);
      writeAllPixelsAlpha<RGBA8Pixel>(out, image, eye);
    } else if ( typeid( *image ) == typeid( SimpleImage< ABGR8Pixel > ) ) {
      // ABGR8
      writePixelDescriptionAlpha(out);
      writeAllPixelsAlpha<ABGR8Pixel>(out, image, eye);
    } else if ( typeid( *image ) == typeid( SimpleImage< ARGB8Pixel > ) ) {
      // ARGB8
      writePixelDescriptionAlpha(out);
      writeAllPixelsAlpha<ARGB8Pixel>(out, image, eye);
    } else if ( typeid( *image ) == typeid( SimpleImage< BGRA8Pixel > ) ) {
      // BGRA8
      writePixelDescriptionAlpha(out);
      writeAllPixelsAlpha<BGRA8Pixel>(out, image, eye);
    } else if ( typeid( *image ) == typeid( SimpleImage< RGB8Pixel > ) ) {
      // RGB8
      writePixelDescription(out);
      writeAllPixels<RGB8Pixel>(out, image, eye);
    } else if ( typeid( *image ) == typeid( SimpleImage< RGBfloatPixel > ) ) {
      // RGBfloat
      writePixelDescription(out);
      writeAllPixels<RGBfloatPixel>(out, image, eye);
    } else if ( typeid( *image ) == typeid( SimpleImage< RGBAfloatPixel > ) ) {
      // RGBAfloat
      writePixelDescriptionAlpha(out);
      writeAllPixelsAlpha<RGBAfloatPixel>(out, image, eye);
    } else
    {
      writePixelDescription(out);
      // Do slow crappy conversion
      cerr << __FILE__ <<":"<<__LINE__<<":Unknown image/pixel type in writeTGA(), doing slow conversion.\n";
      Fragment fragment(Fragment::UnknownShape);
      for ( int y = 0; y < yres; ++y )
        for ( int x = 0; x < xres; x+=Fragment::MaxSize ) {
          int xend = x + Fragment::MaxSize;
          if (xend > xres) xend = xres;
          fragment.setConsecutiveX(x, xend, y, eye);
          image->get(fragment);
          // Now write out the fragments
          for(int frag = fragment.begin(); frag < fragment.end(); ++frag) {
            RGBColor pixel(fragment.getColor(frag).convertRGB());
            writePixel(out, pixel.r(), pixel.g(), pixel.b());
          }
        }
      //      throw InternalError( "Unknown image/pixel type in writeTGA()", __FILE__, __LINE__  );
    }
  }

  static inline int read16bit(
    ifstream &in )
  {
    int byte_1 = in.get();
    int byte_2 = in.get();
    return byte_1 | ( byte_2 << 8 );
  }

  // This function should be removed, because it isn't used.
  static inline int read32bit(
    ifstream &in )
  {
    int byte_1 = in.get();
    int byte_2 = in.get();
    int byte_3 = in.get();
    int byte_4 = in.get();
    return byte_1 | ( byte_2 << 8 ) | ( byte_3 << 16 ) | ( byte_4 << 24 );
  }

  Image* Manta::readTGA(
    string const &filename )
  {
    ifstream in( filename.c_str(), ios::in | ios::binary );
    if ( !in )
      throw InternalError( "Couldn't open TGA file for reading: " + filename);
    int id_length = in.get();
    int color_map_type = in.get();
    if ( color_map_type != 0 )
      throw InternalError( "Color map TGA files currently unsupported");
    int image_type = in.get();
    if ( image_type != 2 )
      throw InternalError( "Only uncompressed true-color TGA files currently supported");
#if 0
    int color_map_first_entry_index = read16bit( in );
    int color_map_length = read16bit( in );
    int color_map_entry_size = in.get();
    int x_origin = read16bit( in );
    int y_origin = read16bit( in );
#else
    // Instead of storing these, we'll just pull them off (to avoid
    // warnings and yet still produce correct behaviour)
    read16bit(in);
    read16bit(in);
    in.get();
    read16bit(in);
    read16bit(in);
#endif
    int width = read16bit( in );
    int height = read16bit( in );
    int pixel_depth = in.get();
    int image_descriptor = in.get();
    // Check image_descriptor
    if (image_descriptor & (192)) {
      // Bits 6-7 are for strange interleaving formats.  If they are
      // set, we should blow up.
      throw InternalError( "Only non interleaved TGA files currently supported");
    }
    in.ignore( id_length );
    bool flip_image = !!(image_descriptor & 32); // bit 5 says flip in y
    if ( pixel_depth == 24 && ( image_descriptor & 15 ) == 0 ) {
      SimpleImage< RGB8Pixel > *si = new SimpleImage< RGB8Pixel >( false, width, height );
      for ( int y = 0; y < height; ++y )
        for ( int x = 0; x < width; ++x ) {
          RGB8Pixel pixel;
          pixel.b = static_cast<unsigned char>(in.get());
          pixel.g = static_cast<unsigned char>(in.get());
          pixel.r = static_cast<unsigned char>(in.get());
          si->set(pixel, x, flip_image ? (height-1-y) : y, 0);
        }
      return si;
    } else if ( pixel_depth == 32 && ( image_descriptor & 15 ) == 8 ) {
      SimpleImage< RGBA8Pixel > *si = new SimpleImage< RGBA8Pixel >( false, width, height );
      for ( int y = 0; y < height; ++y )
        for ( int x = 0; x < width; ++x ) {
          RGBA8Pixel pixel;
          pixel.b = static_cast<unsigned char>(in.get());
          pixel.g = static_cast<unsigned char>(in.get());
          pixel.r = static_cast<unsigned char>(in.get());
          pixel.a = static_cast<unsigned char>(in.get());
          si->set(pixel, x, flip_image ? (height-1-y) : y, 0);
        }
      return si;
    }
    throw InternalError( "Unhandled pixel depth and alpha for TGA files");
  }

