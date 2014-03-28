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

#include <Model/Textures/ImageTexture.h>
#include <Interface/RayPacket.h>
#include <Interface/Image.h>

#include <Core/Color/Color.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>

#include <Image/CoreGraphicsFile.h>
#include <Image/ImageMagickFile.h>
#include <Image/NRRDFile.h>
#include <Image/RGBEFile.h>
#include <Image/TGAFile.h>
#include <Image/EXRFile.h>

namespace Manta {

  template class ImageTexture<Color>;

  // This will potentially throw a InputError exception if there was a
  // problem.
  ImageTexture<Color>* LoadColorImageTexture( const std::string& file_name,
                                              std::ostream* stream,
                                              bool linearize)
  {
    if (stream) (*stream) << "Trying to load "<<file_name<<"\n";
    if (linearize && stream) (*stream) << "Will linearize on input\n";
    Image *image = 0;

    // Load the image.
    // Try and see if it is a nrrd
    bool isNrrd = ( (file_name.rfind(".nrrd") == (file_name.size()-5)) ||
                    (file_name.rfind(".nhdr") == (file_name.size()-5))
                    );
    bool isRGBE = ( (file_name.rfind(".hdr") == (file_name.size() - 4) ||
                     file_name.rfind(".pic") == (file_name.size() - 4) ||
                     file_name.rfind(".rgbe") == (file_name.size() - 5)));


    if (isNrrd) {
      // Check to see if it is a nrrd before trying to read with
      // something else.  ImageMagick will choke on nrrds and throw an
      // exception.
      image = readNRRD( file_name );
      if (stream) (*stream) << "Read by readNRRD\n";
    } else if (isRGBE) {
      image = readRGBE(file_name);
      if (stream) (*stream) << "Read by readRGBE\n";
    } else if ( CoreGraphicsSupported() && isCoreGraphics(file_name) ) {
      image = readCoreGraphics(file_name);
      if (stream) (*stream) << "Read by readCoreGraphics\n";
    } else if ( isEXR( file_name ) && EXRSupported() ) {
      image = readEXR(file_name);
      if (stream) (*stream) << "Read by readEXR\n";
    } else if (ImageMagickSupported()) {
      image = readImageMagick( file_name );
      if (stream) (*stream) << "Read by readImageMagick\n";
    } else {
      // Try our hard coded image readers
      if (file_name.rfind(".tga") == (file_name.size()-4)) {
        // Try to read a tga file.
        image = readTGA( file_name );
        if (stream) (*stream) << "Read by readTGA\n";
      }

      else if (NRRDSupported()) {
        // Try reading the image using teem.
        image = readNRRD( file_name );
        if (stream) (*stream) << "Read by readNRRD\n";
      } else {
        throw InputError("Unsupported image format for image.");
      }
    }

    // Create the texture.
    ImageTexture<Color> *texture = new ImageTexture<Color>(image, linearize);

    // Free time image.
    delete image;

    return texture;

  }

} // end namespace Manta

