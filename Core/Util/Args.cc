
#include <Core/Util/Args.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Geometry/Vector.h>
#include <Core/Color/ColorDB.h>
#include <cstdlib>

using namespace std;

namespace Manta {
  bool getStringArg(size_t& i, const vector<string>& args, string& result)
  {
    if(++i >= args.size()){
      result="";
      i--;
      return false;
    } else {
      result=args[i];
      return true;
    }
  }

  bool getLongArg(size_t& i, const vector<string>& args, long& result)
  {
    return getArg<long>(i, args, result);
  }

  bool getIntArg(size_t& i, const vector<string>& args, int& result)
  {
    return getArg<int>(i, args, result);
  }

  bool getDoubleArg(size_t& i, const vector<string>& args, double& result)
  {
    return getArg<double>(i, args, result);
  }

  // Parse an argument of the form NxM, where N and M are integers
  bool getResolutionArg(size_t& i, const vector<string>& args, int& xres, int& yres)
  {
    if(++i >= args.size()){
      i--;
      return false;
    } else {
      if (getResolutionArg(args[i], xres, yres)) {
        return true;
      } else {
        i--;
        return false;
      }
    }
  }

  bool getResolutionArg(const string& arg, int& xres, int& yres)
  {
    char* ptr = const_cast<char*>(arg.c_str());
    int tmpxres = (int)strtol(ptr, &ptr, 10);
    if(ptr == arg.c_str() || *ptr != 'x'){
      return false;
    }
    char* oldptr = ++ptr; // Skip the x
    int tmpyres = (int)strtol(ptr, &ptr, 10);
    if(ptr == oldptr || *ptr != 0){
      return false;
    }
    xres = tmpxres;
    yres = tmpyres;
    return true;
  }

  bool getVectorArg(size_t& i, const vector<string>& args, Vector& v)
  {
    // NOTE(boulos): Quieting warnings.
    double x = 0.;
    double y = 0.;
    double z = 0.;
    if(!getDoubleArg(i, args, x))
      return false;
    if(!getDoubleArg(i, args, y)){
      i--;
      return false;
    }
    if(!getDoubleArg(i, args, z)){
      i-=2;
      return false;
    }
    v = Vector(x,y,z);
    return true;
  }

  bool getColorArg(size_t& i, const vector<string>& args, Color& color)
  {
    string name;
    if (!getStringArg(i, args, name)) {
      return false;
    }
    if (name == "RGB8" || name == "RGBfloat") {
      ColorComponent r(0);
      ColorComponent g(0);
      ColorComponent b(0);
      if (!getArg<ColorComponent>(i, args, r)) {
        return false;
      }
      if (!getArg<ColorComponent>(i, args, g)) {
        i--;
        return false;
      }
      if (!getArg<ColorComponent>(i, args, b)) {
        i-=2;
        return false;
      }
      color = Color(RGBColor(r,g,b));
      if (name == "RGB8") {
        color *= (ColorComponent)1/255;
      }
    } else {
      // Now try and look up the name
      color = ColorDB::getNamedColor(name);
    }
    return true;
  }

  void parseSpec(const string& spec, string& name, vector<string>& args)
  {
    int len = (int)spec.length();
    int start = 0;

    // Skip leading white space
    while(start < len && (spec[start] == ' ' || spec[start] == '\t' || spec[start] == '\n'))
      start++;
    int end = start;

    // Now find the first left paren
    while(end < len && spec[end] != '(')
      end++;
    end--;
    // Back up to find where name starts
    while(end > 0 && (spec[end] == ' ' || spec[end] == '\t' || spec[end] == '\n'))
      end--;
    end++;
    // Name is the stuff after the whitespace and before '(' (if it exists).
    name = spec.substr(start, end-start);

    // Find the first paren, if any
    int paren = end;
    while(paren < len && spec[paren] != '('){
      if(spec[paren] != ' ' && spec[paren] != '\t' && spec[paren] != '\n')
        throw IllegalValue<string>("Error parsing argument, garbage before (", spec);
      paren++;
    }
    if(paren == len){
      // No args
      return;
    }
    paren++;
    while(len > 0 && spec[len-1] != ')'){
      if(spec[len-1] != ' ' && spec[len-1] != '\t' && spec[len-1] != '\n')
        throw IllegalValue<string>("Error parsing argument, no matching ) or garbage after )", spec);
      len--;
    }
    len--;
    while(paren < len){
      while(paren < len && (spec[paren] == ' ' || spec[paren] == '\t' || spec[paren] == '\n'))
        paren++;
      int to = paren+1;
      while(to < len && (spec[to] != ' ' && spec[to] != '\t' && spec[to] != '\n' && spec[to] != '('))
        to++;
      int to2 = to;
      while(to2 < len && (spec[to2] == ' ' || spec[to2] == '\t' || spec[to2] == '\n'))
        to2++;
      if(spec[to2] == '('){
        // Skip to the matching paren
        to2++;
        int pcount = 1;
        while(to2 < len && pcount > 0){
          if(spec[to2] == '(')
            pcount++;
          else if(spec[to2] == ')')
            pcount--;
          to2++;
        }
        to = to2;
      }
      string arg = spec.substr(paren, to-paren);
      args.push_back(arg);
      paren = to+1;
    }
  }

}

static int arg_debug = 0;

#include <iostream>
using namespace std;

namespace Manta {
  void parseArgs(const string& input, vector<string>& args) {
    if (arg_debug) cout << "input = ("<<input.length()<<")("<<input<<")\n";
    int len = (int)input.length();
    int start = 0;

    // This is our while loop which will pick up tokens.
    while(start < len) {
      if (arg_debug) cout << "start = "<<start<<endl;
      // Skip leading white space
      while(start < len && (input[start] == ' ' || input[start] == '\t' ||
                            input[start] == '\n'))
        start++;
      int end = start;
      if (arg_debug) cout << "After whitespace skip, start = "<<start<<", end = "<<end<<endl;

      // Now that we are past the whitespace check to see if we have a
      // quote or not.
      if (input[start] == '\'') {
        if (arg_debug) cout << "Found a quote\n";
        // Then we need to search for the matching quote
        start++;
        do {
          end++;
        } while(end < len && input[end] != '\'');
        // We need to check to find the matching paren
        if (end == len)
          throw IllegalValue<string>("Error parsing arguments, no matching ' or garbage after '", input);

        // Back off the last character
        //  end--;
      } else {
        if (arg_debug) cout << "Parsing regular arg\n";
        // just look for the next whitespace character
        while(end < len && input[end] != ' ' && input[end] != '\t' &&
              input[end] != '\n') {
          if (arg_debug) cout << "input["<<end<<"] = "<<input[end]<<endl;
          end++;
        }
        // Back off the whitespace character
        //end--;
      }
      if (arg_debug) cout << "grabbing arg from "<<start<<" to "<<end<<endl;
      if (end-start > 0) {
        string arg = input.substr(start, end-start);
        if (arg_debug) cout << "arg = "<<arg<<endl;
        args.push_back(arg);
      }

      // Now we can start where we left off
      start = end+1;
    }
  }

}
