
#ifndef Manta_Interface_SetupCallback_h
#define Manta_Interface_SetupCallback_h

#include <Core/Util/CallbackHandle.h>

namespace Manta {
  class SetupContext;
  class SetupCallback : public CallbackHandle {
  public:
    virtual ~SetupCallback();
    virtual void setupBegin(const SetupContext&, int numChannels) = 0;
    virtual void setupDisplayChannel(SetupContext&) = 0;

  protected:
    SetupCallback();
  private:
    SetupCallback(const SetupCallback&);
    SetupCallback& operator=(const SetupCallback&);
  };
}

#endif
