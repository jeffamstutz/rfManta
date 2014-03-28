
#ifndef Manta_Core_LargeFile_h
#define Manta_Core_LargeFile_h

#include <stdlib.h>
#include <fstream>

namespace Manta 
{
  size_t fread_big(void *ptr, size_t size, size_t nitems, FILE *stream);
}

#endif
