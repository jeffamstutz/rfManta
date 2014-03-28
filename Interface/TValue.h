
#ifndef Manta_Interface_TValue_h
#define Manta_Interface_TValue_h

namespace Manta {
  template<class T> class TValue {
  public:
    T value;

    TValue()
      {
      }
    TValue(const T& value)
      : value(value)
      {
      }

    void operator=(const T& newvalue)
      {
	value = newvalue;
      }
    
    operator T() const
      {
	return value;
      }
  private:
    TValue(const TValue<T>&);
    TValue<T>& operator=(const TValue<T>&);
  };
}

#endif
