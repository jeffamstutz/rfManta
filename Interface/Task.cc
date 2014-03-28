#include <Interface/Task.h>

using namespace Manta;

Task::Task(TaskCallback* task_function) :
  task_list(0),
  task_function(task_function) {
}

void Task::finished() {
  task_list->finishTask(task_id);
}

void Task::run() {
  task_function->call(this);
}

TaskList::TaskList() :
  reduction_function(0),
  num_assigned("TaskList Assignment Count", 0),
  num_remaining("TaskList Remaining Count", 0) {
  tasks.reserve(2);
}

void TaskList::push_back(Task* task) {
  task->task_list = this;
  task->task_id = static_cast<unsigned int>(tasks.size());

  tasks.push_back(task);
  num_remaining++;
}

unsigned int TaskList::pop() {
  return num_assigned++;
}

void TaskList::finishTask(unsigned int task_id) {
  unsigned int num_left = --num_remaining;
  if (num_left == 0) {
    // Call the reduction function
    if (reduction_function)
      reduction_function->call(this);
  }
}

void TaskList::setReduction(ReductionCallback* new_reduction) {
  reduction_function = new_reduction;
}
