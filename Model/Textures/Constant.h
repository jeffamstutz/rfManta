
#ifndef Manta_Model_Constant_h
#define Manta_Model_Constant_h

#include <Interface/Texture.h>
#include <Interface/RayPacket.h>

#include <Interface/InterfaceRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <MantaSSE.h>

namespace Manta {
  class RayPacket;
  class RenderContext;
  template<typename ValueType>
  class Constant : public Texture<ValueType> {
  public:
    Constant();
    Constant(const ValueType& value);
    virtual ~Constant();

    virtual void mapValues(Packet<ValueType>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
    ValueType getValue() const { return value; }
    void setValue( ValueType value_ ) { value = value_; }

    void readwrite(ArchiveElement* archive);
  private:
    Constant(const Constant&);
    Constant& operator=(const Constant&);

    ValueType value;
  };

  template<class ValueType>
    class MantaRTTI<Constant<ValueType> > : public MantaRTTI_DerivedClass<Constant<ValueType>, Texture<ValueType> >, public MantaRTTI_ConcreteClass<Constant<ValueType> >, public MantaRTTI_readwriteMethod<Constant<ValueType> > {
  public:
    static std::string getPublicClassname() {
      return "Constant<"+MantaRTTI<ValueType>::getPublicClassname()+">";
    }
    class Initializer {
    public:
      Initializer() {
        MantaRTTI<Constant<ValueType> >::registerClass();
      }
      void forceinit() {
      }
    };
    static Initializer init;
  };

  template<class ValueType>
  typename MantaRTTI<Constant<ValueType> >::Initializer MantaRTTI<Constant<ValueType> >::init;

  template<class ValueType>
  Constant<ValueType>::Constant(const ValueType& value)
    : value(value)
  {
  }
  
  template<class ValueType>
  Constant<ValueType>::Constant()
  {
  }
  
  template<class ValueType>
  Constant<ValueType>::~Constant()
  {
    MantaRTTI<Constant<ValueType> >::init.forceinit();
  }
  
  template<class ValueType>
    void Constant<ValueType>::mapValues(Packet<ValueType>& results,
                                        const RenderContext&,
                                        RayPacket& rays) const
  {
    for(int i=rays.begin();i<rays.end();i++)
      results.set(i, value);
  }

  template<class ValueType>
  void Constant<ValueType>::readwrite(ArchiveElement* archive)
  {
    archive->readwrite("value", value);
  }

  template<>
    void Constant<Color>::mapValues(Packet<Color>& results,
                                    const RenderContext& context,
                                    RayPacket& rays) const;
  template<>
    void Constant<float>::mapValues(Packet<float>& results,
                                    const RenderContext& context,
                                    RayPacket& rays) const;
}

#endif
