
#include <Core/Color/GrayColor.h>

#include <sstream>
#include <string>

using namespace Manta;
using namespace std;

string GrayColor::toString() const {
  ostringstream out;
  out << "GrayColor("<<value<<")";
  return out.str();
}
