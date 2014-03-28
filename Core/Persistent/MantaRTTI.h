
#ifndef Manta_MantaRTTI_h
#define Manta_MantaRTTI_h

#include <Core/Exceptions/SerializationError.h>
#include <Core/Exceptions/InternalError.h>
#include <map>
#include <string>
#include <typeinfo>

namespace Manta {
  class ArchiveElement;

  /**
   * Degenerate class to specify hints to contain an enum that
   * provides hints to the persistent storage facility about how
   * an object should be stored.
   *   Default: use a hierchical mechanism
   *   Container: the object is a container and does not need type information.
   *   Lightweight: the object is a small class that could be inlined.
   */
  class PersistentStorage {
  public:
    enum StorageHint {
      Default, Container, Lightweight
    };
  private:
    PersistentStorage();
  };

  /**
   * The MantaRTTI<T> class is templated over all types that can be
   * used with the persistent I/O capability.  These classes are never
   * instantiated.  It should implement several static methods,
   * including the following:
   *
   * // Return a human readable string that indicated the name of the
   * // class
   * string getPublicClassname();
   *
   *  // Create an empty instance of the class using the new operator
   *  T* createInstance();
   *
   *  // Recursively serialize/deserialize the object into the given
   *  // record
   *  void readwrite(ArchiveElement*, T&);
   *
   * These methods can often be provided by the default classes below
   * that provide standard implementations by making the
   * specialization inherit from the default implementation class.
   * Macros are provided to further simplify this for most classes.
   *
   * The constructor for these classes should take no arguments.
   * Instances of it are held by the ClassIndex class that indexes all
   * of the children of a given type.
   *
   * This class should always be specialized.  If you see an error
   * here, you need to define the specialization for the class.
   */
  template<class T>
  class MantaRTTI {
    char force_error[0];
  private:
    /*
     * If you get a compile error here complaining about private scope
     * then you likely do have the specialized MantaRTTI defined for
     * the parent, or you have not included it in the current
     * compilation unit
     */
    template<class C>
      static void registerClassAndParents(const std::string& classname);
  };

  class PointerWrapperInterface;

  /**
   * This is the interface to dynamic RTTI.  Use the ClassIndex below to get
   * instances of this given a classname or a typeid.  Functionality is limited
   * but this can operate on any class type.
   */
  class GenericRTTIInterface {
  public:
    virtual ~GenericRTTIInterface();
    virtual std::string getPublicClassname() const = 0;
    virtual PersistentStorage::StorageHint storageHint() const = 0;
    virtual const std::type_info& get_typeinfo() const = 0;
  };

  class PointerWrapperInterface {
  public:
    virtual ~PointerWrapperInterface();

    virtual bool isNull() const = 0;
    virtual void setNull() = 0;
    virtual bool upcast(PointerWrapperInterface* destination) = 0;
    virtual void* getUniqueID() const = 0;
    virtual void readwrite(ArchiveElement*) = 0;
    virtual const GenericRTTIInterface* getRTTI() const = 0;
    virtual bool createObject(const std::string& classname, PointerWrapperInterface** derivedptr = 0) = 0;
  };

  template<class T> class PointerWrapper;

  /**
   * This is the interface to dynamic RTTI for types derived from the given Base.
   * In addition to the capabilities of the GenericRTTIInterface, you can
   * create instances of a class, trigger persistent I/O and get a unique
   * pointer value.  Use the ClassIndex below to get an instance of this
   * interface.
   */
  template<class Base> class ClassRTTIInterface : public GenericRTTIInterface {
  public:
    virtual ~ClassRTTIInterface() {}
    virtual Base* createObject(PointerWrapperInterface** derivedptr) const = 0;
    virtual void readwrite(ArchiveElement* archive, Base* ptr) const = 0;
    virtual void* getPointerValue(Base* ptr) const = 0;
  };

  template<class T>
  class PointerWrapper : public PointerWrapperInterface {
  public:
    PointerWrapper(T* ptr, const ClassRTTIInterface<T>* rtti)
      : ptr(ptr), rtti(rtti) {
    }
    virtual ~PointerWrapper() {}
    T* getPointer() const {
      return ptr;
    }
    void setPointer(T* newptr) {
      ptr = newptr;
    }

