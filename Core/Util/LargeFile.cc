
#include <Core/Util/LargeFile.h>
#include <fstream>

namespace Manta
{

size_t fread_big(void *ptr, size_t size, size_t nitems, FILE *stream)
{
	long chunkSize = 0x7fffffff;
	long remaining = size*nitems;
	while (remaining > 0) {
		long toRead = remaining>chunkSize?chunkSize:remaining;
		long result;
		if ((result = fread(ptr, toRead, 1, stream)) != 1) {
			fprintf(stderr, "read error: %ld != %ld",
					toRead, result);
			return 0;
		}
		remaining -= toRead;
		ptr = (char*)ptr + toRead;
	}
	return nitems;
}

};
