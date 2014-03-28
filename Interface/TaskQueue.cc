#include <Interface/TaskQueue.h>
#include <cstdlib>

#define WQ_USE_STL 0
using namespace Manta;

TaskQueue::TaskQueue() {
#if !(WQ_USE_STL)
  tasks.resize(8, NULL);
  read_index = write_index = 0;
#endif
}

void TaskQueue::lock() {
  queue_mutex.lock();
}
void TaskQueue::unlock() {
  queue_mutex.unlock();
}

void TaskQueue::insert(TaskList* new_work) {
#if WQ_USE_STL
  lock();
  tasks.push(new_work);
  unlock();
#else
  lock();
  tasks[write_index] = new_work;
  write_index++;
  size_t cur_size = tasks.size();
  write_index &= (cur_size-1);
  if (write_index == read_index) {
    // Just hit the loop around point, so we need a bigger queue
    tasks.resize(2 * cur_size, NULL);
    // Set us to be the end of the old circular buffer
    write_index = cur_size;
    read_index = 0;
  }
  unlock();
#endif
}

Task* TaskQueue::grabWork() {
  Task* result = 0;
  lock();
#if WQ_USE_STL
  if (tasks.size() > 0) {
    TaskList* top_list = tasks.front();
    unsigned int idx = top_list->pop();
    result = top_list->tasks[idx];
    // If we grabbed the last one, remove the TaskList
    if (top_list->tasks.size() - 1 == idx) {
      //std::cerr << "Finished TaskList " << result << std::endl;
      tasks.pop();
    }
  }
#else
  // Queue is only empty (or over-full if write_index == read_index)
  if (write_index != read_index) {
    TaskList* top_list = tasks[read_index];
    unsigned int idx = top_list->pop();
    result = top_list->tasks[idx];
    // If we grabbed the last one, remove the TaskList
    if (top_list->tasks.size() - 1 == idx) {
      //std::cerr << "Finished TaskList " << result << std::endl;
      read_index++;
      read_index &= (tasks.size() - 1);
    } else {
      //std::cerr << "Returning standard work " << result << "(was index " << idx << " of " << top_list->tasks.size() << ")\n";
    }
  }
#endif
  unlock();
  return result;
}
