#ifndef CSAFE_OBJECT_H
#define CSAFE_OBJECT_H

#include <Interface/Object.h>
#include <MantaTypes.h>
#include <Interface/Context.h>
#include <Interface/Texture.h>
#include <Interface/RayPacket.h>
#include <Core/Color/ColorSpace.h>
#include <Model/Materials/Volume.h>
#include <Model/Primitives/ValuePrimitive.h>

namespace Manta
{

class CSAFEPrim : public ValuePrimitive<float> {
public:
  CSAFEPrim(Primitive* object, float* data)
    : ValuePrimitive<float>(object), _data(data), _isOccluded(-1)
    {
      updateOcclusion();
    }
  virtual ~CSAFEPrim() {}
  virtual void intersect(const RenderContext& context, RayPacket& rays) const
    {
      if(_isOccluded)
	return;
      
      ValuePrimitive<float>::intersect(context, rays);
    }
  virtual float getValue() const { return _data[__cindex]; }
  void updateOcclusion()
    {
      _isOccluded = false;
      float data;
       for(int i = 0; i <__numIndices; i++) {
	data = _data[i];
	if (data < __dmins[i] || data > __dmaxs[i]) {
	  _isOccluded = true;
	  return;
	}
      }
    }
  /* void makeDirty()
    {
      _isOccluded = -1;
      }*/

  static void init(int numIndices, float* dmins, float* dmaxs, int cindex)
    {
      __numIndices = numIndices;
      __dmins = dmins;
      __dmaxs = dmaxs;
      __cindex = cindex;
    }


  static void setNumIndices(int numIndices) { __numIndices = numIndices; }
  static void setDMins(float* dmins) { __dmins = dmins; }
  static void setDMaxs(float* dmaxs) {__dmaxs = dmaxs; }
  static void setCIndex(int cindex) { __cindex = cindex; }

protected:
  static int __numIndices;
  float* _data;
  static float* __dmins, *__dmaxs;
  static int __cindex;
  bool _isOccluded;
};

}

#endif
