
#ifndef Manta_Model_WireframeTexture_h
#define Manta_Model_WireframeTexture_h

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
  class WireframeTexture : public Texture<ValueType> {
  public:
    WireframeTexture();
    WireframeTexture(const ValueType& border,
                     const ValueType& interior,
                     float border_width);
    virtual ~WireframeTexture();

    virtual void mapValues(Packet<ValueType>&, const RenderContext& context, RayPacket& rays) const;
    void readwrite(ArchiveElement* archive);
  private:
    WireframeTexture(const WireframeTexture&);
    WireframeTexture& operator=(const WireframeTexture&);

    ValueType values[2]; // NOTE(boulos): This is because
                         // Color(ComponentType) is not allowable, so
                         // we don't want default constructors run.
    float border_width;
  };

  template<class ValueType>
  WireframeTexture<ValueType>::WireframeTexture()
  {
  }

  template<class ValueType>
  WireframeTexture<ValueType>::WireframeTexture(const ValueType& border,
                                                const ValueType& interior,
                                                float border_width)
    : border_width(border_width)
  {
    values[0] = border;
    values[1] = interior;
  }

  template<class ValueType>
    void WireframeTexture<ValueType>::mapValues(Packet<ValueType>& results,
                                                const RenderContext& context,
                                                RayPacket& rays) const
  {
    rays.computeTextureCoordinates2(context);

    for(int i=rays.begin();i<rays.end();i++){
      float u = rays.getTexCoords(i)[0];
      float v = rays.getTexCoords(i)[1];
      float w = 1.f - u - v;

      // Object space border check (can also do projected size)
      bool u_border = (u < .5f * border_width || (1-u) < .5f * border_width);
      bool v_border = (v < .5f * border_width || (1-v) < .5f * border_width);
      bool w_border = (w < .5f * border_width || (1-w) < .5f * border_width);
      bool border = u_border || v_border || w_border;
      if (border) {
        results.set(i, values[0]);
      } else {
        results.set(i, values[1]);
      }
    }
  }

  template<class ValueType>
    class MantaRTTI<WireframeTexture<ValueType> > : public MantaRTTI_DerivedClass<WireframeTexture<ValueType>, Texture<ValueType> >, public MantaRTTI_ConcreteClass<WireframeTexture<ValueType> >, public MantaRTTI_readwriteMethod<WireframeTexture<ValueType> > {
  public:
    static std::string getPublicClassname() {
      return "WireframeTexture<"+MantaRTTI<ValueType>::getPublicClassname()+">";
    }
    class Initializer {
    public:
      Initializer() {
        MantaRTTI<WireframeTexture<ValueType> >::registerClass();
      }
      void forceinit() {
      }
    };
    static Initializer init;
  };

  template<class ValueType>
  typename MantaRTTI<WireframeTexture<ValueType> >::Initializer MantaRTTI<WireframeTexture<ValueType> >::init;

  template<class ValueType>
  void WireframeTexture<ValueType>::readwrite(ArchiveElement* archive)
  {
    archive->readwrite("border", values[0]);
    archive->readwrite("interior", values[1]);
    archive->readwrite("border_width", border_width);
  }

  template<class ValueType>
  WireframeTexture<ValueType>::~WireframeTexture()
  {
    MantaRTTI<WireframeTexture<ValueType> >::init.forceinit();
  }
}


#endif
