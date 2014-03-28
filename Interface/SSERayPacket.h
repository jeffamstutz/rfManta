
//A wrapper structure for a typical SSE/packet traversal, such as a BVH

#ifndef Manta_SSERayPacket_h
#define Manta_SSERayPacket_h

#ifdef MANTA_SSE

#include <Interface/RayPacket.h>

namespace Manta {

  struct MANTA_ALIGN(16) SSERayPacket
  {
    sse_t activeMask[RayPacketData::SSE_MaxSize];
    int activeRays;
    sse_t* orig[3];
    sse_t* dir[3];
    sse_t* inv_dir[3];
    sse_t* signs[3];
    sse_t* normal[3];
    sse_t* minT;
    RayPacket* rp;
  };
}

#endif // MANTA_SSE

#endif
