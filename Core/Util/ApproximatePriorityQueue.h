#ifndef MANTA_CORE_UTIL_APPROXIMATE_PRIORITY_QUEUE_H_
#define MANTA_CORE_UTIL_APPROXIMATE_PRIORITY_QUEUE_H_

#include <Core/Util/PriorityQueue.h>

#include <Core/Math/MT_RNG.h>
#include <Core/Thread/Mutex.h>
#include <vector>

namespace Manta {

  // This class represents a priority queue that is approximate by way
  // of randomization.  A push is pushed into a priority queue chosen
  // at random, and a pop is similarly popped from another random
  // queue.  Each queue is protected by a mutex to ensure thread-safe
  // usage.  For this reason, queries like empty() or size() are
  // expensive because they require locking each queue for access.
  // However, probabilistically speaking more queues yields less
  // contention and higher approximation while less queues yields more
  // contention and less approximate priority (with the limits being
  // no contention and no priority metric or 100% contention with
  // exact priorities)

  template<class DataType, class PriorityType>
  class ApproximatePriorityQueue {
  public:
    typedef PriorityQueue<DataType, PriorityType> QueueType;

    ApproximatePriorityQueue(size_t num_queues, size_t max_threads = 64) :
      num_queues(num_queues), max_threads(max_threads) {
      rngs = new MT_RNG[max_threads];
      for (size_t i = 0; i < max_threads; i++) {
        rngs[i].seed(i * 12345 + 62284);
      }
      queues = new QueueType[num_queues];
      for (size_t i = 0; i < num_queues; i++) {
        // TODO(boulos): Number them using ostringstream? Seems pretty
        // heavyweight include...
        mutexes.push_back(new Mutex("ApproximatePriorityQueue mutex"));
      }
    }

    ~ApproximatePriorityQueue() {
      grabAllLocks();

      delete[] rngs;
      delete[] queues;

      releaseAllLocks();
    }

    // NOTE(boulos): To get an accurate "everything empty" test, we
    // must first grab all the locks and then check.  We are only
    // empty if all queues are empty, so we simply AND this result
    // across all the queues.
    bool empty() const {
      grabAllLocks();
      bool is_empty = true;
      for (size_t i = 0; i < num_queues; i++) {
        is_empty &= queues[i].empty();
      }
      releaseAllLocks();
      return is_empty;
    }

    bool empty(size_t which_queue) {
      ASSERT(which_queue < num_queues);
      grabLock(which_queue);
      bool result = queues[which_queue].empty();
      releaseLock(which_queue);
      return result;
    }

    // As above, an accurate size request requires that the queues
    // aren't changing, so we must first grab all the locks.
    size_t size() const {
      grabAllLocks();
      size_t sum = 0;
      for (size_t i = 0; i < num_queues; i++) {
        sum += queues[i].size();
      }
      releaseAllLocks();
      return sum;
    }

    size_t size(size_t which_queue) const {
      ASSERT(which_queue < num_queues);
      grabLock(which_queue);
      size_t result = queues[which_queue].size();
      releaseLock(which_queue);
      return result;
    }

    // NOTE(boulos): Reservation doesn't require grabAllLocks since we
    // can just lock for each queue.
    void reserve(size_t new_size) {
      for (size_t i = 0; i < num_queues; i++) {
        reserve(new_size, i);
      }
    }

    void reserve(size_t new_size, size_t which_queue) {
      ASSERT(which_queue < num_queues);
      grabLock(which_queue);
      queues[which_queue].reserve(new_size);
      releaseLock(which_queue);
    }

    // To actually have this entire ApproximatePriorityQueue empty, we need to
    // grab all locks first
    void clear() {
      grabAllLocks();
      for (size_t i = 0; i < num_queues; i++) {
        queues[i].clear();
      }
      releaseAllLocks();
    }

    void clear(size_t which_queue) {
      ASSERT(which_queue < num_queues);
      grabLock(which_queue);
      queues[which_queue].clear();
      releaseLock(which_queue);
    }

    // NOTE(boulos): Since top and pop might run into empty queues but
    // the actual set of queues might not be empty (as would happen
    // near the end of the priority queue usage) we retry at least
    // num_queues times.  Probabilistically speaking, we should have
    // visited all the queues.  Naturally, before actually deciding
    // there are no elements left an actual size() call should be
    // made.
    DataType top(size_t thread_id) const {
      ASSERT(thread_id < max_threads);

      size_t iterations = 0;
      do {
        size_t which_queue = rngs[thread_id].nextInt() % num_queues;
        grabLock(which_queue);
        if (!queues[which_queue].empty()) {
          // Found one
          DataType result = queues[which_queue].top();
          releaseLock(which_queue);
          return result;
        }
        releaseLock(which_queue);
      } while (iterations++ < num_queues);

      return DataType(0);
    }

    DataType pop(size_t thread_id) const {
      ASSERT(thread_id < max_threads);

      size_t iterations = 0;
      do {
        size_t which_queue = rngs[thread_id].nextInt() % num_queues;
        grabLock(which_queue);
        if (!queues[which_queue].empty()) {
          // Found one
          DataType result = queues[which_queue].pop();
          releaseLock(which_queue);
          return result;
        }
        releaseLock(which_queue);
      } while (iterations++ < num_queues);

      return DataType(0);
    }

    // NOTE(boulos): We can always push, so there's no need for the
    // looping construct in the top and pop functions.
    void push(const DataType& data, const PriorityType& priority, size_t thread_id) const {
      ASSERT(thread_id < max_threads);

      size_t which_queue = rngs[thread_id].nextInt() % num_queues;
      grabLock(which_queue);
      queues[which_queue].push(data, priority);
      releaseLock(which_queue);
    }

  private:
    inline void grabLock(size_t which_lock) const {
      mutexes[which_lock]->lock();
    }
    inline void releaseLock(size_t which_lock) const {
      mutexes[which_lock]->unlock();
    }

    void grabAllLocks() const {
      for (size_t i = 0; i < num_queues; i++) {
        grabLock(i);
      }
    }

    void releaseAllLocks() const {
      for (size_t i = 0; i < num_queues; i++) {
        releaseLock(i);
      }
    }

    size_t num_queues;
    size_t max_threads;
    MT_RNG* rngs;
    QueueType* queues;
    std::vector<Mutex*> mutexes;
  };

}; // end namespace Manta

#endif // MANTA_CORE_UTIL_APPROXIMATE_PRIORITY_QUEUE_H_
