
#include <Core/Color/Color.h>
#include <Core/Color/RegularColorMap.h>
#include <Core/Math/MinMax.h>

#include <fstream>
using std::ifstream;

#include <iostream>
using std::cerr;

#include <string>
using std::string;

using namespace Manta;

RegularColorMap::RegularColorMap(const char* filename, unsigned int size)
  : blended(size), new_blended(size)
{
  if (!fillColor(filename, colors)) {
    cerr<<"RegularColorMap::RegularColorMap("<<filename
         <<"):  Unable to load colormap from file, using default\n";
    fillColor(GreyScale, colors);
  }

  fillBlendedColors(blended);
  lookup.setResultsPtr(&blended);
}

RegularColorMap::RegularColorMap(Array1<Color>& colors, unsigned int size)
  : blended(size), colors(colors), new_blended(size)
{
  fillBlendedColors(blended);
  lookup.setResultsPtr(&blended);
}

RegularColorMap::RegularColorMap(unsigned int type, unsigned int size)
  : blended(size), new_blended(size)
{
  fillColor(type, colors);
  fillBlendedColors(blended);
  lookup.setResultsPtr(&blended);
}

void RegularColorMap::fillColor(unsigned int type, Array1<Color> &colors)
{
  // Make sure we are dealing with a clean array
  colors.remove_all();

  // Add colors base on color map type
  switch (type) {
  case InvRainbow:
    colors.add(Color(RGB(0, 0, 1)));
    colors.add(Color(RGB(0, 0.40000001, 1)));
    colors.add(Color(RGB(0, 0.80000001, 1)));
    colors.add(Color(RGB(0, 1, 0.80000001)));
    colors.add(Color(RGB(0, 1, 0.40000001)));
    colors.add(Color(RGB(0, 1, 0)));
    colors.add(Color(RGB(0.40000001, 1, 0)));
    colors.add(Color(RGB(0.80000001, 1, 0)));
    colors.add(Color(RGB(1, 0.91764706, 0)));
    colors.add(Color(RGB(1, 0.80000001, 0)));
    colors.add(Color(RGB(1, 0.40000001, 0)));
    colors.add(Color(RGB(1, 0, 0)));
    break;
  case Rainbow:
    colors.add(Color(RGB(1, 0, 0)));
    colors.add(Color(RGB(1, 0.40000001, 0)));
    colors.add(Color(RGB(1, 0.80000001, 0)));
    colors.add(Color(RGB(1, 0.91764706, 0)));
    colors.add(Color(RGB(0.80000001, 1, 0)));
    colors.add(Color(RGB(0.40000001, 1, 0)));
    colors.add(Color(RGB(0, 1, 0)));
    colors.add(Color(RGB(0, 1, 0.40000001)));
    colors.add(Color(RGB(0, 1, 0.80000001)));
    colors.add(Color(RGB(0, 0.80000001, 1)));
    colors.add(Color(RGB(0, 0.40000001, 1)));
    colors.add(Color(RGB(0, 0, 1)));
    break;
  case InvBlackBody:
    colors.add(Color(RGB(1, 1, 1)));
    colors.add(Color(RGB(1, 1, 0.70588237)));
    colors.add(Color(RGB(1, 0.96862745, 0.47058824)));
    colors.add(Color(RGB(1, 0.89411765, 0.3137255)));
    colors.add(Color(RGB(1, 0.80000001, 0.21568628)));
    colors.add(Color(RGB(1, 0.63921571, 0.078431375)));
    colors.add(Color(RGB(1, 0.47058824, 0)));
    colors.add(Color(RGB(0.90196079, 0.27843139, 0)));
    colors.add(Color(RGB(0.78431374, 0.16078432, 0)));
    colors.add(Color(RGB(0.60000002, 0.070588239, 0)));
    colors.add(Color(RGB(0.40000001, 0.0078431377, 0)));
    colors.add(Color(RGB(0.20392157, 0, 0)));
    colors.add(Color(RGB(0, 0, 0)));
    break;
  case BlackBody:
    colors.add(Color(RGB(0, 0, 0)));
    colors.add(Color(RGB(0.20392157, 0, 0)));
    colors.add(Color(RGB(0.40000001, 0.0078431377, 0)));
    colors.add(Color(RGB(0.60000002, 0.070588239, 0)));
    colors.add(Color(RGB(0.78431374, 0.16078432, 0)));
    colors.add(Color(RGB(0.90196079, 0.27843139, 0)));
    colors.add(Color(RGB(1, 0.47058824, 0)));
    colors.add(Color(RGB(1, 0.63921571, 0.078431375)));
    colors.add(Color(RGB(1, 0.80000001, 0.21568628)));
    colors.add(Color(RGB(1, 0.89411765, 0.3137255)));
    colors.add(Color(RGB(1, 0.96862745, 0.47058824)));
    colors.add(Color(RGB(1, 1, 0.70588237)));
    colors.add(Color(RGB(1, 1, 1)));
    break;
  case GreyScale:
    colors.add(Color(RGB(0,0,0)));
    colors.add(Color(RGB(1,1,1)));
    break;
  case InvGreyScale:
    colors.add(Color(RGB(1,1,1)));
    colors.add(Color(RGB(0,0,0)));
    break;
  case InvRainbowIso:
    colors.add(Color(RGB(0.528, 0.528, 1.0)));
    colors.add(Color(RGB(0.304, 0.5824, 1.0)));
    colors.add(Color(RGB(0.0, 0.6656, 0.832)));
    colors.add(Color(RGB(0.0, 0.712, 0.5696)));
    colors.add(Color(RGB(0.0, 0.744, 0.2976)));
    colors.add(Color(RGB(0.0, 0.76, 0.0)));
    colors.add(Color(RGB(0.304, 0.76, 0.0)));
    colors.add(Color(RGB(0.5504, 0.688, 0.0)));
    colors.add(Color(RGB(0.68, 0.624, 0.0)));
    colors.add(Color(RGB(0.752, 0.6016, 0.0)));
    colors.add(Color(RGB(1.0, 0.5008, 0.168)));
    colors.add(Color(RGB(1.0, 0.424, 0.424)));
    break;
  default:
    cerr<<"RegularColorMap::fillColor(type="<<type
        <<"):  Invalid type, using gray scale\n";
    colors.add(Color(RGB(1,1,1)));
    colors.add(Color(RGB(0,0,0)));
  }
}

