
#include <Image/CoreGraphicsFile.h>

#include <Core/Color/RGBColor.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>

#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>

#include <ApplicationServices/ApplicationServices.h>

#include <iostream>

using namespace Manta;

namespace /* anonymous */ {

  CGImageRef loadImage(const std::string& filename)
  {
    CFStringRef path = CFStringCreateWithCString(NULL, filename.c_str(), 0);
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, false);
    CGImageRef image = NULL;
    CGImageSourceRef image_source;
    CFDictionaryRef options = NULL;
    CFStringRef keys[2];
    CFTypeRef values[2];

    keys[0] = kCGImageSourceShouldCache;
    values[0] = (CFTypeRef)kCFBooleanTrue;
    keys[1] = kCGImageSourceShouldAllowFloat;
    values[1] = (CFTypeRef)kCFBooleanTrue;

    options = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2,
                                 &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

    image_source = CGImageSourceCreateWithURL(url, options);

    if(image_source == NULL) {
      throw InputError("Error loading CoreGraphics File: "+filename);
    }

    image = CGImageSourceCreateImageAtIndex(image_source, 0, NULL);

    CFRelease(image_source);

    return image;
  }

  CGContextRef createRGBABitmapContext(CGImageRef image, void* data)
  {
    CGContextRef context = NULL;
    CGColorSpaceRef color_space;
    int byte_count;
    int bytes_per_row;

    size_t w = CGImageGetWidth(image);
    size_t h = CGImageGetHeight(image);

    bytes_per_row = (w * 4 * sizeof(float));
    byte_count = bytes_per_row * h;

    // Load the sRGB profile from file since 10.4 does not support a direct sRGB profile
    // defaults to a generic RGB space if the file does not exist
    CGColorSpaceRef alternate = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CFURLRef icc_profile_path =
      CFURLCreateWithFileSystemPath( NULL,
                                     CFSTR("/System/Library/ColorSync/Profiles/sRGB Profile.icc"),
                                     kCFURLPOSIXPathStyle, false);
    CGDataProviderRef icc_profile = CGDataProviderCreateWithURL(icc_profile_path);

    // NOTE(boulos): According to this page
    // http://developer.apple.com/releasenotes/Cocoa/Foundation.html
    // apple has wisely decided to introduce CGFloat as a wrapper that
    // switches between float and double between 32-bit and 64-bit
    // applications. Because naturally, 64-bit builds want to use
    // doubles for everything...
    typedef CGFloat MantaCGFloat;

    const MantaCGFloat range[] = {0,1,0,1,0,1}; // min/max of the three components
    color_space = CGColorSpaceCreateICCBased( 3, range, icc_profile, alternate );

    if(color_space == NULL) {
      CGDataProviderRelease(icc_profile);
      throw InternalError("Could not create CoreGraphics bitmap context");
    }

    context = CGBitmapContextCreate(data,
                                    w, h,
                                    32,
                                    bytes_per_row,
                                    color_space,
                                    kCGImageAlphaNoneSkipLast |
                                    kCGBitmapFloatComponents  |
                                    kCGBitmapByteOrder32Host);


    CGDataProviderRelease(icc_profile);
    if(color_space == alternate) {
      CGColorSpaceRelease(alternate);
    } else {
      CGColorSpaceRelease(color_space);
      CGColorSpaceRelease(alternate);
    }

    return context;
  }
}

extern "C"
bool isCoreGraphics( const std::string& filename )
{
  // For some reason when trying to load a ppm file we can create the image
  // source but we can't load the actual data.  Either there's a bug in apple's
  // code (this was tested with OS X 10.5), we don't understand how the code
  // below should work, or the ppm file is not in the format apple was
  // expecting.  In any case, we explicitly check for this.
  if (filename.rfind(".ppm") == (filename.size() - 4))
    return false;

  // try to create an image source for the file
  CFStringRef path = CFStringCreateWithCString(NULL, filename.c_str(), 0);
  CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, false);
  CGImageSourceRef image_source;
  CFDictionaryRef options = NULL;
  CFStringRef keys[2];
  CFTypeRef values[2];

  keys[0] = kCGImageSourceShouldCache;
  values[0] = (CFTypeRef)kCFBooleanTrue;
  keys[1] = kCGImageSourceShouldAllowFloat;
  values[1] = (CFTypeRef)kCFBooleanTrue;

  options = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2,
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);

  image_source = CGImageSourceCreateWithURL(url, options);

  bool readable = (image_source ? true : false);

  if(image_source)
    CFRelease(image_source);

  return readable;
}

extern "C"
void writeCoreGraphics( const Image* image, const std::string &filename, int which=0 )
{
  throw OutputError("CoreGraphics File I/O does not currently support writing");
}

extern "C"
Image* readCoreGraphics( const std::string& filename )
{
  CGImageRef cg_image = loadImage(filename);

  if(cg_image == NULL) {
    throw InputError("Error reading image from CoreGraphics source: "+filename);
  }

  size_t width = CGImageGetWidth(cg_image);
  size_t height = CGImageGetHeight(cg_image);

  float* raw_pixels = new float[width*height*4];

  SimpleImage<RGBAfloatPixel>* image = new SimpleImage<RGBAfloatPixel>(false, width, height);

  CGContextRef context = createRGBABitmapContext(cg_image, raw_pixels);
  if(context == NULL) {
    throw InternalError("Could not create CoreGraphics image context");
  }

  CGRect rect = {{0,0},{width,height}};
  CGContextTranslateCTM(context,0,height);
  CGContextScaleCTM(context, 1.0, -1.0);
  CGContextDrawImage(context, rect, cg_image);

  // we must perform a pixel by pixel copy because of SimpleImage's padding scheme.
  RGBAfloatPixel* pixels = (RGBAfloatPixel*)raw_pixels;
  for(unsigned int j = 0; j < height; ++j) {
    for(unsigned int i = 0; i < width; ++i) {
      RGBAfloatPixel* p = &pixels[i + width*j];
      image->set(*p, i, j, 0);
    }
  }

  CGContextRelease(context);
  CGImageRelease(cg_image);

  delete[] raw_pixels;

  return image;
}

// Returns true if this reader is supported
extern "C"
bool CoreGraphicsSupported()
{
  return true;
}