    bool isNull() const {
      return ptr == 0;
    }
    void setNull() {
      ptr = 0;
    }
    virtual bool upcast(PointerWrapperInterface* destination) {
      if(!ptr)
        return 0;
      return MantaRTTI<T>::upcast(ptr, destination);
    }
    virtual bool createObject(const std::string& classname,
                              PointerWrapperInterface** derivedptr = 0);
    virtual void* getUniqueID() const {
      // This will cast it to the most derived class and return
      // the void* equivalent.  This ensures that the pointer will be
      // unique.
      return rtti->getPointerValue(ptr);
    }
    virtual void readwrite(ArchiveElement* element) {
      rtti->readwrite(element, ptr);
    }
    virtual const GenericRTTIInterface* getRTTI() const {
      return rtti;
    }
  private:
    T* ptr;
    const ClassRTTIInterface<T>* rtti;
  };

  /**
   * This class indexes MantaRTTI information for all of the known subclasses
   * of a particular class.  For example, for the following classes:
   * class A {};
   * class B : public A {};
   * class C : public B {};
   * There will be three different index classes, ClassIndex<A> will contain
   * records for A, B, and C, ClassIndex<B> will contain records for B and C,
   * and C will contain a record only for C.
   *
   * This provides access to the MantaRTTI information through an
   * abstract interface.  This design avoids the need to require virtual
   * specific virtual functions in the class.
   */
  template<class Base>
  class ClassIndex {
  private:
    template<class Derived>
    class Entry : public ClassRTTIInterface<Base> {
    public:
      virtual ~Entry() {}
      virtual std::string getPublicClassname() const {
        return MantaRTTI<Derived>::getPublicClassname();
      }
      virtual PersistentStorage::StorageHint storageHint() const {
        return MantaRTTI<Derived>::storageHint();
      }
      virtual const std::type_info& get_typeinfo() const {
        return typeid(Derived);
      }
      virtual Derived* createObject(PointerWrapperInterface** derivedptr) const {
        Derived* result = MantaRTTI<Derived>::createInstance();
        if(result && derivedptr)
          *derivedptr = new PointerWrapper<Derived>(result, ClassIndex<Derived>::getRTTI(typeid(Derived)));
        return result;
      }
      virtual void readwrite(ArchiveElement* archive, Base* ptr) const {
        Derived* derivedptr = dynamic_cast<Derived*>(ptr);
        if(!derivedptr){
          std::string n1 = typeid(Derived).name();
          std::string n2 = typeid(Base).name();
          throw InternalError("failed dynamic cast from " + n1 + " to " + n2 + " in ClassIndex(should not fail)");
        }
        MantaRTTI<Derived>::readwrite(archive, *derivedptr);
      }
      virtual void* getPointerValue(Base* ptr) const {
        Derived* derivedptr = dynamic_cast<Derived*>(ptr);
        return static_cast<void*>(derivedptr);
      }
    };
  public:

    template<class Derived>
    static void registerClass(const std::string& name) {
      ClassIndex<Base>* db = singleton();
      Entry<Derived>* entry = new Entry<Derived>();
      std::pair<typename namedb_type::iterator, bool> result;
      result = db->index.insert(typename namedb_type::value_type(name, entry));

      if(result.second){
        // Duplicate entry
        const std::type_info& ti = result.first->second->get_typeinfo();
        if(ti != typeid(Derived)){
          std::string n1 = typeid(Derived).name();
          std::string n2 = ti.name();
          throw InternalError("Distinct types (" + n1 + " and " + n2 + ") registered under the same public classname (" + name + ")");
        }
      }

      std::pair<typename namedb_type::iterator, bool> result2;
      result2 = db->index.insert(typename namedb_type::value_type(entry->get_typeinfo().name(), entry));

      if(result2.second){
        // Duplicate entry
        const std::type_info& ti = result.first->second->get_typeinfo();
        if(ti != typeid(Derived)){
          std::string n1 = typeid(Derived).name();
          std::string n2 = ti.name();
          throw InternalError("Distinct types (" + n1 + " and " + n2 + ") registered under the same C++ type name (" + name + ")");
        }
      }
    }

