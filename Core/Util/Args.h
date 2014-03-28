
#ifndef Manta_Core_Args_h
#define Manta_Core_Args_h

#include <MantaTypes.h>
#include <string>
#include <vector>
#include <sstream>

namespace Manta {
  class Vector;

  using namespace std;
  bool getStringArg(size_t& i, const vector<string>&, string& result);
  bool getDoubleArg(size_t& i, const vector<string>&, double& result);
  bool getIntArg(size_t& i, const vector<string>&, int& result);
  bool getLongArg(size_t& i, const vector<string>&, long& result);
  bool getResolutionArg(size_t& i, const vector<string>&, int& xres, int& yres);
  bool getVectorArg(size_t& i, const vector<string>&, Vector& p);
  void parseSpec(const string& spec, string& name, vector<string>& args);

  // This parsers out a color argument from the commandline.
  // The format can be one of three things:
  //
  // 1. colorName      - the color is looked up in the ColorDB.
  // 2. RGB8 r g b     - RGB components in [0,255]
  // 3. RGBfloat r g b - RGB components in [0,1]
  bool getColorArg(size_t& i, const vector<string>&, Color& color);
  
  // Parse a resolution value from a string.
  bool getResolutionArg(const string& arg, int& xres, int& yres);

  template<typename T>
  bool parseValue(const string& arg, T& result) {
    istringstream in(arg);
    T temp;
    // Attempt to pull in the value
    in >> temp;
    if (!in.fail()) {
      // Everything was OK, so assign it to the return parameter.
      result = temp;
      return true;
    } else {
      return false;
    }
  }    
  // Generic version that grabs an argument of type T.
  template<typename T>
  bool getArg(size_t& i, const vector<string>& args, T& result) {
    // Check to make sure args[i] exists.
    if(++i >= args.size()) {
      i--;
      return false;
    }
    if (parseValue(args[i], result)) {
      return true;
    } else {
      i--;
      return false;
    }
  }
        
  // This will separate input into whitespace separated arguments.  If
  // you want a single argument to contain whitespace you must
  // encapsulate the argument with single quotes.
  void parseArgs(const string& input, vector<string>& args);
}

#endif
