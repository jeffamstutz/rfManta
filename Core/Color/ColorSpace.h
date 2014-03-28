
#ifndef Manta_Core_ColorSpace_h
#define Manta_Core_ColorSpace_h

#include <Core/Color/GrayColor.h>
#include <Core/Color/GrayTraits.h>
#include <Core/Color/RGBColor.h>
#include <Core/Color/RGBTraits.h>
#include <Core/Color/Spectrum.h>
#include <Core/Math/Expon.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Persistent/MantaRTTI.h>

#include <string>

namespace Manta {
  template<typename Traits> class ColorSpace {
  public:
    typedef typename Traits::ComponentType ComponentType;
    enum { NumComponents = Traits::NumComponents};

    ColorSpace() { }

    ColorSpace(const ColorSpace<Traits>& copy)
    {
      for(int i=0;i<NumComponents;i++)
        data[i]=copy.data[i];
    }
#ifndef SWIG
    ColorSpace& operator=(const ColorSpace<Traits> &copy) {
      for(int i=0;i<NumComponents;i++)
        data[i] = copy.data[i];
      return *this;
    }
#endif
    ~ColorSpace() {
    }

#ifndef SWIG
    // Access individual components
    const ComponentType& operator[](int i) const
    {
      return data[i];
    }
    ComponentType& operator[](int i)
    {
      return data[i];
    }
#else
    %extend {
      ComponentType& __getitem__( int i ) {
        return self->operator[](i);
      }
    }
#endif

    // These are the fixpoints, rather than true colors black and white
    // black is the fixpoint under addition
    // white is the fixpoint under multiplication
    // for linear color spaces, they will be where all components
    // are zero or one, respectively.
    static ColorSpace<Traits> black() {
      return ColorSpace<Traits>(0);
    }
    static ColorSpace<Traits> white() {
      return ColorSpace<Traits>(1);
    }

    // Conversions to concrete color classes

    //
    // This is what I originally did, but I thought that the error
    // messages were confusing when you tried to pass the wrong thing
    // into the ctor
    //
    //template<class C>
    //  explicit ColorSpace(const C& color) {
    //  Traits::convertFrom(data, color);
    //}

    explicit ColorSpace(const GrayColor& color) {
      Traits::convertFrom(data, color);
    }
    explicit ColorSpace(const RGBColor& color) {
      Traits::convertFrom(data, color);
    }
    explicit ColorSpace(const Spectrum& spectrum) {
      Traits::convertFrom(data, spectrum);
    }

    // This is what I originally did, but I didn't like the syntax.
    // This was fine:
    //    color.convert<RGBColor>()
    // but if the class was templated you were forced to do:
    //    color.template convert<RGBColor>()
    // which is ugly, and many users probably wouldn't understand it.
    // I am leaving it since it still works fine.
#if 0
    template<typename C>
      C convert() const {
      C returnValue;
      Traits::convertTo(returnValue, data);
      return returnValue;
    }
#endif
    RGBColor convertRGB() const {
      RGBColor returnValue;
      Traits::convertTo(returnValue, data);
      return returnValue;
    }

    // Operators - with scalars
    template<typename Scalar>
    ColorSpace<Traits> operator*(Scalar scale) const {
      ColorSpace<Traits> returnValue;
      for(int i=0;i<NumComponents;i++)
        returnValue.data[i] = data[i] * scale;
      return returnValue;
    }
    template<typename Scalar>
    ColorSpace<Traits>& operator*=(Scalar scale) {
      for(int i=0;i<NumComponents;i++)
        data[i] *= scale;
      return *this;
    }

    template<typename Scalar>
    ColorSpace<Traits> operator/(Scalar scale) const {
      ColorSpace<Traits> returnValue;
      for(int i=0;i<NumComponents;i++)
        returnValue.data[i] = data[i] / scale;
      return returnValue;
    }
    template<typename Scalar>
    ColorSpace<Traits>& operator/=(Scalar scale) {
      for(int i=0;i<NumComponents;i++)
        data[i] /= scale;
      return *this;
    }

    // Operators - with other colors
    ColorSpace<Traits> operator*(const ColorSpace<Traits>& c) const {
    ColorSpace<Traits> returnValue;
      for(int i=0;i<NumComponents;i++)
        returnValue.data[i] = data[i] * c.data[i];
      return returnValue;
    }
    ColorSpace<Traits>& operator*=(const ColorSpace<Traits>& c) {
      for(int i=0;i<NumComponents;i++)
        data[i] *= c.data[i];
      return *this;
    }

    ColorSpace<Traits> operator+(const ColorSpace<Traits>& c) const {
      ColorSpace<Traits> returnValue;
      for(int i=0;i<NumComponents;i++)
        returnValue.data[i] = data[i] + c.data[i];
      return returnValue;
    }
    ColorSpace<Traits>& operator+=(const ColorSpace<Traits>& c) {
      for(int i=0;i<NumComponents;i++)
        data[i] += c.data[i];
      return *this;
    }

    ColorSpace<Traits> operator-(const ColorSpace<Traits>& c) const {
      ColorSpace<Traits> returnValue;
      for(int i=0;i<NumComponents;i++)
        returnValue.data[i] = data[i] - c.data[i];
      return returnValue;
    }
    ColorSpace<Traits>& operator-=(const ColorSpace<Traits>& c) {
      for(int i=0;i<NumComponents;i++)
        data[i] -= c.data[i];
      return *this;
    }

    bool operator==(const ColorSpace<Traits>& c) {
      for(int i=0;i<NumComponents;i++)
        if(data[i] != c.data[i])
          return false;
      return true;
    }
    bool operator!=(const ColorSpace<Traits>& c) {
      for(int i=0;i<NumComponents;i++)
        if(data[i] != c.data[i])
          return true;
      return false;
    }


    ComponentType Mean() const
    {
      ComponentType sum = 0;
      for (int i = 0; i < NumComponents; i++)
        sum += data[i];
      return sum*(1/ComponentType(NumComponents));
    }

    ComponentType maxComponent() const
    {
      ComponentType max = data[0];
      for(int i = 1; i < NumComponents; i++)
        if(data[i] > max)
          max = data[i];
      return max;
    }
    ComponentType minComponent() const
    {
      ComponentType min = data[0];
      for(int i = 1; i < NumComponents; i++)
        if(data[i] > min)
          min = data[i];
      return min;
    }

    // Apply functor
    ColorSpace<Traits> attenuate(ComponentType scale) const {
      ColorSpace<Traits> returnValue;
      for(int i=0;i<NumComponents;i++)
        returnValue.data[i] = Exp(scale*data[i]);
      return returnValue;
    }

    // This a nasty function for a templated function that will be
    // included by the world.  I don't want to have to include sstream
    // in order to have the implementation here, so I will stick it
    // (the implementation) in another header file called
    // ColorSpace_fancy.h.  If you want to use this function include
    // that header file and it will get instantiated properly.
    std::string toString() const;

    template<typename Scalar>
    ColorSpace<Traits> Pow(Scalar exponent) const
    {
      ColorSpace<Traits> returnValue;
      for (int i=0; i < NumComponents; i++)
        returnValue.data[i] = Manta::Pow(data[i], static_cast<ComponentType>(exponent));
      return returnValue;
    }

    ColorSpace<Traits> Log() const
    {
      ColorSpace<Traits> returnValue;
      for (int i=0; i < NumComponents; i++)
        returnValue.data[i] = log(data[i]);
      return returnValue;
    }

    ColorSpace<Traits> attenuate(const ColorSpace<Traits> &scale) const
    {
      ColorSpace<Traits> returnValue;
      for(int i=0;i<NumComponents;i++)
        returnValue.data[i] = Exp(scale.data[i]*data[i]);
      return returnValue;
    }

    ComponentType luminance() const
    {
      return Traits::luminance(data);
    }

    static ComponentType linearize(ComponentType val) {
      return Traits::linearize(val);
    }

#ifndef SWIG // Swig gets confused about NumComponents, and you can
             // use the regular [] operators.
    ComponentType data[NumComponents];
#endif

    void readwrite(ArchiveElement*);

  protected:
    // DO NOT MAKE THIS PUBLIC!
    ColorSpace(ComponentType fillValue) {
      for(int i=0;i<NumComponents;i++)
        data[i] = fillValue;
    }
    friend class MantaRTTI<ColorSpace<Traits> >;
  };

