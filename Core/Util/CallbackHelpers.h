
#ifndef Manta_Interface_CallbackHelpers_h
#define Manta_Interface_CallbackHelpers_h

#include <Core/Util/CallbackHandle.h>

// You should not use these directly - use Callback::create instead
namespace Manta {
  //////////////////////////////
  // Base classes that can be used by both the static and non-static
  // Callback classes.
  //

  // 0 call time args
  class CallbackBase_0Data : public CallbackHandle {
  public:
    CallbackBase_0Data()
    {
    }
    virtual ~CallbackBase_0Data()
    {
    }
    virtual void call() = 0;
  private:
    CallbackBase_0Data(const CallbackBase_0Data&);
    CallbackBase_0Data& operator=(const CallbackBase_0Data&);
  };

  // 1 call time args
  template<typename Data1>
  class CallbackBase_1Data : public CallbackHandle {
  public:
    CallbackBase_1Data()
    {
    }
    virtual ~CallbackBase_1Data()
    {
    }
    virtual void call(Data1 data1) = 0;
  private:
    CallbackBase_1Data(const CallbackBase_1Data&);
    CallbackBase_1Data& operator=(const CallbackBase_1Data&);
  };

  // 2 call time args
  template<typename Data1, typename Data2>
  class CallbackBase_2Data : public CallbackHandle {
  public:
    CallbackBase_2Data()
    {
    }
    virtual ~CallbackBase_2Data()
    {
    }
    virtual void call(Data1 data1, Data2 data2) = 0;
  private:
    CallbackBase_2Data(const CallbackBase_2Data&);
    CallbackBase_2Data& operator=(const CallbackBase_2Data&);
  };