    static const ClassRTTIInterface<Base>* getRTTI(const std::string& name) {
      return singleton()->lookupEntry(name);
    }
    static const ClassRTTIInterface<Base>* getRTTI(const std::type_info& ti){
      return singleton()->lookupEntry(ti);
    }

    std::string getPublicClassname() const {
      return lookupEntry(typeid(Base))->getPublicClassname();
    }
  private:
    static ClassIndex<Base>* singleton() {
      if(!singleton_instance)
        singleton_instance = new ClassIndex<Base>();
      return singleton_instance;
    }
    const ClassRTTIInterface<Base>* lookupEntry(const std::string& name) const {
      typename std::map<std::string, ClassRTTIInterface<Base>*>::const_iterator iter = index.find(name);
      if(iter == index.end())
        return 0;
      return iter->second;
    }
    const ClassRTTIInterface<Base>* lookupEntry(const std::type_info& ti) const {
      typename std::map<std::string, ClassRTTIInterface<Base>*>::const_iterator iter = index.find(ti.name());
      if(iter == index.end())
        return 0;
      return iter->second;
    }

    static ClassIndex<Base>* singleton_instance;
    typedef std::map<std::string, ClassRTTIInterface<Base>*> namedb_type;
    namedb_type index;
  };

  template<class Class>
  ClassIndex<Class>* ClassIndex<Class>::singleton_instance;


  template<class T>
  bool PointerWrapper<T>::createObject(const std::string& classname,
                                       PointerWrapperInterface** derivedptr)
  {
    const ClassRTTIInterface<T>* new_rtti = ClassIndex<T>::getRTTI(classname);
    if(!new_rtti)
      return false;
    ptr = new_rtti->createObject(derivedptr);
    if(!ptr)
      return false;
    rtti = new_rtti;
    return true;
  }

  // Helper classes for implementing MantaRTTI specializations
  template<class T>
  class MantaRTTI_readwriteMethod {
  public:
    static void readwrite(ArchiveElement* archive, T& data) {
      data.readwrite(archive);
    }
    static PersistentStorage::StorageHint storageHint() {
      return PersistentStorage::Default;
    }
  private:
    MantaRTTI_readwriteMethod();
  };

  template<class T>
  class MantaRTTI_readwriteMethod_lightweight {
  public:
    static void readwrite(ArchiveElement* archive, T& data) {
      data.readwrite(archive);
    }
    static PersistentStorage::StorageHint storageHint() {
      return PersistentStorage::Lightweight;
    }
  private:
    MantaRTTI_readwriteMethod_lightweight();
  };

  template<class T>
  class MantaRTTI_readwriteNone {
  public:
    static void readwrite(ArchiveElement* archive, T& data) {
    }
    static PersistentStorage::StorageHint storageHint() {
      return PersistentStorage::Default;
    }
  private:
    MantaRTTI_readwriteNone();
  };

  template<class T>
  class MantaRTTI_ConcreteClass {
  public:
    static T* createInstance() {
      return new T();
    }
  private:
    MantaRTTI_ConcreteClass();
  };
  template<class T>
  class MantaRTTI_AbstractClass {
  public:
    static T* createInstance() {
      return 0;
    }
  private:
    MantaRTTI_AbstractClass();
  };

  template<class T>
  class MantaRTTI_BaseClass {
  public:
    static bool registerClass();

    template<class C>
    static void registerClassAndParents(const std::string& classname) {
      ClassIndex<T>::template registerClass<C>(classname);
    }

    static bool upcast(T* ptr, PointerWrapperInterface* destination) {
      PointerWrapper<T>* dp = dynamic_cast<PointerWrapper<T>*>(destination);
      if(dp){
        dp->setPointer(ptr);
        return true;
      } else {
        return false;
      }
    }
  private:
    MantaRTTI_BaseClass();
  };

  template<class T, class Parent>
  class MantaRTTI_DerivedClass {
  public:
    static bool registerClass();

    template<class C>
    static void registerClassAndParents(const std::string& classname) {
      ClassIndex<T>::template registerClass<C>(classname);
      MantaRTTI<Parent>::template registerClassAndParents<C>(classname);
    }

