
#include <Core/Color/RGBTraits.h>
#include <Core/Color/ColorSpace.h>

using namespace Manta;
using namespace std;

template<>
bool MantaRTTI<ColorSpace<RGBTraits> >::force_initialize = MantaRTTI<ColorSpace<RGBTraits> >::registerClass();