  template<typename Traits, typename Scalar>
    ColorSpace<Traits> operator*(Scalar scale, const ColorSpace<Traits>& s)
    {
      ColorSpace<Traits> returnValue;
      for(int i=0;i<ColorSpace<Traits>::NumComponents;i++)
        returnValue[i] = scale * s[i];
      return returnValue;
    }

  template<typename Traits>
    class MantaRTTI<ColorSpace<Traits> >
    : public MantaRTTI_ConcreteClass<ColorSpace<Traits> >,
      public MantaRTTI_BaseClass<ColorSpace<Traits> >
  {
  public:
    static std::string getPublicClassname() {
      return "Color";
    }
    static void readwrite(ArchiveElement* archive, ColorSpace<Traits>& data) {
      if(archive->reading()){
        // Read a number of different color representations
        std::string str;
        archive->readwrite("data", str);
        RGBColor rgb;
        Spectrum spectrum;
        if(rgb.readPersistentString(str)){
          Traits::convertFrom(data.data, rgb);
        } else if(spectrum.readPersistentString(str)){
          Traits::convertFrom(data.data, spectrum);
        } else {
          throw SerializationError("Unknown color value: " + str);
        }
      } else {
        typename Traits::ColorType outcolor;
        Traits::convertTo(outcolor, data.data);
        std::string str = outcolor.writePersistentString();
        archive->readwrite("data", str);
      }
    }
    static PersistentStorage::StorageHint storageHint() {
      return PersistentStorage::Lightweight;
    }
    static bool force_initialize;
  };

}

#endif