bool RegularColorMap::fillColor(const char* file, Array1<Color>& colors)
{
  const char *me="RegularColorMap::fillColor(file)";
  Array1<Color> new_colors;
  ifstream infile(file);
  if (!infile) {
    cerr<<me<<":  Color map file, "<<file<<", cannot be opened for reading\n";
    return false;
  }

  float r=0;
  infile>>r;
  float max=r;
  do {
    // Slurp up the colors
    float g=0;
    float b=0;
    infile>>g>>b;

    if (r>max)
      max=r;
    if (g>max)
      max=g;
    if (b>max)
      max=b;

    new_colors.add(Color(RGB(r,g,b)));
    infile>>r;
  } while(infile);

  if (max>1) {
    cerr<<me<<":  Renormalizing colors for range of 0 to 255\n";
    Real inv255=1/static_cast<Real>(255);
    for (int i=0; i < new_colors.size(); ++i)
      new_colors[i]=new_colors[i] * inv255;
  }

  // Copy the contents of new_colors to colors
  colors=new_colors;

  return true;
}

void RegularColorMap::fillBlendedColors(Array1<Color>& blended)
{
  ScalarTransform1D<int, Color> my_lookup;
  my_lookup.setResultsPtr(&colors);
  my_lookup.scale(0, blended.size() - 1);

  for (int i=0; i < blended.size(); ++i)
    blended[i]=my_lookup.interpolate(i);
}

unsigned int RegularColorMap::parseType(const char* type) {
  const string type_string(type);
  if (type_string == "InvRainbowIso" || type_string == "invrainbowiso" ||
      type_string == "iril")
    return InvRainbowIso;
  else if (type_string == "InvRainbow" || type_string == "invrainbow" ||
           type_string == "ir")
    return InvRainbow;
  else if (type_string == "Rainbow" || type_string == "rainbow" ||
           type_string == "r")
    return Rainbow;
  else if (type_string == "InvBlackBody" || type_string == "invblackbody" ||
           type_string == "ib")
    return InvBlackBody;
  else if (type_string == "BlackBody" || type_string == "blackbody" ||
           type_string == "b")
    return BlackBody;
  else if (type_string == "InvGreyScale" || type_string == "invgrayscale" ||
           type_string == "ig")
    return InvGreyScale;
  else if (type_string == "GreyScale" || type_string == "grayscale" ||
           type_string == "g")
    return GreyScale;

  return Unknown;
}
