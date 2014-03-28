
#ifndef SWIGINTERFACE_WXMANTA__H
#define SWIGINTERFACE_WXMANTA__H

#include <MantaTypes.h>
#include <Interface/MantaInterface.h>
#include <Interface/RayPacket.h>
#include <Core/Color/Color.h>


namespace Manta {
  void shootOneRay( MantaInterface *manta_interface,
                    Color *result_color,
                    RayPacket *result_rays,
                    Real image_x,
                    Real image_y,
                    int channel_index );
};

#endif
