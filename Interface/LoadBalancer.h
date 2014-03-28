
#ifndef Manta_Interface_LoadBalancer_h
#define Manta_Interface_LoadBalancer_h

namespace Manta {
  class RenderContext;
  class SetupContext;
  class LoadBalancer {
  public:
    virtual ~LoadBalancer();

    virtual void setupBegin(const SetupContext&, int numChannels) = 0;
    virtual void setupDisplayChannel(SetupContext& context,
				     int numAssignments) = 0;
    virtual void setupFrame(const RenderContext& context) = 0;
    virtual bool getNextAssignment(const RenderContext& context,
				   int& start, int& end) = 0;
  protected:
    LoadBalancer();
  private:
    LoadBalancer(const LoadBalancer&);
    LoadBalancer& operator=(const LoadBalancer&);
  };
}

#endif
