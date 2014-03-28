
#include <Core/Color/RGBColor.h>
#include <Core/Color/ColorDB.h>

#include <sstream>
#include <string>
#include <iomanip>
#include <ctype.h>

using namespace Manta;
using namespace std;

string RGBColor::toString() const {
  ostringstream out;
  out << "RGBColor("<<data[0]<<", "<<data[1]<<", "<<data[2]<<")";
  return out.str();
}

std::string RGBColor::writePersistentString() const
{
  // First, see if we can approximate the color with an rgb that is divisible by 256 so that
  // we can use the HTML hex notation
  bool hexworks = true;
  int hex[3];
  for(int i=0;i<3;i++){
    if(data[i] < 0 || data[i] > 1){
      hexworks = false;
    } else {
      hex[i] = (int)(data[0]*255 + 0.5);
      if(hex[i]/255.0 != data[i])
        hexworks = false;
    }
  }
  ostringstream rep;
  if(hexworks){
    rep << "#";
    for(int i=0;i<3;i++)
      rep << setfill('0') << setbase(16) << setw(2) << hex[i];
  } else {
    rep << "rgb: " << data[0] << " " << data[1] << " " << data[2];
  }
  return rep.str();
}

static inline int hexvalue(char c){
  if(c >= '0' && c <= '9')
    return c-'0';
  else if(c >= 'a' && c <= 'f')
    return c-'a';
  else if(c >= 'A' && c <= 'F')
    return c-'A';
  else
    return 0;
}

bool RGBColor::readPersistentString(const std::string& str)
{
  if(str[0] == '#'){
    // Parse html style hex string
    std::string::size_type idx = 1;
    while(idx < str.size() && ((str[idx] >= '0' && str[idx] <= '9') || (str[idx] >= 'a' && str[idx] <= 'f') || (str[idx] >= 'A' && str[idx] <= 'F')))
      idx++;

    if(idx == 7){
      data[0] = (hexvalue(str[1])<<4) + hexvalue(str[2]);
      data[1] = (hexvalue(str[3])<<4) + hexvalue(str[4]);
      data[2] = (hexvalue(str[5])<<4) + hexvalue(str[6]);
    } else if(idx == 4){
      data[0] = (hexvalue(str[1])<<4) + hexvalue(str[1]);
      data[1] = (hexvalue(str[2])<<4) + hexvalue(str[2]);
      data[2] = (hexvalue(str[3])<<4) + hexvalue(str[3]);
    } else {
      return false;
    }
    while(idx < str.size() && !isspace(str[idx]))
      idx++;
    if(idx != str.size())
      return false;

    data[0] /= 255.;
    data[1] /= 255.;
    data[2] /= 255.;
    return true;
  } else if(str.substr(0, 4) == "rgb:"){
    // RGB triple
    std::istringstream in(str.substr(4));
    in >> data[0] >> data[1] >> data[2];
    if(!in)
      return false;
    return true;
  } else if(ColorDB::getNamedColor(*this, str)){
    // A named color
    return true;
  } else {
    // Try to parse as an rgb triple
    std::istringstream in(str);
    in >> data[0] >> data[1] >> data[2];
    if(!in)
      return false;
    return true;
  }
}
