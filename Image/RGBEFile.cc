
#include <Image/RGBEFile.h>
#include <Core/Util/rgbe.h>

#include <Image/Pixel.h>
#include <Image/SimpleImage.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Exceptions/OutputError.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace Manta;

Image* Manta::readRGBE(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
      throw InternalError("Couldn't open RGBE file for reading: " + filename);
    }

    int width, height;
    int error = RGBE_ReadHeader(file, &width, &height, NULL);
    if ( error != RGBE_RETURN_SUCCESS ) {
      fclose(file);
      throw InternalError("RGBE_ReadHeader failed for file: " + filename);
    }

    unsigned int w = (unsigned int)width;
    unsigned int h = (unsigned int)height;

    float* image = new float[3*w*h];
    if (!image) {
      fclose(file);
      throw InternalError("RGBEFile::readRGBE failed to allocate temporary \
          raster for file: " + filename);
    }

    error = RGBE_ReadPixels_RLE(file, image, w, h);
    if ( error != RGBE_RETURN_SUCCESS ) {
      delete[] image;
      fclose(file);
      throw InternalError("RGBE_ReadPixels_RLE failed for file: " + filename);
    }

    fclose(file);

    SimpleImage< RGBfloatPixel > *result = 
      new SimpleImage< RGBfloatPixel >( false, width, height );
    if (!result) {
      delete[] image;
      throw InternalError("RGBEFile::readRGBE failed to allocate Image" + filename );

    }

    for (unsigned int y = 0; y < h; y++ ) {
        for (unsigned int x = 0; x < w; x++) {
            unsigned int index = ((h-y-1)*w + x)*3;
            RGBfloatPixel pixel;
            pixel.r = image[index+0];
            pixel.g = image[index+1];
            pixel.b = image[index+2];
            result->set(pixel, x, y, 0);
        }
    }
    delete[] image;
    return result;
}

void Manta::writeRGBE(Image const *image, std::string const &filename,
                      int which) 
{
    FILE* out = fopen( filename.c_str(), "wb" );
    if ( !out )
      throw OutputError( " RBGEIO::writeImage(" + filename + 
          ") failed to open file." );

    // Determine the resolution.
    bool stereo;
    int w, h;
    image->getResolution( stereo, w, h );

    float* raster = new(std::nothrow)float[3*w*h];
    if (!raster) {
      fclose(out);
      throw InternalError("RGBEIO::writeImage failed to allocate raster.");
    }

    const SimpleImage<RGBAfloatPixel>* si = 
      dynamic_cast<const SimpleImage<RGBAfloatPixel> *>(image);
    if ( !si ) {
      throw InternalError("RGBEIO::writeImage only Simple RGBAfloatPixel \
          images supported.");
    }

    
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        const RGBAfloatPixel p = si->get(x, y, which);
        int index = ( (h-y-1)*w + x ) * 3;

        raster[index+0] = p.r;
        raster[index+1] = p.g;
        raster[index+2] = p.b;
      }
    }

    int error_code = RGBE_WriteHeader(out, w, h, NULL);
    if (error_code != RGBE_RETURN_SUCCESS)
    {
      delete[] raster;
      fclose(out);
      throw OutputError( " RBGEIO::writeImage(" + filename + 
          ") failed to write header." );
    }

    error_code = RGBE_WritePixels(out, raster, w*h);
    if (error_code != RGBE_RETURN_SUCCESS)
    {
      delete[] raster;
      fclose(out);
      throw OutputError( " RBGEIO::writeImage(" + filename + 
          ") failed to write pixels." );
    }

    delete[] raster;
    fclose(out);
}

