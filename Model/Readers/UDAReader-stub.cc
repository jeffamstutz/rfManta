#include <Model/Readers/UDAReader.h>
#include <iostream>
using namespace Manta;
using namespace std;

void UDAReader::readUDA(string directory, string)
{
    cerr << "UDAReader compiled without libxml, please link libxml2 and recompile Manta\n";
}

void UDAReader::readUDAHeader(string directory)
{
    cerr << "UDAReader compiled without libxml, please link libxml2 and recompile Manta\n";
}
