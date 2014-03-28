
#ifndef Manta_Engine_NullDisplay_h
#define Manta_Engine_NullDisplay_h

#include <Interface/ImageDisplay.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class NullDisplay : public ImageDisplay {
  public:
    NullDisplay(const vector<string>& args);
    virtual ~NullDisplay();
    virtual void setupDisplayChannel(SetupContext&);
    virtual void displayImage(const DisplayContext& context,
			      const Image* image);
    static ImageDisplay* create(const vector<string>& args);
  private:
    NullDisplay(const NullDisplay&);
    NullDisplay& operator=(const NullDisplay&);
  };
}

#endif
