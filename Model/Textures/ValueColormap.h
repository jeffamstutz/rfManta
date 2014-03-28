#ifndef Manta_Model_Textures_ValueColormap_h
#define Manta_Model_Textures_ValueColormap_h

#include <Core/Color/Colormaps/LinearColormap.h>
#include <Interface/Colormap.h>
#include <Interface/Texture.h>
#include <Model/Primitives/ValuePrimitive.h>

namespace Manta {
  template<typename T, bool DO_SAFE_TYPECASTING=true>
  class ValueColormap : public Texture<Color> {
  public:
    ValueColormap(Colormap<Color, T> *cmap) : cmap(cmap), own_cmap(false) {}

    ValueColormap(float min, float max, ColormapName kind=Default) :
      cmap(LinearColormap<T>::createColormap(kind, min, max)), own_cmap(true) {}

    ~ValueColormap(){
      if(own_cmap)
        delete cmap;
    }

    void setRange(T min, T max) {
      if (own_cmap)
        static_cast<LinearColormap<T>*>(cmap)->setRange(min, max); }

    void wrapColor(bool enableWrap) {
      if (own_cmap)
        static_cast<LinearColormap<T>*>(cmap)->wrapColor(enableWrap);
    }

    void mapValues(Packet<Color>& results,
                   const RenderContext&,
                   RayPacket& rays) const {
      for(int i=rays.begin(); i<rays.end(); i++){
        const ValuePrimitive<T> *prim = 0;

        if(DO_SAFE_TYPECASTING){
          prim = dynamic_cast<const ValuePrimitive<T> *>(rays.getHitPrimitive(i));
          if(!prim){
            throw InternalError("ValueColormap was given primitives that are not ValuePrimitives");
          }
        }
        else{
          prim = static_cast<const ValuePrimitive<T> *>(rays.getHitPrimitive(i));
        }

        const T value = prim->getValue();
        results.set(i, cmap->color(value));
      }
    }

  private:
    Colormap<Color, T> *cmap;
    bool own_cmap;
  };
}

#endif
