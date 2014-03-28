#ifndef MANTA_INTERFACE_TASK_QUEUE_H_
#define MANTA_INTERFACE_TASK_QUEUE_H_

#include <Core/Util/SpinLock.h>
#include <Interface/Task.h>

#include <queue>
#include <vector>

namespace Manta {
  class TaskQueue {
  public:
    TaskQueue();
    ///  Insert a new TaskList into the queue.
    ///  \param[in] new_work  The task list to be inserted.
    void insert(TaskList* new_work);

    ///  Attempt to grab work from the TaskQueue.
    ///  \return Returns NULL if no work is available or a valid Task* otherwise.
    Task* grabWork();
  private:
    void lock();
    void unlock();

#if WQ_USE_STL
    std::queue<TaskList*> tasks;
#else
    std::vector<TaskList*> tasks;
    std::size_t read_index;
    std::size_t write_index;
#endif

    SpinLock queue_mutex;
  };
} // end namespace Manta


#endif // MANTA_CORE_UTIL_TASK_QUEUE_H_
