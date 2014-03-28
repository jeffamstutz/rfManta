
#ifndef Manta_Interface_IdleMode_h
#define Manta_Interface_IdleMode_h

namespace Manta {
  class SetupContext;
  class RenderContext;
  class RayPacket;
  class IdleMode {
  public:
    virtual ~IdleMode();

    // changeIdleMode is called whenever a switch from Idle->Active or
    // Active->Idle occurs.  If something in the universe has changed,
    // then changed = true, otherwise changed=false.  For example,
    // moving the camera will notify you that changed=true.
    // Additionally, changeIdleMode is called for the first frame.
    virtual void changeIdleMode(const SetupContext&, bool changed,
                                bool firstFrame, bool& pipelineNeedsSetup) = 0;
  protected:
    IdleMode();
  private:
    IdleMode(const IdleMode&);
    IdleMode& operator=(const IdleMode&);
  };
}

#endif
