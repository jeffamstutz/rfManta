#ifndef MANTA_INTERFACE_TASK_H_
#define MANTA_INTERFACE_TASK_H_

// NOTE(boulos): Because Task and TaskList rely on Callback, they have
// to be part of Interface to avoid a cyclic dependency.

#include <vector>
#include <Core/Thread/AtomicCounter.h>
#include <Core/Util/Callback.h>

namespace Manta {
  class TaskList;

  class Task {
  public:
    typedef CallbackBase_1Data<Task*> TaskCallback;
    Task(TaskCallback* task_function);

    static const unsigned int MaxScratchpadSize = 2048;

    unsigned int task_id;
    TaskList* task_list;
    char scratchpad_data[Task::MaxScratchpadSize];

    // The function the task actually runs (always appends Task* to the end)
    TaskCallback* task_function;
    void finished();
    void run();
  };

  class TaskList {
  public:
    typedef CallbackBase_1Data<TaskList*> ReductionCallback;
    TaskList();

    // NOTE(boulos): push_back will modify your task to point to this
    // TaskList and assign it an id.
    void push_back(Task* task);
    unsigned int pop();

    void finishTask(unsigned int task_id);

    void setReduction(ReductionCallback* new_reduction);
    // After finishing all the tasks, the reduction is called by the
    // finishing thread
    ReductionCallback* reduction_function;
    std::vector<Task*> tasks;
    AtomicCounter num_assigned;
    // Need to keep track of how many tasks are left unfinished
    AtomicCounter num_remaining;
  };
} // end namespace Manta

#endif // MANTA_CORE_UTIL_TASK_H_
