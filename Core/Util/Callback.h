/*
  This file was automatically generated.  Don't modify by hand.

  For a detailed explaination of the Callback construct please see:

  doc/Callbacks.txt

  Briefly explained:

  1.  Only void return functions are supported.
  2.  Data is the call time bound arguments.
  3.  Arg  is the callback creation time bound arguments.
  4.  Data parameters preceed the Arg parameters of the callback function.
  5.  If you don't find a create function that match the number of
  Data and Arg parameters you need, add the create function and
  corresponding Callback_XData_XArg class.

*/

#ifndef Manta_Interface_Callback_h
#define Manta_Interface_Callback_h

#include <Core/Util/CallbackHelpers.h>

namespace Manta {
  class Callback {
  public:
    //////////////////////////////
    // Global functions or static class member functions

    // 0 call time args --- 0 creation time args
    static
    CallbackBase_0Data*
    create(void (*pmf)()) {
      return new Callback_Static_0Data_0Arg(pmf);
    }

    // 0 call time args --- 1 creation time args
    template<typename Arg1> static
    CallbackBase_0Data*
    create(void (*pmf)(Arg1), Arg1 arg1) {
      return new Callback_Static_0Data_1Arg<Arg1>(pmf, arg1);
    }

    // 0 call time args --- 2 creation time args
    template<typename Arg1, typename Arg2> static
    CallbackBase_0Data*
    create(void (*pmf)(Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_Static_0Data_2Arg<Arg1, Arg2>(pmf, arg1, arg2);
    }

    // 0 call time args --- 3 creation time args
    template<typename Arg1, typename Arg2, typename Arg3> static
    CallbackBase_0Data*
    create(void (*pmf)(Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3) {
      return new Callback_Static_0Data_3Arg<Arg1, Arg2, Arg3>(pmf, arg1, arg2, arg3);
    }

    // 0 call time args --- 5 creation time args
    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> static
    CallbackBase_0Data*
    create(void (*pmf)(Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {
      return new Callback_Static_0Data_5Arg<Arg1, Arg2, Arg3, Arg4, Arg5>(pmf, arg1, arg2, arg3, arg4, arg5);
    }

    // 1 call time args --- 0 creation time args
    template<typename Data1> static
    CallbackBase_1Data<Data1>*
    create(void (*pmf)(Data1)) {
      return new Callback_Static_1Data_0Arg<Data1>(pmf);
    }

    // 1 call time args --- 1 creation time args
    template<typename Data1, typename Arg1> static
    CallbackBase_1Data<Data1>*
    create(void (*pmf)(Data1, Arg1), Arg1 arg1) {
      return new Callback_Static_1Data_1Arg<Data1, Arg1>(pmf, arg1);
    }

    // 1 call time args --- 2 creation time args
    template<typename Data1, typename Arg1, typename Arg2> static
    CallbackBase_1Data<Data1>*
    create(void (*pmf)(Data1, Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_Static_1Data_2Arg<Data1, Arg1, Arg2>(pmf, arg1, arg2);
    }

    // 2 call time args --- 0 creation time args
    template<typename Data1, typename Data2> static
    CallbackBase_2Data<Data1, Data2>*
    create(void (*pmf)(Data1, Data2)) {
      return new Callback_Static_2Data_0Arg<Data1, Data2>(pmf);
    }

    // 2 call time args --- 2 creation time args
    template<typename Data1, typename Data2, typename Arg1, typename Arg2> static
    CallbackBase_2Data<Data1, Data2>*
    create(void (*pmf)(Data1, Data2, Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_Static_2Data_2Arg<Data1, Data2, Arg1, Arg2>(pmf, arg1, arg2);
    }

    // 3 call time args --- 1 creation time args
    template<typename Data1, typename Data2, typename Data3, typename Arg1> static
    CallbackBase_3Data<Data1, Data2, Data3>*
    create(void (*pmf)(Data1, Data2, Data3, Arg1), Arg1 arg1) {
      return new Callback_Static_3Data_1Arg<Data1, Data2, Data3, Arg1>(pmf, arg1);
    }

    // 3 call time args --- 2 creation time args
    template<typename Data1, typename Data2, typename Data3, typename Arg1, typename Arg2> static
    CallbackBase_3Data<Data1, Data2, Data3>*
    create(void (*pmf)(Data1, Data2, Data3, Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_Static_3Data_2Arg<Data1, Data2, Data3, Arg1, Arg2>(pmf, arg1, arg2);
    }

    // 4 call time args --- 0 creation time args
    template<typename Data1, typename Data2, typename Data3, typename Data4> static
    CallbackBase_4Data<Data1, Data2, Data3, Data4>*
    create(void (*pmf)(Data1, Data2, Data3, Data4)) {
      return new Callback_Static_4Data_0Arg<Data1, Data2, Data3, Data4>(pmf);
    }

    // 4 call time args --- 1 creation time args
    template<typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1> static
    CallbackBase_4Data<Data1, Data2, Data3, Data4>*
    create(void (*pmf)(Data1, Data2, Data3, Data4, Arg1), Arg1 arg1) {
      return new Callback_Static_4Data_1Arg<Data1, Data2, Data3, Data4, Arg1>(pmf, arg1);
    }

    // 4 call time args --- 2 creation time args
    template<typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1, typename Arg2> static
    CallbackBase_4Data<Data1, Data2, Data3, Data4>*
    create(void (*pmf)(Data1, Data2, Data3, Data4, Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_Static_4Data_2Arg<Data1, Data2, Data3, Data4, Arg1, Arg2>(pmf, arg1, arg2);
    }

    //////////////////////////////
    // Class member functions

    // 0 call time args --- 0 creating time args
    template<class T> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)()) {
      return new Callback_0Data_0Arg<T>(ptr, pmf);
    }

    // 0 call time args --- 1 creating time args
    template<class T, typename Arg1> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)(Arg1), Arg1 arg1) {
      return new Callback_0Data_1Arg<T, Arg1>(ptr, pmf, arg1);
    }

    // 0 call time args --- 2 creating time args
    template<class T, typename Arg1, typename Arg2> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)(Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_0Data_2Arg<T, Arg1, Arg2>(ptr, pmf, arg1, arg2);
    }

    // 0 call time args --- 3 creating time args
    template<class T, typename Arg1, typename Arg2, typename Arg3> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)(Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3) {
      return new Callback_0Data_3Arg<T, Arg1, Arg2, Arg3>(ptr, pmf, arg1, arg2, arg3);
    }

    // 0 call time args --- 4 creating time args
    template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)(Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
      return new Callback_0Data_4Arg<T, Arg1, Arg2, Arg3, Arg4>(ptr, pmf, arg1, arg2, arg3, arg4);
    }

    // 0 call time args --- 5 creating time args
    template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)(Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {
      return new Callback_0Data_5Arg<T, Arg1, Arg2, Arg3, Arg4, Arg5>(ptr, pmf, arg1, arg2, arg3, arg4, arg5);
    }

    // 1 call time args --- 0 creating time args
    template<class T, typename Data1> static
    CallbackBase_1Data<Data1>*
    create(T* ptr, void (T::*pmf)(Data1)) {
      return new Callback_1Data_0Arg<T, Data1>(ptr, pmf);
    }

    // 1 call time args --- 1 creating time args
    template<class T, typename Data1, typename Arg1> static
    CallbackBase_1Data<Data1>*
    create(T* ptr, void (T::*pmf)(Data1, Arg1), Arg1 arg1) {
      return new Callback_1Data_1Arg<T, Data1, Arg1>(ptr, pmf, arg1);
    }

    // 1 call time args --- 2 creating time args
    template<class T, typename Data1, typename Arg1, typename Arg2> static
    CallbackBase_1Data<Data1>*
    create(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_1Data_2Arg<T, Data1, Arg1, Arg2>(ptr, pmf, arg1, arg2);
    }

    // 1 call time args --- 3 creating time args
    template<class T, typename Data1, typename Arg1, typename Arg2, typename Arg3> static
    CallbackBase_1Data<Data1>*
    create(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3) {
      return new Callback_1Data_3Arg<T, Data1, Arg1, Arg2, Arg3>(ptr, pmf, arg1, arg2, arg3);
    }

    // 1 call time args --- 4 creating time args
    template<class T, typename Data1, typename Arg1, typename Arg2, typename Arg3, typename Arg4> static
    CallbackBase_1Data<Data1>*
    create(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2, Arg3, Arg4), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
      return new Callback_1Data_4Arg<T, Data1, Arg1, Arg2, Arg3, Arg4>(ptr, pmf, arg1, arg2, arg3, arg4);
    }

