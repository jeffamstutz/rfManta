
#ifndef Manta_Interface_Packet_h
#define Manta_Interface_Packet_h

#include <RayPacketParameters.h>
#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <Core/Util/Align.h>

namespace Manta {
  template<typename ValueType>
    class MANTA_ALIGN(16) Packet {
  public:
    enum {
      MaxSize = RAYPACKET_MAXSIZE
    };
    MANTA_ALIGN(16) ValueType data[MaxSize];
    const ValueType& get(int idx) const {
      return data[idx];
    }
    ValueType get(int idx) {
      return data[idx];
    }
    void set(int idx, ValueType value) {
      data[idx] = value;
    }
  };
  template<>
    class MANTA_ALIGN(16) Packet<Color> {
  public:
    enum {
      MaxSize = RAYPACKET_MAXSIZE
    };
    MANTA_ALIGN(16) Color::ComponentType colordata[Color::NumComponents][MaxSize];
    Color get(int idx) const {
      return Color(RGBColor(colordata[0][idx], colordata[1][idx], colordata[2][idx]));
    }
    void set(int idx, const Color& value) {
      colordata[0][idx] = value[0];
      colordata[1][idx] = value[1];
      colordata[2][idx] = value[2];
    }
    void fill(int start, int end, const Color& value) {
      for(int i=start;i<end;i++){
        colordata[0][i] = value[0];
        colordata[1][i] = value[1];
        colordata[2][i] = value[2];
      }
    }
  };
  template<>
    class MANTA_ALIGN(16) Packet<Vector> {
  public:
    enum {
      MaxSize = RAYPACKET_MAXSIZE
    };
    MANTA_ALIGN(16) Real vectordata[3][MaxSize];
    Vector get(int idx) const {
      return Vector(vectordata[0][idx], vectordata[1][idx], vectordata[2][idx]);
    }
    void set(int idx, const Vector& value) {
      vectordata[0][idx] = value[0];
      vectordata[1][idx] = value[1];
      vectordata[2][idx] = value[2];
    }
  };
}

#endif
