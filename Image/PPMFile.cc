#include <Core/Exceptions/UnknownPixelFormat.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>
#include <Image/PPMFile.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>

#include <fstream>
using std::ifstream;
using std::ofstream;

#include <iostream>
using std::cerr;

#include <string>
using std::string;

using namespace Manta;

void eatPPMCommentsWhitespace(ifstream& str)
{
  char c;
  str.get(c);
  while (true) {
    if (c==' '||c=='\t'||c=='\n') {
      str.get(c);
      continue;
    } else if (c=='#') {
      str.get(c);
      while (c!='\n')
        str.get(c);
    } else {
      str.unget();
      break;
    }
  }
}

extern "C" void writePPM(Image const* image, string const& filename,
                         int which)
{
  // Validate image type
  if (typeid(*image) != typeid(SimpleImage<RGB8Pixel>)) {
    throw UnknownPixelFormat("Unsupported image type for PPM output");
  }

  // Open PPM file and write header
  ofstream out(filename.c_str());
  if (!out.is_open())
    throw InputError("Failed to open \"" + filename + "\" for writing\n");

  out<<"P6\n# Binary PPM image created with Manta\n";

  bool stereo;
  int width, height;
  image->getResolution(stereo, width, height);
  out<<width<<" "<<height<<"\n";
  out<<"255\n";

  // Write the data
  RGB8Pixel* pixels=dynamic_cast<SimpleImage<RGB8Pixel> const *>(image)->getRawPixels(0);
  RGBColor rgb;
  unsigned char color[3];
  // TODO(boulos): Fix getResolution to be unsigned everywhere...
  for (int v=0; v<height; ++v) {
    for (int u=0; u<width; ++u) {
      convertToRGBColor(rgb, pixels[v*width + u]);
      color[0]=static_cast<unsigned char>(rgb.r()*255);
      color[1]=static_cast<unsigned char>(rgb.g()*255);
      color[2]=static_cast<unsigned char>(rgb.b()*255);

      out.write(reinterpret_cast<char*>(color), 3*sizeof(unsigned char));
    }
  }
}

extern "C" Image* readPPM(const string& filename)
{
  // Open PPM file
  ifstream in(filename.c_str());
  if (!in.is_open())
    throw InputError("Failed to open \"" + filename + "\" for reading\n");

  // Read magic number
  string magic;
  in>>magic;
  if (magic != "P6" && magic != "P3" && magic != "P5" && magic != "P2")
    throw InputError("Unrecognized PPM magic \"" + magic + "\"\n");

  // Read resolution and max value
  unsigned int width, height, maximum;
  eatPPMCommentsWhitespace(in);
  in>>width>>height;
  eatPPMCommentsWhitespace(in);
  in>>maximum;

  // Eat only the trailing '\n' (if it's there)
  {
    char c;
    while (true) {
      in.get(c);
      if (c=='\n')
        break;
    }
  }

  // Create an image
  Image* image=new SimpleImage<RGB8Pixel>(false, width, height);
  RGB8Pixel* pixels=dynamic_cast<SimpleImage<RGB8Pixel> const *>(image)->getRawPixels(0);
  Real invmax=1/static_cast<Real>(maximum);
  RGB8Pixel pixel;

  // Read the data
  if (magic=="P6") {
    unsigned char color[3];
    for (unsigned v=0; v<height; ++v) {
      for (unsigned u=0; u<width; ++u) {
        in.read(reinterpret_cast<char*>(color), 3*sizeof(unsigned char));
        convertToPixel(pixel, RGBColor(color[0]*invmax, color[1]*invmax,
                                       color[2]*invmax));
        pixels[v*width + u]=pixel;
      }
    }
  } else if (magic=="P3") {
    int r, g, b;
    for (unsigned v=0; v<height; ++v) {
      for (unsigned u=0; u<width; ++u) {
        in>>r>>g>>b;
        convertToPixel(pixel, RGBColor(r*invmax, g*invmax, b*invmax));
        pixels[v*width + u]=pixel;
      }
    }
  } else if (magic=="P5") {
    int gray=0;
    for (unsigned v=0; v<height; ++v) {
      for (unsigned u=0; u<width; ++u) {
        if (maximum < 256)
          in.read(reinterpret_cast<char*>(gray), 1*sizeof(unsigned char));
        else
          in.read(reinterpret_cast<char*>(gray), 2*sizeof(unsigned char));
        convertToPixel(pixel, RGBColor(gray*invmax, gray*invmax,
                                       gray*invmax));
        pixels[v*width + u]=pixel;
      }
    }
  } else if (magic=="P2") {
    int gray;
    for (unsigned v=0; v<height; ++v) {
      for (unsigned u=0; u<width; ++u) {
        in>>gray;
        convertToPixel(pixel, RGBColor(gray*invmax, gray*invmax, gray*invmax));
        pixels[v*width + u]=pixel;
      }
    }
  } else {
    throw InputError("Unrecognized PPM magic \"" + magic + "\"\n");
  }

  return image;
}
