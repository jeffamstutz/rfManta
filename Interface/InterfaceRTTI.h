
#ifndef Manta_InterfaceRTTI_h
#define Manta_InterfaceRTTI_h

#include <Core/Persistent/MantaRTTI.h>

namespace Manta {
  class AccelerationStructure;
  class AmbientLight;
  class Background;
  class Clonable;
  class Interpolable;
  class Light;
  class LightSet;
  class Material;
  class Object;
  class Primitive;
  class RenderParameters;
  class Scene;
  class TexCoordMapper;

  template<class ValueType> class Texture;

  MANTA_DECLARE_RTTI_BASECLASS(AmbientLight, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_BASECLASS(Light, AbstractClass,readwriteNone);
  MANTA_DECLARE_RTTI_DERIVEDCLASS(Material, Interpolable, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_DERIVEDCLASS(TexCoordMapper, Interpolable, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_BASECLASS(Scene, ConcreteClass, readwriteMethod);
  MANTA_DECLARE_RTTI_BASECLASS(Background, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_DERIVEDCLASS(Object, Interpolable, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_DERIVEDCLASS(Primitive, Object, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_BASECLASS(LightSet, ConcreteClass, readwriteMethod);
  MANTA_DECLARE_RTTI_BASECLASS(RenderParameters, ConcreteClass, readwriteMethod);
  MANTA_DECLARE_RTTI_DERIVEDCLASS(Interpolable, Clonable, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_BASECLASS(Clonable, AbstractClass, readwriteNone);
  MANTA_DECLARE_RTTI_DERIVEDCLASS(AccelerationStructure, Object, AbstractClass, readwriteNone);

  template<class ValueType>
  class MantaRTTI<Texture<ValueType> > : public MantaRTTI_DerivedClass<Texture<ValueType>, Interpolable>, public MantaRTTI_AbstractClass<Texture<ValueType> >, public MantaRTTI_readwriteNone<Texture<ValueType> > {
  public:
    static bool force_initialize;
    static std::string getPublicClassname() {
      return "Texture<"+MantaRTTI<ValueType>::getPublicClassname()+">";
    }
  };

  template<class ValueType>
    bool MantaRTTI<Texture<ValueType> >::force_initialize = MantaRTTI<Texture<ValueType> >::registerClass();
}

#endif
