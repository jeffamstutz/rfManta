#ifndef Manta_Interface_Clonable_h
#define Manta_Interface_Clonable_h

namespace Manta {
  class Clonable {
  public:
    virtual ~Clonable();
    enum CloneDepth {shallow //
    };
    virtual Clonable* clone(CloneDepth depth, Clonable* incoming=0)
    {
      if (incoming)
        return incoming;
      else
        return this;
    }
  };
}
#endif //Manta_Interface_Clonable_h