  // 3 call time args
  template<typename Data1, typename Data2, typename Data3>
  class CallbackBase_3Data : public CallbackHandle {
  public:
    CallbackBase_3Data()
    {
    }
    virtual ~CallbackBase_3Data()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3) = 0;
  private:
    CallbackBase_3Data(const CallbackBase_3Data&);
    CallbackBase_3Data& operator=(const CallbackBase_3Data&);
  };

  // 4 call time args
  template<typename Data1, typename Data2, typename Data3, typename Data4>
  class CallbackBase_4Data : public CallbackHandle {
  public:
    CallbackBase_4Data()
    {
    }
    virtual ~CallbackBase_4Data()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4) = 0;
  private:
    CallbackBase_4Data(const CallbackBase_4Data&);
    CallbackBase_4Data& operator=(const CallbackBase_4Data&);
  };

  // 5 call time args
  template<typename Data1, typename Data2, typename Data3, typename Data4, typename Data5>
  class CallbackBase_5Data : public CallbackHandle {
  public:
    CallbackBase_5Data()
    {
    }
    virtual ~CallbackBase_5Data()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4, Data5 data5) = 0;
  private:
    CallbackBase_5Data(const CallbackBase_5Data&);
    CallbackBase_5Data& operator=(const CallbackBase_5Data&);
  };

  // 6 call time args
  template<typename Data1, typename Data2, typename Data3, typename Data4, typename Data5, typename Data6>
  class CallbackBase_6Data : public CallbackHandle {
  public:
    CallbackBase_6Data()
    {
    }
    virtual ~CallbackBase_6Data()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4, Data5 data5, Data6 data6) = 0;
  private:
    CallbackBase_6Data(const CallbackBase_6Data&);
    CallbackBase_6Data& operator=(const CallbackBase_6Data&);
  };

  //////////////////////////////
  // Global functions or static class member functions
  //

  // 0 call time args --- 0 creating time args

  class Callback_Static_0Data_0Arg : public CallbackBase_0Data {
  public:
    Callback_Static_0Data_0Arg(void (*pmf)())
      : pmf(pmf)
    {
    }
    virtual ~Callback_Static_0Data_0Arg()
    {
    }
    virtual void call()
    {
      pmf();
    }
  private:
    void (*pmf)();
  };

  // 0 call time args --- 1 creating time args
  template<typename Arg1>
  class Callback_Static_0Data_1Arg : public CallbackBase_0Data {
  public:
    Callback_Static_0Data_1Arg(void (*pmf)(Arg1), Arg1 arg1)
      : pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_Static_0Data_1Arg()
    {
    }
    virtual void call()
    {
      pmf(arg1);
    }
  private:
    void (*pmf)(Arg1);
    Arg1 arg1;
  };

  // 0 call time args --- 2 creating time args
  template<typename Arg1, typename Arg2>
  class Callback_Static_0Data_2Arg : public CallbackBase_0Data {
  public:
    Callback_Static_0Data_2Arg(void (*pmf)(Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_Static_0Data_2Arg()
    {
    }
    virtual void call()
    {
      pmf(arg1, arg2);
    }
  private:
    void (*pmf)(Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 0 call time args --- 3 creating time args
  template<typename Arg1, typename Arg2, typename Arg3>
  class Callback_Static_0Data_3Arg : public CallbackBase_0Data {
  public:
    Callback_Static_0Data_3Arg(void (*pmf)(Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3)
      : pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3)
    {
    }
    virtual ~Callback_Static_0Data_3Arg()
    {
    }
    virtual void call()
    {
      pmf(arg1, arg2, arg3);
    }
  private:
    void (*pmf)(Arg1, Arg2, Arg3);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
  };

  // 0 call time args --- 5 creating time args
  template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
  class Callback_Static_0Data_5Arg : public CallbackBase_0Data {
  public:
    Callback_Static_0Data_5Arg(void (*pmf)(Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
      : pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4), arg5(arg5)
    {
    }
    virtual ~Callback_Static_0Data_5Arg()
    {
    }
    virtual void call()
    {
      pmf(arg1, arg2, arg3, arg4, arg5);
    }
  private:
    void (*pmf)(Arg1, Arg2, Arg3, Arg4, Arg5);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
    Arg4 arg4;
    Arg5 arg5;
  };

  // 1 call time args --- 0 creating time args
  template<typename Data1>
  class Callback_Static_1Data_0Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_Static_1Data_0Arg(void (*pmf)(Data1))
      : pmf(pmf)
    {
    }
    virtual ~Callback_Static_1Data_0Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      pmf(data1);
    }
  private:
    void (*pmf)(Data1);
  };

  // 1 call time args --- 1 creating time args
  template<typename Data1, typename Arg1>
  class Callback_Static_1Data_1Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_Static_1Data_1Arg(void (*pmf)(Data1, Arg1), Arg1 arg1)
      : pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_Static_1Data_1Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      pmf(data1, arg1);
    }
  private:
    void (*pmf)(Data1, Arg1);
    Arg1 arg1;
  };

  // 1 call time args --- 2 creating time args
  template<typename Data1, typename Arg1, typename Arg2>
  class Callback_Static_1Data_2Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_Static_1Data_2Arg(void (*pmf)(Data1, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_Static_1Data_2Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      pmf(data1, arg1, arg2);
    }
  private:
    void (*pmf)(Data1, Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 2 call time args --- 0 creating time args
  template<typename Data1, typename Data2>
  class Callback_Static_2Data_0Arg : public CallbackBase_2Data<Data1, Data2> {
  public:
    Callback_Static_2Data_0Arg(void (*pmf)(Data1, Data2))
      : pmf(pmf)
    {
    }
    virtual ~Callback_Static_2Data_0Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2)
    {
      pmf(data1, data2);
    }
  private:
    void (*pmf)(Data1, Data2);
  };

  // 2 call time args --- 2 creating time args
  template<typename Data1, typename Data2, typename Arg1, typename Arg2>
  class Callback_Static_2Data_2Arg : public CallbackBase_2Data<Data1, Data2> {
  public:
    Callback_Static_2Data_2Arg(void (*pmf)(Data1, Data2, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_Static_2Data_2Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2)
    {
      pmf(data1, data2, arg1, arg2);
    }
  private:
    void (*pmf)(Data1, Data2, Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 3 call time args --- 1 creating time args
  template<typename Data1, typename Data2, typename Data3, typename Arg1>
  class Callback_Static_3Data_1Arg : public CallbackBase_3Data<Data1, Data2, Data3> {
  public:
    Callback_Static_3Data_1Arg(void (*pmf)(Data1, Data2, Data3, Arg1), Arg1 arg1)
      : pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_Static_3Data_1Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3)
    {
      pmf(data1, data2, data3, arg1);
    }
  private:
    void (*pmf)(Data1, Data2, Data3, Arg1);
    Arg1 arg1;
  };

  // 3 call time args --- 2 creating time args
  template<typename Data1, typename Data2, typename Data3, typename Arg1, typename Arg2>
  class Callback_Static_3Data_2Arg : public CallbackBase_3Data<Data1, Data2, Data3> {
  public:
    Callback_Static_3Data_2Arg(void (*pmf)(Data1, Data2, Data3, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_Static_3Data_2Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3)
    {
      pmf(data1, data2, data3, arg1, arg2);
    }
  private:
    void (*pmf)(Data1, Data2, Data3, Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 4 call time args --- 0 creating time args
  template<typename Data1, typename Data2, typename Data3, typename Data4>
  class Callback_Static_4Data_0Arg : public CallbackBase_4Data<Data1, Data2, Data3, Data4> {
  public:
    Callback_Static_4Data_0Arg(void (*pmf)(Data1, Data2, Data3, Data4))
      : pmf(pmf)
    {
    }
    virtual ~Callback_Static_4Data_0Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4)
    {
      pmf(data1, data2, data3, data4);
    }
  private:
    void (*pmf)(Data1, Data2, Data3, Data4);
  };

  // 4 call time args --- 1 creating time args
  template<typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1>
  class Callback_Static_4Data_1Arg : public CallbackBase_4Data<Data1, Data2, Data3, Data4> {
  public:
    Callback_Static_4Data_1Arg(void (*pmf)(Data1, Data2, Data3, Data4, Arg1), Arg1 arg1)
      : pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_Static_4Data_1Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4)
    {
      pmf(data1, data2, data3, data4, arg1);
    }
  private:
    void (*pmf)(Data1, Data2, Data3, Data4, Arg1);
    Arg1 arg1;
  };

  // 4 call time args --- 2 creating time args
  template<typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1, typename Arg2>
  class Callback_Static_4Data_2Arg : public CallbackBase_4Data<Data1, Data2, Data3, Data4> {
  public:
    Callback_Static_4Data_2Arg(void (*pmf)(Data1, Data2, Data3, Data4, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_Static_4Data_2Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4)
    {
      pmf(data1, data2, data3, data4, arg1, arg2);
    }
  private:
    void (*pmf)(Data1, Data2, Data3, Data4, Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  //////////////////////////////
  // Class member functions

  // 0 call time args --- 0 creating time args
  template<class T>
  class Callback_0Data_0Arg : public CallbackBase_0Data {
  public:
    Callback_0Data_0Arg(T* ptr, void (T::*pmf)())
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_0Data_0Arg()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)();
    }
  private:
    T* ptr;
    void (T::*pmf)();
  };

  // 0 call time args --- 1 creating time args
  template<class T, typename Arg1>
  class Callback_0Data_1Arg : public CallbackBase_0Data {
  public:
    Callback_0Data_1Arg(T* ptr, void (T::*pmf)(Arg1), Arg1 arg1)
      : ptr(ptr), pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_0Data_1Arg()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)(arg1);
    }
  private:
    T* ptr;
    void (T::*pmf)(Arg1);
    Arg1 arg1;
  };

  // 0 call time args --- 2 creating time args
  template<class T, typename Arg1, typename Arg2>
  class Callback_0Data_2Arg : public CallbackBase_0Data {
  public:
    Callback_0Data_2Arg(T* ptr, void (T::*pmf)(Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_0Data_2Arg()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)(arg1, arg2);
    }
  private:
    T* ptr;
    void (T::*pmf)(Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 0 call time args --- 3 creating time args
  template<class T, typename Arg1, typename Arg2, typename Arg3>
  class Callback_0Data_3Arg : public CallbackBase_0Data {
  public:
    Callback_0Data_3Arg(T* ptr, void (T::*pmf)(Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3)
    {
    }
    virtual ~Callback_0Data_3Arg()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)(arg1, arg2, arg3);
    }
  private:
    T* ptr;
    void (T::*pmf)(Arg1, Arg2, Arg3);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
  };

  // 0 call time args --- 4 creating time args
  template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  class Callback_0Data_4Arg : public CallbackBase_0Data {
  public:
    Callback_0Data_4Arg(T* ptr, void (T::*pmf)(Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4)
    {
    }
    virtual ~Callback_0Data_4Arg()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)(arg1, arg2, arg3, arg4);
    }
  private:
    T* ptr;
    void (T::*pmf)(Arg1, Arg2, Arg3, Arg4);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
    Arg4 arg4;
  };

  // 0 call time args --- 5 creating time args
  template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
  class Callback_0Data_5Arg : public CallbackBase_0Data {
  public:
    Callback_0Data_5Arg(T* ptr, void (T::*pmf)(Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4), arg5(arg5)
    {
    }
    virtual ~Callback_0Data_5Arg()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)(arg1, arg2, arg3, arg4, arg5);
    }
  private:
    T* ptr;
    void (T::*pmf)(Arg1, Arg2, Arg3, Arg4, Arg5);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
    Arg4 arg4;
    Arg5 arg5;
  };

  // 1 call time args --- 0 creating time args
  template<class T, typename Data1>
  class Callback_1Data_0Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_1Data_0Arg(T* ptr, void (T::*pmf)(Data1))
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_1Data_0Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      (ptr->*pmf)(data1);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1);
  };

  // 1 call time args --- 1 creating time args
  template<class T, typename Data1, typename Arg1>
  class Callback_1Data_1Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_1Data_1Arg(T* ptr, void (T::*pmf)(Data1, Arg1), Arg1 arg1)
      : ptr(ptr), pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_1Data_1Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      (ptr->*pmf)(data1, arg1);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Arg1);
    Arg1 arg1;
  };

  // 1 call time args --- 2 creating time args
  template<class T, typename Data1, typename Arg1, typename Arg2>
  class Callback_1Data_2Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_1Data_2Arg(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_1Data_2Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      (ptr->*pmf)(data1, arg1, arg2);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 1 call time args --- 3 creating time args
  template<class T, typename Data1, typename Arg1, typename Arg2, typename Arg3>
  class Callback_1Data_3Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_1Data_3Arg(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3)
    {
    }
    virtual ~Callback_1Data_3Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      (ptr->*pmf)(data1, arg1, arg2, arg3);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Arg1, Arg2, Arg3);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
  };

  // 1 call time args --- 4 creating time args
  template<class T, typename Data1, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  class Callback_1Data_4Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_1Data_4Arg(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4)
    {
    }
    virtual ~Callback_1Data_4Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      (ptr->*pmf)(data1, arg1, arg2, arg3, arg4);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Arg1, Arg2, Arg3, Arg4);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
    Arg4 arg4;
  };

  // 1 call time args --- 5 creating time args
  template<class T, typename Data1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
  class Callback_1Data_5Arg : public CallbackBase_1Data<Data1> {
  public:
    Callback_1Data_5Arg(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4), arg5(arg5)
    {
    }
    virtual ~Callback_1Data_5Arg()
    {
    }
    virtual void call(Data1 data1)
    {
      (ptr->*pmf)(data1, arg1, arg2, arg3, arg4, arg5);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Arg1, Arg2, Arg3, Arg4, Arg5);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
    Arg4 arg4;
    Arg5 arg5;
  };

  // 2 call time args --- 0 creating time args
  template<class T, typename Data1, typename Data2>
  class Callback_2Data_0Arg : public CallbackBase_2Data<Data1, Data2> {
  public:
    Callback_2Data_0Arg(T* ptr, void (T::*pmf)(Data1, Data2))
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_2Data_0Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2)
    {
      (ptr->*pmf)(data1, data2);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2);
  };

  // 2 call time args --- 1 creating time args
  template<class T, typename Data1, typename Data2, typename Arg1>
  class Callback_2Data_1Arg : public CallbackBase_2Data<Data1, Data2> {
  public:
    Callback_2Data_1Arg(T* ptr, void (T::*pmf)(Data1, Data2, Arg1), Arg1 arg1)
      : ptr(ptr), pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_2Data_1Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2)
    {
      (ptr->*pmf)(data1, data2, arg1);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Arg1);
    Arg1 arg1;
  };

  // 2 call time args --- 2 creating time args
  template<class T, typename Data1, typename Data2, typename Arg1, typename Arg2>
  class Callback_2Data_2Arg : public CallbackBase_2Data<Data1, Data2> {
  public:
    Callback_2Data_2Arg(T* ptr, void (T::*pmf)(Data1, Data2, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_2Data_2Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2)
    {
      (ptr->*pmf)(data1, data2, arg1, arg2);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 2 call time args --- 3 creating time args
  template<class T, typename Data1, typename Data2, typename Arg1, typename Arg2, typename Arg3>
  class Callback_2Data_3Arg : public CallbackBase_2Data<Data1, Data2> {
  public:
    Callback_2Data_3Arg(T* ptr, void (T::*pmf)(Data1, Data2, Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2), arg3(arg3)
    {
    }
    virtual ~Callback_2Data_3Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2)
    {
      (ptr->*pmf)(data1, data2, arg1, arg2, arg3);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Arg1, Arg2, Arg3);
    Arg1 arg1;
    Arg2 arg2;
    Arg3 arg3;
  };

  // 3 call time args --- 0 creating time args
  template<class T, typename Data1, typename Data2, typename Data3>
  class Callback_3Data_0Arg : public CallbackBase_3Data<Data1, Data2, Data3> {
  public:
    Callback_3Data_0Arg(T* ptr, void (T::*pmf)(Data1, Data2, Data3))
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_3Data_0Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3)
    {
      (ptr->*pmf)(data1, data2, data3);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Data3);
  };

  // 3 call time args --- 1 creating time args
  template<class T, typename Data1, typename Data2, typename Data3, typename Arg1>
  class Callback_3Data_1Arg : public CallbackBase_3Data<Data1, Data2, Data3> {
  public:
    Callback_3Data_1Arg(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Arg1), Arg1 arg1)
      : ptr(ptr), pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_3Data_1Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3)
    {
      (ptr->*pmf)(data1, data2, data3, arg1);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Data3, Arg1);
    Arg1 arg1;
  };

  // 4 call time args --- 0 creating time args
  template<class T, typename Data1, typename Data2, typename Data3, typename Data4>
  class Callback_4Data_0Arg : public CallbackBase_4Data<Data1, Data2, Data3, Data4> {
  public:
    Callback_4Data_0Arg(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4))
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_4Data_0Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4)
    {
      (ptr->*pmf)(data1, data2, data3, data4);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Data3, Data4);
  };

  // 4 call time args --- 1 creating time args
  template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1>
  class Callback_4Data_1Arg : public CallbackBase_4Data<Data1, Data2, Data3, Data4> {
  public:
    Callback_4Data_1Arg(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Arg1), Arg1 arg1)
      : ptr(ptr), pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_4Data_1Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4)
    {
      (ptr->*pmf)(data1, data2, data3, data4, arg1);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Data3, Data4, Arg1);
    Arg1 arg1;
  };

  // 4 call time args --- 2 creating time args
  template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1, typename Arg2>
  class Callback_4Data_2Arg : public CallbackBase_4Data<Data1, Data2, Data3, Data4> {
  public:
    Callback_4Data_2Arg(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Arg1, Arg2), Arg1 arg1, Arg2 arg2)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_4Data_2Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4)
    {
      (ptr->*pmf)(data1, data2, data3, data4, arg1, arg2);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Data3, Data4, Arg1, Arg2);
    Arg1 arg1;
    Arg2 arg2;
  };

  // 5 call time args --- 0 creating time args
  template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Data5>
  class Callback_5Data_0Arg : public CallbackBase_5Data<Data1, Data2, Data3, Data4, Data5> {
  public:
    Callback_5Data_0Arg(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Data5))
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_5Data_0Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4, Data5 data5)
    {
      (ptr->*pmf)(data1, data2, data3, data4, data5);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Data3, Data4, Data5);
  };

  // 6 call time args --- 0 creating time args
  template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Data5, typename Data6>
  class Callback_6Data_0Arg : public CallbackBase_6Data<Data1, Data2, Data3, Data4, Data5, Data6> {
  public:
    Callback_6Data_0Arg(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Data5, Data6))
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_6Data_0Arg()
    {
    }
    virtual void call(Data1 data1, Data2 data2, Data3 data3, Data4 data4, Data5 data5, Data6 data6)
    {
      (ptr->*pmf)(data1, data2, data3, data4, data5, data6);
    }
  private:
    T* ptr;
    void (T::*pmf)(Data1, Data2, Data3, Data4, Data5, Data6);
  };

