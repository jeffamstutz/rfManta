
#ifndef Manta_stdRefl_h
#define Manta_stdRefl_h

#include <Core/Persistent/MantaRTTI.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Util/NotFinished.h>
#include <vector>

namespace Manta {
  template<class T>
    class MantaRTTI<std::vector<T> > : public MantaRTTI_BaseClass<std::vector<T> > {
  public:
    static std::string getPublicClassname() {
      init.forceinit();
      return "std::vector<" + MantaRTTI<T>::getPublicClassname() + ">";
    }
    static std::vector<T>* createInstance() {
      init.forceinit();
      return new std::vector<T>();
    }
    static void readwrite(ArchiveElement* archive, std::vector<T>& data) {
      init.forceinit();
      if(archive->reading()){
        data.resize(0);
        while(archive->nextContainerElement()){
          T val;
          data.push_back(val);
          archive->readwrite("element", data.back());
        }
      } else {
        for(typename std::vector<T>::iterator iter = data.begin(); iter != data.end(); iter++)
          archive->readwrite("element", *iter);
      }
    }
    static PersistentStorage::StorageHint storageHint() {
      return PersistentStorage::Container;
    }
  private:
    class Initializer {
    public:
      Initializer() {
        ClassIndex<std::vector<T> >::template registerClass<std::vector<T> >(getPublicClassname());
      }
      void forceinit() {
      }
    };
    static Initializer init;
    static std::string* classname;
  };
  template<class T>
  typename MantaRTTI<std::vector<T> >::Initializer MantaRTTI<std::vector<T> >::init;

  template<class T>
    class MantaRTTI<std::vector<T*> > : public MantaRTTI_BaseClass<std::vector<T*> > {
  public:
    static std::string getPublicClassname() {
      init.forceinit();
      return "std::vector<" + MantaRTTI<T>::getPublicClassname() + "*>";
    }
    static std::vector<T*>* createInstance() {
      return new std::vector<T*>();
    }
    static void readwrite(ArchiveElement* archive, std::vector<T*>& data) {
      init.forceinit();
      if(archive->reading()){
        data.resize(0);
        while(archive->nextContainerElement()){
          T* ptr;
          archive->readwrite("element", ptr);
          data.push_back(ptr);
        }
      } else {
        for(typename std::vector<T*>::iterator iter = data.begin(); iter != data.end(); iter++)
          archive->readwrite("element", *iter);
      }
    }
    static PersistentStorage::StorageHint storageHint() {
      return PersistentStorage::Container;
    }
  private:
    class Initializer {
    public:
      Initializer();
      void forceinit() {
      }
    };
    static Initializer init;
  };
  template<class T>
  MantaRTTI<std::vector<T*> >::Initializer::Initializer()
  {
    ClassIndex<std::vector<T*> >::template registerClass<std::vector<T*> >(getPublicClassname());
  }
  template<class T>
  typename MantaRTTI<std::vector<T*> >::Initializer MantaRTTI<std::vector<T*> >::init;

}

#endif
