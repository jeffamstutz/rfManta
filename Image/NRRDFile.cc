
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


#include <Image/NRRDFile.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>

#include <Core/Exceptions/UnknownPixelFormat.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>

#include <teem/nrrd.h>

using namespace Manta;

extern "C" void writeNRRD( Image const *image, std::string const &file_name, int eye ) {
  // Create a nrrd to save the image.
  Nrrd *out_nrrd = nrrdNew();

  // Determine the resolution.
  bool stereo;
  int width, height;
  image->getResolution( stereo, width, height );
  
  // Three dimensions for images
  out_nrrd->dim = 3;

  int pixel_width;

  /////////////////////////////////////////////////////////////////////////////
  // Determine the pixel type and size.  Also swizzle the pixel data
  // into a new buffer, eliminating padding and rearranging the
  // color/alpha channels into a nrrd-friendly order.
  if (typeid(*image) == typeid(SimpleImage<RGB8Pixel>)) {
    const SimpleImage<RGB8Pixel> *si = dynamic_cast<const SimpleImage<RGB8Pixel> *>(image);

    pixel_width = 3;
    out_nrrd->type = nrrdTypeUChar;

    unsigned char *data = new unsigned char[pixel_width*width*height];
    for(int i=0; i<height; i++){
      for(int j=0; j<width; j++){
        const RGB8Pixel p = si->get(j,i,eye);

        data[pixel_width*width*i + pixel_width*j + 0] = p.r;
        data[pixel_width*width*i + pixel_width*j + 1] = p.g;
        data[pixel_width*width*i + pixel_width*j + 2] = p.b;
      }
    }

    out_nrrd->data = data;
  }

  else if (typeid(*image) == typeid(SimpleImage<RGBA8Pixel>)) {
    const SimpleImage<RGBA8Pixel> *si = dynamic_cast<const SimpleImage<RGBA8Pixel> *>(image);

    pixel_width = 4;
    out_nrrd->type = nrrdTypeUChar;    

    unsigned char *data = new unsigned char[pixel_width*width*height];
    for(int i=0; i<height; i++){
      for(int j=0; j<width; j++){
        const RGBA8Pixel p = si->get(j,i,eye);

        data[pixel_width*width*i + pixel_width*j + 0] = p.r;
        data[pixel_width*width*i + pixel_width*j + 1] = p.g;
        data[pixel_width*width*i + pixel_width*j + 2] = p.b;
        data[pixel_width*width*i + pixel_width*j + 3] = p.a;
      }
    }

    out_nrrd->data = data;
  }
  
  else if (typeid(*image) == typeid(SimpleImage<RGBfloatPixel>)) {
    const SimpleImage<RGBfloatPixel> *si = dynamic_cast<const SimpleImage<RGBfloatPixel> *>(image);

    pixel_width = 3;
    out_nrrd->type = nrrdTypeFloat;

    float *data = new float[pixel_width*width*height];
    for(int i=0; i<height; i++){
      for(int j=0; j<width; j++){
        const RGBfloatPixel p = si->get(j,i,eye);

        data[pixel_width*width*i + pixel_width*j + 0] = p.r;
        data[pixel_width*width*i + pixel_width*j + 1] = p.g;
        data[pixel_width*width*i + pixel_width*j + 2] = p.b;
      }
    }

    out_nrrd->data = data;
  }

  else if (typeid(*image) == typeid(SimpleImage<RGBAfloatPixel>)) {
    const SimpleImage<RGBAfloatPixel> *si = dynamic_cast<const SimpleImage<RGBAfloatPixel> *>(image);

    pixel_width = 4;
    out_nrrd->type = nrrdTypeFloat;

    float *data = new float[pixel_width*width*height];
    for(int i=0; i<height; i++){
      for(int j=0; j<width; j++){
        const RGBAfloatPixel p = si->get(j,i,eye);

        data[pixel_width*width*i + pixel_width*j + 0] = p.r;
        data[pixel_width*width*i + pixel_width*j + 1] = p.g;
        data[pixel_width*width*i + pixel_width*j + 2] = p.b;
        data[pixel_width*width*i + pixel_width*j + 3] = p.a;
      }
    }

    out_nrrd->data = data;
  }
  else {
    nrrdNix( out_nrrd );
    throw UnknownPixelFormat( "Unsupported image type for nrrd output." );
  }

  /////////////////////////////////////////////////////////////////////////////
	// Specify the size of the data.
  size_t sizes[3] = { pixel_width, width, height };
  nrrdAxisInfoSet_nva( out_nrrd, nrrdAxisInfoSize, sizes );

  /////////////////////////////////////////////////////////////////////////////
  // Filp the image.
  Nrrd *flipped = nrrdNew();
  if (nrrdFlip( flipped, out_nrrd, 2 )) {
    const char *reason = biffGetDone( NRRD );
    
    nrrdNuke( out_nrrd );
    nrrdNuke( flipped );
    throw OutputError( "Could not flip nrrd image: " + string( reason ) );
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Write the nrrd to disk.
  if (nrrdSave( file_name.c_str(), flipped, 0 )) {
    const char *reason = biffGetDone( NRRD );

    nrrdNuke( out_nrrd );
    nrrdNuke( flipped );
    throw OutputError( "Could not save image as nrrd: " + string( reason ) );
  }

  nrrdNuke( out_nrrd );
  nrrdNuke( flipped );
}

extern "C" Image *readNRRD( const std::string &file_name ) {

  ////////////////////////////////////////////////////////////////////////////
  // Attempt to open the file
	Nrrd *new_nrrd = nrrdNew();
	if (nrrdLoad( new_nrrd, file_name.c_str(), 0 )) {
		char *reason = biffGetDone( NRRD );
    
    nrrdNuke( new_nrrd );
    throw InputError( "Loading Nrrd Failed: " + string ( reason ) );
	}

	// Check that it has the right dimensions.
	if ((new_nrrd->dim != 3)) {

    nrrdNuke( new_nrrd );
		throw InputError( "nrrd must have three dimensions" );
	}
  
	/////////////////////////////////////////////////////////////////////////////
	// Obtain the data.
	int width  = new_nrrd->axis[1].size;
	int height = new_nrrd->axis[2].size;
  int pixel_dimension = new_nrrd->axis[0].size;
  unsigned char* data = static_cast<unsigned char*>(new_nrrd->data);
  
  /////////////////////////////////////////////////////////////////////////////
  // Determine what type of pixels to use.
  if (new_nrrd->type == nrrdTypeUChar) {
    // RGB Pixels
    if (pixel_dimension == 3) {
      SimpleImage<RGB8Pixel>* si = new SimpleImage<RGB8Pixel>( false, width, height );
      for ( int y = 0; y < height; ++y )
        for ( int x = 0; x < width; ++x ) {
          RGB8Pixel pixel;
          pixel.r = static_cast<unsigned char>(data[0]);
          pixel.g = static_cast<unsigned char>(data[1]);
          pixel.b = static_cast<unsigned char>(data[2]);
          si->set(pixel, x, (height-1-y), 0);
          data+=3;
        }
      nrrdNuke(new_nrrd);
      return si;
    }
    
    // RGBA Pixels
    else if (pixel_dimension == 4) {
      SimpleImage<RGBA8Pixel>* si = new SimpleImage<RGBA8Pixel>( false, width, height );
      for ( int y = 0; y < height; ++y )
        for ( int x = 0; x < width; ++x ) {
          RGBA8Pixel pixel;
          pixel.r = static_cast<unsigned char>(data[0]);
          pixel.g = static_cast<unsigned char>(data[1]);
          pixel.b = static_cast<unsigned char>(data[2]);
          pixel.a = static_cast<unsigned char>(data[3]);
          si->set(pixel, x, (height-1-y), 0);
          data+=4;
        }
      nrrdNuke(new_nrrd);
      return si;
    }

    else {
      nrrdNuke(new_nrrd);
      throw InputError("Unknown pixel dimension");
    }
  } else {
    nrrdNuke(new_nrrd);
    throw InputError("Unknown type for nrrd load.");
  }
}

extern "C" bool NRRDSupported() {
  return true;
}