#ifndef SWIG // For now these const versions won't compile in swig
  //////////////////////////////
  // Class member functions

  // 0 call time args --- 0 creating time args
  template<class T>
  class Callback_0Data_0Arg_const : public CallbackBase_0Data {
  public:
    Callback_0Data_0Arg_const(T* ptr, void (T::*pmf)() const)
      : ptr(ptr), pmf(pmf)
    {
    }
    virtual ~Callback_0Data_0Arg_const()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)();
    }
  private:
    T* ptr;
    void (T::*pmf)() const;
  };

  // 0 call time args --- 1 creating time args
  template<class T, typename Arg1>
  class Callback_0Data_1Arg_const : public CallbackBase_0Data {
  public:
    Callback_0Data_1Arg_const(T* ptr, void (T::*pmf)(Arg1) const, Arg1 arg1)
      : ptr(ptr), pmf(pmf), arg1(arg1)
    {
    }
    virtual ~Callback_0Data_1Arg_const()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)(arg1);
    }
  private:
    T* ptr;
    void (T::*pmf)(Arg1) const;
    Arg1 arg1;
  };

  // 0 call time args --- 2 creating time args
  template<class T, typename Arg1, typename Arg2>
  class Callback_0Data_2Arg_const : public CallbackBase_0Data {
  public:
    Callback_0Data_2Arg_const(T* ptr, void (T::*pmf)(Arg1, Arg2) const, Arg1 arg1, Arg2 arg2)
      : ptr(ptr), pmf(pmf), arg1(arg1), arg2(arg2)
    {
    }
    virtual ~Callback_0Data_2Arg_const()
    {
    }
    virtual void call()
    {
      (ptr->*pmf)(arg1, arg2);
    }
  private:
    T* ptr;
    void (T::*pmf)(Arg1, Arg2) const;
    Arg1 arg1;
    Arg2 arg2;
  };

#endif // #ifndef SWIG

} // end namespace Manta
#endif // #ifndef Manta_Interface_CallbackHelpers_h
