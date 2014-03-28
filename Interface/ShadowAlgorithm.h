

#ifndef Manta_Interface_ShadowAlgorithm_h
#define Manta_Interface_ShadowAlgorithm_h

#include <Interface/RayPacket.h>
#include <string>

namespace Manta {
  class LightSet;
  class RayPacket;
  class RenderContext;


  class ShadowAlgorithm {
  public:
    ShadowAlgorithm();
    virtual ~ShadowAlgorithm();

#ifndef SWIG
    struct StateBuffer {
      // Most shadowAlgorithms will only need to store an int or two in the
      // state buffer, so we can get alignment guarantees and avoid cast
      // nastiness by supplying them here.   Callers of the shadow algorithm
      // should not assume anything about the data in this object!
      int i1, i2;
      char data[128];

      template<class T> T& getData() {

        // This pragma relates to the following expression being
        // constant (which it is).  Since sizeof(T) is evaluated after
        // the preprocessor, we can't get around not doing it here like
        // this.

#     if defined(__sgi) && !defined(__GNUC__)
#       pragma set woff 1209
        ASSERT(sizeof(T) <= sizeof(data));
#       pragma set woff 1209
#     else
        ASSERT(sizeof(T) <= sizeof(data));
#     endif
        return *(T*)data;
      }

      enum State {
        Start, Continuing, Finished
      };
      State state;

      StateBuffer() {
        state = Start;
      }
      bool done() const {
        return state == Finished;
      }
    };

    virtual void computeShadows(const RenderContext& context, StateBuffer& stateBuffer,
                                const LightSet* lights, RayPacket& source, RayPacket& shadowRays) = 0;
#endif
    
    virtual std::string getName() const = 0;
    virtual std::string getSpecs() const = 0;

  private:
    ShadowAlgorithm(const ShadowAlgorithm&);
    ShadowAlgorithm& operator=(const ShadowAlgorithm&);
  };
}

#endif