    static bool upcast(T* ptr, PointerWrapperInterface* destination) {
      PointerWrapper<T>* dp = dynamic_cast<PointerWrapper<T>*>(destination);
      if(dp){
        dp->setPointer(ptr);
        return true;
      } else {
        return MantaRTTI<Parent>::upcast(ptr, destination);
      }
    }
  private:
    MantaRTTI_DerivedClass();
    // This is a string* instead of a string to avoid initialization order issues
  };

  template<class T, class Parent>
    bool MantaRTTI_DerivedClass<T, Parent>::registerClass()
  {
    registerClassAndParents<T>(MantaRTTI<T>::getPublicClassname());
    return MantaRTTI<Parent>::force_initialize;
  }

  template<class T, class Parent1, class Parent2>
  class MantaRTTI_DerivedClass2 {
  public:
    static bool registerClass();

    template<class C>
    static void registerClassAndParents(const std::string& classname) {
      ClassIndex<T>::template registerClass<C>(classname);
      MantaRTTI<Parent1>::template registerClassAndParents<C>(classname);
      MantaRTTI<Parent2>::template registerClassAndParents<C>(classname);
    }
    static bool upcast(T* ptr, PointerWrapperInterface* destination) {
      PointerWrapper<T>* dp = dynamic_cast<PointerWrapper<T>*>(ptr);
      if(dp){
        dp->setPointer(ptr);
        return true;
      } else {
        return MantaRTTI<Parent1>::upcast(ptr, destination) || MantaRTTI<Parent2>::upcast(ptr, destination);
      }
    }
  private:
    MantaRTTI_DerivedClass2();
    // This is a string* instead of a string to avoid initialization order issues
  };

  template<class T, class Parent1, class Parent2>
    bool MantaRTTI_DerivedClass2<T, Parent1, Parent2>::registerClass()
  {
    registerClassAndParents<T>(MantaRTTI<T>::getPublicClassname());
    return MantaRTTI<Parent1>::force_initialize | MantaRTTI<Parent2>::force_initialize;
  }

  template<class T>
  bool MantaRTTI_BaseClass<T>::registerClass()
  {
    registerClassAndParents<T>(MantaRTTI<T>::getPublicClassname());
    return true;
  }

  template<>
  class MantaRTTI<float> {
  public:
    static std::string getPublicClassname() {
      return "float";
    }
  };

  template<>
  class MantaRTTI<double> {
  public:
    static std::string getPublicClassname() {
      return "double";
    }
  };

  template<>
  class MantaRTTI<int> {
  public:
    static std::string getPublicClassname() {
      return "int";
    }
  };

  template<>
  class MantaRTTI<unsigned int> {
  public:
    static std::string getPublicClassname() {
      return "unsigned int";
    }
  };
}


// Convenience macros
#define MANTA_DECLARE_RTTI_DERIVEDCLASS(declclass, baseclass, classtype, rwtype)  \
template<>\
class MantaRTTI<declclass> : public MantaRTTI_DerivedClass<declclass, baseclass>, \
                             public MantaRTTI_##classtype<declclass>, \
                             public MantaRTTI_##rwtype<declclass> \
{ \
 public: \
  static bool force_initialize; \
  static std::string getPublicClassname() \
  { \
    return #declclass; \
  } \
}

#define MANTA_DECLARE_RTTI_DERIVEDCLASS2(declclass, baseclass1, baseclass2, classtype, rwtype) \
template<>\
class MantaRTTI<declclass> : public MantaRTTI_DerivedClass2<declclass, baseclass1, baseclass2>, \
                             public MantaRTTI_##classtype<declclass>, \
                             public MantaRTTI_##rwtype<declclass> \
{ \
 public: \
  static bool force_initialize; \
  static std::string getPublicClassname() \
  { \
    return #declclass; \
  } \
}

#define MANTA_DECLARE_RTTI_BASECLASS(declclass, classtype, rwtype)  \
template<>\
class MantaRTTI<declclass> : public MantaRTTI_BaseClass<declclass>, \
                             public MantaRTTI_##classtype<declclass>, \
                             public MantaRTTI_##rwtype<declclass> \
{ \
 public: \
  static bool force_initialize; \
  static std::string getPublicClassname() \
  { \
    return #declclass; \
  } \
}


#define MANTA_REGISTER_CLASS(classname) \
bool MantaRTTI<classname>::force_initialize = MantaRTTI<classname>::registerClass()

#endif
