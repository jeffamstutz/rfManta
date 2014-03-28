#ifndef MANTA_CORE_UTIL_PRIORITY_QUEUE_H_
#define MANTA_CORE_UTIL_PRIORITY_QUEUE_H_

#include <Core/Util/Assert.h>

namespace Manta {

  // This class is a simple max heap that allows for push, pop,
  // reserve, empty, size, and top (similar to the STL
  // priority_queue). This class _differs_ from the STL implementation
  // in that it returns by value instead of by reference. The STL
  // avoids this due to the perceived efficiency hit of return by
  // value (a copy constructor call); however, if your DataType is an
  // integral type this is not an issue. Return by value also avoids
  // any dangling pointer issues.
  //
  // This class is also not templated on a priority/comparison
  // function. Users of this class are instead required to provide the
  // priority upon insertion. Currently, it is not possible to see the
  // priority of an item. TODO(boulos): Change this?
  //
  // In addition, this class allows for a simple clear method that
  // simply causes the queue to appear empty. This makes it useful for
  // avoiding reallocation of memory each frame.
  template<class DataType, class PriorityType>
  class PriorityQueue {
  public:
    struct PriorityQueueEntry {
      DataType data;
      PriorityType priority;
    };

    PriorityQueue(size_t default_size = 8) : qsize(0), capacity(default_size) {
      queue_data = new PriorityQueueEntry[default_size];
    }

    ~PriorityQueue() {
      delete[] queue_data;
    }

    bool empty() const {
      return qsize == 0;
    }
    size_t size() const {
      return qsize;
    }

    void reserve(size_t new_size) {
      if (new_size > capacity) {
        reallocate(new_size);
      }
    }

    void clear() {
      qsize = 0;
    }

    DataType top() const {
      ASSERT(empty() == false);
      return queue_data[0].data;
    }

    DataType pop() {
      DataType result = queue_data[0].data;

      // NOTE(boulos): This part looks for a new root node. So the part
      // where you replace queue[p] with queue[c] is actually okay (since
      // we're removing the root node).
      size_t p = 0;
      while((p<<1)+2 < qsize){
        size_t c1 = (p<<1)+1;
        size_t c2 = (p<<1)+2;
#if 0
        size_t c;
        if(queue_data[c1].priority > queue_data[c2].priority)
          c = c1;
        else
          c = c2;
#else
        size_t c = (queue_data[c1].priority > queue_data[c2].priority) ? c1 : c2;
#endif
        queue_data[p] = queue_data[c];
        p = c;
      }

      // NOTE(boulos): qsize >= 1, so this is legal and correct.
      queue_data[p] = queue_data[qsize-1];
      reheapify(p);

      qsize--;
      return result;
    }

    void push(const DataType& data, const PriorityType& priority) {
      if (qsize >= capacity) {
        // Grow by a factor of 2
        reallocate(2 * qsize);
      }
      queue_data[qsize].data = data;
      queue_data[qsize].priority = priority;
      size_t p = qsize++;
      reheapify(p);
    }

  private:
    void reheapify(size_t p) {
      while(p){
        size_t parent = (p-1)>>1;
        if(queue_data[p].priority > queue_data[parent].priority){
          // Swap
          PriorityQueueEntry tmp = queue_data[p];
          queue_data[p] = queue_data[parent];
          queue_data[parent] = tmp;
        } else {
          break;
        }
        p = parent;
      }
    }

    void reallocate(size_t new_size) {
      PriorityQueueEntry* new_data = new PriorityQueueEntry[new_size];
      ASSERT(new_data);
      for (size_t i = 0; i < qsize; i++) {
        new_data[i] = queue_data[i];
      }
      delete[] queue_data;
      queue_data = new_data;
      capacity = new_size;
    }

    size_t qsize;
    size_t capacity;
    PriorityQueueEntry* queue_data;
  };

}; // end namespace Manta

#endif // _MANTA_PRIORITY_QUEUE_H_
