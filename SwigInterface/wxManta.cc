
#include <SwigInterface/wxManta.h>

namespace Manta {

  // Helper function to cast manta_interface to RTRT
  void shootOneRay( MantaInterface *manta_interface,
                    Color *result_color,
                    RayPacket *result_rays,
                    Real image_x,
                    Real image_y,
                    int channel_index ) {

    // Check to see that the pointer is to an RTRT instance.
    if ((rtrt = dynamic_cast<RTRT *>(manta_interface))) {

      // Shoot the ray (in a very un-threadsafe manner)
      rtrt->shootOneRay( *result_color, *result_rays, image_x, image_y, channel_index );
    }
    else {
      throw InvalidState( "shootOneRay not passed RTRT instance", __FILE__, __LINE__ );
    }
  }
};

