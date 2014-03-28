/*
 *  UDAReaher.h: Parse a subset of UDA XML files to load in volume/sphere data
 *
 *  Written by:
 *   Carson Brownlee
 *   Department of Computer Science
 *   University of Utah
 *   April 2008
 *
 *  Copyright (C) 2008 U of U
 */
#ifndef UDAREADER_H
#define UDAREADER_H 

#include <Core/Geometry/Vector.h>

#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <Core/Containers/GridArray3.h>

namespace Manta
{
 
class UDAReader
{
public:
        enum { particleVariable, cCVariable, nCVariable, doubleT, floatT, pointT, vectorT, unknown };
    struct VarInfo
    {
      VarInfo() { type = dataType = unknown; compressed = false; dataIndex = -1; 
      start = end = 0; }
      std::string name, filename;
        int start, end, index, patchId, type, dataType, numParticles, dataIndex;
      bool compressed;
    };
    struct Patch
    {
        int id;
        Vector lower, upper, lowIndex, highIndex;
    };
    struct Timestep
    {
      Timestep() { sphereData = NULL; volume = new GridArray3<float>(); numSpheres = 0; }
      std::string dir;
        Vector lower, upper, indices;
      std::map <std::string, std::vector<VarInfo> > dataMapping;
      std::map <int, Patch> patches;
      GridArray3<float>* volume;
      float* sphereData;
      int numSphereVars, numSpheres;
    };
    struct VarHeader  //variable information, from the index.xml header
    {
      VarHeader() {}
      VarHeader(std::string name_, std::string varType_, std::string dataType_)
	  { name = name_; varType = varType_; dataType = dataType_; }
      std::string name, varType, dataType;
    };

    UDAReader() {}
    void readUDAHeader(std::string directory);
    void readUDA(std::string directory, std::string volumeVarName);
    void parseTimestepFile(std::string filename);
    void parseDataFile(std::string file, Timestep& t);
    std::vector<Timestep> timesteps;
    int getNumVariables() { return _varHeaders.size(); }
    VarHeader getVarHeader(int i) { int j = 0; for(std::map<std::string, VarHeader>::iterator itr = _varHeaders.begin(); itr != _varHeaders.end(); itr++) { if (j++ == i) return itr->second; } return VarHeader(); }
private:
    std::string _endianness;
    std::string _volumeVarName;
    std::string _directory;
    void readData(std::string filename, Timestep& t);
    Vector readPoint(char* p);
    float readFloat(char* p);
    double readDouble(char* p);
    std::map<std::string, VarHeader> _varHeaders;
};

}
 
#endif

