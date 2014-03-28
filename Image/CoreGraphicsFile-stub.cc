
#include <Image/CoreGraphicsFile.h>

#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>

class Manta::Image;

extern "C" 
bool isCoreGraphics( const std::string& filename )
{
  return false;
}

extern "C" 
void writeCoreGraphics( const Manta::Image* image, const std::string &filename, int which=0 )
{
  throw Manta::OutputError("CoreGraphics is not supported on this platform");
}

extern "C" 
Manta::Image* readCoreGraphics( const std::string& filename )
{
  throw Manta::OutputError("CoreGraphics is not supported on this platform");
}

// Returns true if this reader is supported
extern "C" 
bool CoreGraphicsSupported()
{
  return false;
}
