#ifndef MANTA_CORE_UTIL_SPINLOCK_H_
#define MANTA_CORE_UTIL_SPINLOCK_H_

#include <MachineParameters.h>

#ifndef MANTA_X86
#include <Core/Thread/Mutex.h>
#endif

namespace Manta {
#ifdef MANTA_X86
  class SpinLock {
  public:
    SpinLock() : value(0) {
    }
    int value;

    inline void unlock() {
      volatile int return_val;
      do {
        return_val = 0;
        __asm__ __volatile__(
          "lock;\n"
          "xchg %1, %0;\n" :
          "+m" (value), "+r"(return_val) :
          "m" (value) , "r" (return_val)
          /* no unknown clobbers */
          );
        // Check that we unlocked the lock (meaning that return_val = 1)
      } while (return_val == 0);

      return;
    }


    inline void lock() {
      volatile int return_val;

      do {
        return_val = 1;
        __asm__ __volatile__(
          "lock;\n"
          "xchg %1, %0;\n" :
          "+m" (value), "+r"(return_val) :
          "m" (value) , "r" (return_val)
          /* no unknown clobbers */
          );
        // Check that we got the lock, so return_val == 0 to exit
      } while (return_val == 1);
      return;
    }

    inline bool tryLock() {
      volatile int return_val = 1;

      __asm__ __volatile__(
        "lock;\n"
        "xchg %1, %0;\n" :
        "+m" (value), "+r"(return_val) :
        "m" (value) , "r" (return_val)
        /* no unknown clobbers */
        );
      return return_val == 0;
    }
  };
#else
  class SpinLock {
  public:
    SpinLock() : spin_mutex("SpinLock") {
    }

    void lock() {
      while (!(spin_mutex.tryLock())) {
      }
    }
    void unlock() {
      spin_mutex.unlock();
    }
    bool tryLock() {
      return spin_mutex.tryLock();
    }

    Mutex spin_mutex;
  };
#endif // MANTA_X86
}
#endif // MANTA_CORE_UTIL_SPINLOCK_H_
