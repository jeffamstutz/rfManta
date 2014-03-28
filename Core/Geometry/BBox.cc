#include <iostream>
#include <Core/Geometry/BBox.h>
#include <Core/Persistent/ArchiveElement.h>
using namespace Manta;

namespace Manta {
  std::ostream& operator<< (std::ostream& os, const BBox& bounds) {
    os << bounds.getMin() << " " << bounds.getMax();
    return os;
  }

  MANTA_REGISTER_CLASS(BBox);

  void BBox::readwrite(ArchiveElement* archive) {
    archive->readwrite("min", bounds[0]);
    archive->readwrite("max", bounds[1]);
  }
}
