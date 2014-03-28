
#ifndef Manta_Model_CheckerTexture_h
#define Manta_Model_CheckerTexture_h

#include <Interface/Texture.h>
#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Interface/InterfaceRTTI.h>
#include <Interface/RayPacket.h>
#include <MantaSSE.h>

namespace Manta {
  class RayPacket;
  class RenderContext;
  template<typename ValueType>
  class CheckerTexture : public Texture<ValueType> {
  public:
    CheckerTexture();
    CheckerTexture(const ValueType& value1,
                   const ValueType& value2,
                   const Vector& v1, const Vector& v2);
    virtual ~CheckerTexture();

    virtual void mapValues(Packet<ValueType>&, const RenderContext& context, RayPacket& rays) const;
    void readwrite(ArchiveElement* archive);
  private:
    CheckerTexture(const CheckerTexture&);
    CheckerTexture& operator=(const CheckerTexture&);

    // NOTE(boulos): This has to be an array so that the default
    // constructor (which has to be defined for the Persistent stuff)
    // for Color(ComponentType) isn't called (because that's
    // protected) so we get a confusing error.
    ValueType values[2];
    Vector v1;
    Vector v2;
    bool need_w;
  };

  template<class ValueType>
  CheckerTexture<ValueType>::CheckerTexture()
  {
  }

  template<class ValueType>
  CheckerTexture<ValueType>::CheckerTexture(const ValueType& value1,
                                            const ValueType& value2,
                                            const Vector& v1,
                                            const Vector& v2)
    : v1(v1), v2(v2)
  {
    values[0] = value1;
    values[1] = value2;
    if(v1.z() == 0 && v2.z() == 0)
      need_w = false;
    else
      need_w = true;
  }

  template<class ValueType>
    void CheckerTexture<ValueType>::mapValues(Packet<ValueType>& results,
                                              const RenderContext& context,
                                              RayPacket& rays) const
  {
    if(need_w)
      rays.computeTextureCoordinates3(context);
    else
      rays.computeTextureCoordinates2(context);
    for(int i=rays.begin();i<rays.end();i++){
      Real vv1 = Dot(rays.getTexCoords(i), v1);
      Real vv2 = Dot(rays.getTexCoords(i), v2);
      if(vv1<0)
        vv1=-vv1+1;
      if(vv2<0)
        vv2=-vv2+1;
      int i1 = (int)vv1;
      int i2 = (int)vv2;
      int which = (i1+i2)&1;
      results.set(i, values[which]);
    }
  }

#ifdef MANTA_SSE
  template<>
    void CheckerTexture<Color>::mapValues(Packet<Color>& results,
                                          const RenderContext& context,
                                          RayPacket& rays) const;
  template<>
    void CheckerTexture<float>::mapValues(Packet<float>& results,
                                          const RenderContext& context,
                                          RayPacket& rays) const;
#endif

  template<class ValueType>
    class MantaRTTI<CheckerTexture<ValueType> > : public MantaRTTI_DerivedClass<CheckerTexture<ValueType>, Texture<ValueType> >, public MantaRTTI_ConcreteClass<CheckerTexture<ValueType> >, public MantaRTTI_readwriteMethod<CheckerTexture<ValueType> > {
  public:
    static std::string getPublicClassname() {
      return "CheckerTexture<"+MantaRTTI<ValueType>::getPublicClassname()+">";
    }
    class Initializer {
    public:
      Initializer() {
        MantaRTTI<CheckerTexture<ValueType> >::registerClass();
      }
      void forceinit() {
      }
    };
    static Initializer init;
  };

  template<class ValueType>
  typename MantaRTTI<CheckerTexture<ValueType> >::Initializer MantaRTTI<CheckerTexture<ValueType> >::init;

  template<class ValueType>
  void CheckerTexture<ValueType>::readwrite(ArchiveElement* archive)
  {
    archive->readwrite("value1", values[0]);
    archive->readwrite("value2", values[1]);
    archive->readwrite("v1", v1);
    archive->readwrite("v2", v2);
    if(archive->reading()){
      if(v1.z() == 0 && v2.z() == 0)
        need_w = false;
      else
        need_w = true;
    }
  }

  template<class ValueType>
  CheckerTexture<ValueType>::~CheckerTexture()
  {
    MantaRTTI<CheckerTexture<ValueType> >::init.forceinit();
  }
}


#endif