    // 1 call time args --- 5 creating time args
    template<class T, typename Data1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> static
    CallbackBase_1Data<Data1>*
    create(T* ptr, void (T::*pmf)(Data1, Arg1, Arg2, Arg3, Arg4, Arg5), Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {
      return new Callback_1Data_5Arg<T, Data1, Arg1, Arg2, Arg3, Arg4, Arg5>(ptr, pmf, arg1, arg2, arg3, arg4, arg5);
    }

    // 2 call time args --- 0 creating time args
    template<class T, typename Data1, typename Data2> static
    CallbackBase_2Data<Data1, Data2>*
    create(T* ptr, void (T::*pmf)(Data1, Data2)) {
      return new Callback_2Data_0Arg<T, Data1, Data2>(ptr, pmf);
    }

    // 2 call time args --- 1 creating time args
    template<class T, typename Data1, typename Data2, typename Arg1> static
    CallbackBase_2Data<Data1, Data2>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Arg1), Arg1 arg1) {
      return new Callback_2Data_1Arg<T, Data1, Data2, Arg1>(ptr, pmf, arg1);
    }

    // 2 call time args --- 2 creating time args
    template<class T, typename Data1, typename Data2, typename Arg1, typename Arg2> static
    CallbackBase_2Data<Data1, Data2>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_2Data_2Arg<T, Data1, Data2, Arg1, Arg2>(ptr, pmf, arg1, arg2);
    }

    // 2 call time args --- 3 creating time args
    template<class T, typename Data1, typename Data2, typename Arg1, typename Arg2, typename Arg3> static
    CallbackBase_2Data<Data1, Data2>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Arg1, Arg2, Arg3), Arg1 arg1, Arg2 arg2, Arg3 arg3) {
      return new Callback_2Data_3Arg<T, Data1, Data2, Arg1, Arg2, Arg3>(ptr, pmf, arg1, arg2, arg3);
    }

    // 3 call time args --- 0 creating time args
    template<class T, typename Data1, typename Data2, typename Data3> static
    CallbackBase_3Data<Data1, Data2, Data3>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Data3)) {
      return new Callback_3Data_0Arg<T, Data1, Data2, Data3>(ptr, pmf);
    }

    // 3 call time args --- 1 creating time args
    template<class T, typename Data1, typename Data2, typename Data3, typename Arg1> static
    CallbackBase_3Data<Data1, Data2, Data3>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Arg1), Arg1 arg1) {
      return new Callback_3Data_1Arg<T, Data1, Data2, Data3, Arg1>(ptr, pmf, arg1);
    }

    // 4 call time args --- 0 creating time args
    template<class T, typename Data1, typename Data2, typename Data3, typename Data4> static
    CallbackBase_4Data<Data1, Data2, Data3, Data4>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4)) {
      return new Callback_4Data_0Arg<T, Data1, Data2, Data3, Data4>(ptr, pmf);
    }

    // 4 call time args --- 1 creating time args
    template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1> static
    CallbackBase_4Data<Data1, Data2, Data3, Data4>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Arg1), Arg1 arg1) {
      return new Callback_4Data_1Arg<T, Data1, Data2, Data3, Data4, Arg1>(ptr, pmf, arg1);
    }

    // 4 call time args --- 2 creating time args
    template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Arg1, typename Arg2> static
    CallbackBase_4Data<Data1, Data2, Data3, Data4>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Arg1, Arg2), Arg1 arg1, Arg2 arg2) {
      return new Callback_4Data_2Arg<T, Data1, Data2, Data3, Data4, Arg1, Arg2>(ptr, pmf, arg1, arg2);
    }

    // 5 call time args --- 0 creating time args
    template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Data5> static
    CallbackBase_5Data<Data1, Data2, Data3, Data4, Data5>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Data5)) {
      return new Callback_5Data_0Arg<T, Data1, Data2, Data3, Data4, Data5>(ptr, pmf);
    }

    // 6 call time args --- 0 creating time args
    template<class T, typename Data1, typename Data2, typename Data3, typename Data4, typename Data5, typename Data6> static
    CallbackBase_6Data<Data1, Data2, Data3, Data4, Data5, Data6>*
    create(T* ptr, void (T::*pmf)(Data1, Data2, Data3, Data4, Data5, Data6)) {
      return new Callback_6Data_0Arg<T, Data1, Data2, Data3, Data4, Data5, Data6>(ptr, pmf);
    }

    //////////////////////////////
    // Class member functions

#ifndef SWIG // For now these const versions won't compile in swig
    // 0 call time args --- 0 creating time args
    template<class T> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)() const) {
      return new Callback_0Data_0Arg_const<T>(ptr, pmf);
    }
#endif // #ifndef SWIG

#ifndef SWIG // For now these const versions won't compile in swig
    // 0 call time args --- 1 creating time args
    template<class T, typename Arg1> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)(Arg1) const, Arg1 arg1) {
      return new Callback_0Data_1Arg_const<T, Arg1>(ptr, pmf, arg1);
    }
#endif // #ifndef SWIG

#ifndef SWIG // For now these const versions won't compile in swig
    // 0 call time args --- 2 creating time args
    template<class T, typename Arg1, typename Arg2> static
    CallbackBase_0Data*
    create(T* ptr, void (T::*pmf)(Arg1, Arg2) const, Arg1 arg1, Arg2 arg2) {
      return new Callback_0Data_2Arg_const<T, Arg1, Arg2>(ptr, pmf, arg1, arg2);
    }
#endif // #ifndef SWIG

  private:
    Callback(const Callback&);
    Callback& operator=(const Callback&);
  }; // end class Callback
} // end namespace Manta
#endif // #ifndef Manta_Interface_Callback_h
