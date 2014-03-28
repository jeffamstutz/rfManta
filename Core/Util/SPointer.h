
#ifndef Manta_SPointer_h
#define Manta_SPointer_h

#include <Core/Exceptions/NullPointerException.h>
#include <typeinfo>

namespace Manta {
  template<class T> class SPointer {
  public:

    SPointer() : ptr(0) {}
    SPointer(T* ptr) : ptr(ptr)
    {
      if(ptr) ptr->addReference();
    }
    SPointer(const SPointer<T>& copy) 
      : ptr(copy.getPtr())
    {
      if(ptr) ptr->addReference();
    }
    template<class S>
    SPointer(const SPointer<S>& copy) 
      : ptr(copy.getPtr())
    {
      if(ptr) ptr->addReference();
    }
    
    SPointer<T>& operator=(T* copy)
    {
      if(ptr != copy){
        if(ptr && ptr->removeReference() == 0)
          delete ptr;
        ptr=copy;
        if(ptr)
          ptr->addReference();
      }
      return *this;
    }

    SPointer<T>& operator=(const SPointer<T>& copy)
    {
      return operator=(copy.ptr);
    }

    template<class S>
    SPointer<T>& operator=(const SPointer<S>& copy)
    {
      return operator=(copy.getPtr());
    }
    
    ~SPointer()
    {
      if(ptr && ptr->removeReference() == 0)
        delete ptr;
    }
    
    void detach()
    {
      if(!ptr)
        return;
      if(ptr->removeReference() == 0){
        // We had the only copy, increment the refcount and return        
        ptr->addReference();
        return; 
      }
      ptr=ptr->clone();
      ptr->addReference();
    }
    bool isNull()
    {
      return ptr == 0;
    }
    
    inline const T* operator->() const
    {
      if(!ptr)
        throw NullPointerException(typeid(T));
      return ptr;
    }
    inline T* operator->()
    {
      if(!ptr)
        throw NullPointerException(typeid(T));
      return ptr;
    }
    inline T* getPtr()
    {
      return ptr;
    }
    inline T* getPtr() const
    {
      return ptr;
    }
    inline operator bool() const
    {
      return ptr != 0;
    }
    inline bool operator == (const SPointer<T>& a) const
    {
      return a.ptr == ptr;
    }
    inline bool operator != (const SPointer<T>& a) const
    {
      return a.ptr != ptr;
    }
    inline bool operator == (const T* a) const
    {
      return a == ptr;
    }
    inline bool operator != (const T* a) const
    {
      return a != ptr;
    }
  private:
    T* ptr;
  }; // end class SPointer
}

#endif
