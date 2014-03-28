
#ifndef Manta_ArchiveElement_h
#define Manta_ArchiveElement_h

#include <Core/Persistent/MantaRTTI.h>
#include <Core/Exceptions/SerializationError.h>
#include <string>
#include <typeinfo>

namespace Manta {
  class ArchiveElement {
  public:
    bool reading() const {
      return isreading;
    }
    bool writing() const {
      return !isreading;
    }

    template<class T>
      void readwrite(const std::string& fieldname, T& data);
    template<class T>
      void readwrite(const std::string& fieldname, T*& data);
    template<class T>
      void readwrite(const std::string& fieldname, const T*& data);

    virtual void readwrite(const std::string& fieldname, bool& data) = 0;
    virtual void readwrite(const std::string& fieldname, signed char& data) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned char& data) = 0;
    virtual void readwrite(const std::string& fieldname, short& data) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned short& data) = 0;
    virtual void readwrite(const std::string& fieldname, int& data) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned int& data) = 0;
    virtual void readwrite(const std::string& fieldname, long& data) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned long& data) = 0;
    virtual void readwrite(const std::string& fieldname, long long& data) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned long long& data) = 0;
    virtual void readwrite(const std::string& fieldname, float& data) = 0;
    virtual void readwrite(const std::string& fieldname, double& data) = 0;
    virtual void readwrite(const std::string& fieldname, long double& data) = 0;

    virtual void readwrite(const std::string& fieldname, bool* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, signed char* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned char* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, short* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned short* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, int* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned int* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, long* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned long* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, long long* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, unsigned long long* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, float* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, double* data, int numElements) = 0;
    virtual void readwrite(const std::string& fieldname, long double* data, int numElements) = 0;

    virtual void readwrite(const std::string& fieldname, std::string& data) = 0;

    virtual bool nextContainerElement() = 0;
    virtual bool hasField(const std::string& fieldname) const = 0;
  protected:
    virtual void readwrite(const std::string& fieldname, PointerWrapperInterface& ptr, bool isPointer) = 0;

    virtual ~ArchiveElement();
    ArchiveElement(bool isreading);
    bool isreading;
  };

  template<class T>
  void ArchiveElement::readwrite(const std::string& fieldname, T& data)
  {
    if(writing() && typeid(T) != typeid(data))
      throw SerializationError("Cannot serialize reference to derived class");

    const ClassRTTIInterface<T>* rtti = ClassIndex<T>::getRTTI(typeid(T));
    if(!rtti){
      std::string baseclassname = MantaRTTI<T>::getPublicClassname();
      std::string derivedclassname = typeid(data).name();
      throw SerializationError("Class: " + derivedclassname + " is not a known subclass of " + baseclassname + "(probably missing RTTI information)");
    }
    PointerWrapper<T> ptr(&data, rtti);
    readwrite(fieldname, ptr, false);
  }


  template<class T>
  void ArchiveElement::readwrite(const std::string& fieldname, T*& data)
  {
    const ClassRTTIInterface<T>* rtti;
    if(reading() || !data)
      rtti = ClassIndex<T>::getRTTI(typeid(T));
    else
      rtti = ClassIndex<T>::getRTTI(typeid(*data));
    if(!rtti){
      std::string baseclassname = MantaRTTI<T>::getPublicClassname();
      std::string derivedclassname = typeid(*data).name();
      throw SerializationError("Class: " + derivedclassname + " is not a known subclass of " + baseclassname + "(probably missing RTTI information)");
    }
    PointerWrapper<T> ptr(data, rtti);
    readwrite(fieldname, ptr, true);
    if(reading())
      data = ptr.getPointer();
  }

  template<class T>
  void ArchiveElement::readwrite(const std::string& fieldname, const T*& data) {
    T* tmp = const_cast<T*>(data);
    const ClassRTTIInterface<T>* rtti;
    if(reading() || !tmp)
      rtti = ClassIndex<T>::getRTTI(typeid(T));
    else
      rtti = ClassIndex<T>::getRTTI(typeid(*tmp));
    if(!rtti){
      std::string baseclassname = MantaRTTI<T>::getPublicClassname();
      std::string derivedclassname = typeid(*tmp).name();
      throw SerializationError("Class: " + derivedclassname + " is not a known subclass of " + baseclassname + "(probably missing RTTI information)");
    }
    PointerWrapper<T> ptr(tmp, rtti);
    readwrite(fieldname, ptr, true);
    if(reading())
      data = ptr.getPointer();
  }

}//namespace

#endif
