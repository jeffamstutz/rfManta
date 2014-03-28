/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.


   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/



#include <Core/Thread/Thread.h>
#include <Core/Thread/Barrier.h>
#include <Core/Thread/Runnable.h>
#include <Core/Thread/ThreadGroup.h>
#include <Core/Thread/Mutex.h>
#include <Core/Thread/Time.h>
#include <Core/Thread/AtomicCounter.h>
#include <iostream>
using std::cerr;
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace Manta;

class AtomicIncrementTest : public Runnable {
    AtomicCounter* counter;
    int count;
    int proc;
    int np;
public:
  AtomicIncrementTest(AtomicCounter* barrier, int count, int proc, int np);
  virtual void run();
};

class AtomicVariableIncrementTest : public Runnable {
    AtomicCounter* counter;
    int count;
    int proc;
    int np;
public:
  AtomicVariableIncrementTest(AtomicCounter* barrier, int count, int proc, int np);
  virtual void run();
};

void usage(char* progname)
{
    cerr << "usage: " << progname << " nprocessors count\n";
}

int main(int argc, char* argv[])
{
  int np=0;
  int count=0;
  if(argc != 3){
    usage(argv[0]);
    return -1;
  }
  np=atoi(argv[1]);
  count=atoi(argv[2]);
  if (count < 1) {
    cerr << "Count must be at least 1";
    return -1;
  }

  AtomicCounter* counter=new AtomicCounter("test counter", 0);

  {
    // Test atomic increment
    ThreadGroup* increment_group=new ThreadGroup("test group");
    for(int i=0;i<np;i++){
      char buf[100];
      sprintf(buf, "worker %d", i);
      new Thread(new AtomicIncrementTest(counter, count, i, np), strdup(buf), increment_group);
    }
    increment_group->join();
    AtomicCounter& counter_ref = *counter;
    int result = counter_ref;
    int expected_increment = np * count;
    printf("adding result = %d (expected value = %d)\n", result, expected_increment);
    if (result != expected_increment) {
      return -1;
    }
  }

  {
    // Test reset of counter
    const int kResetValue = 0;
    AtomicCounter& counter_ref = *counter;
    counter_ref.set(kResetValue);
    int result = counter_ref;
    printf("Reset to %d = %d\n", kResetValue, result);
    if (result != kResetValue) {
      return -1;
    }
  }

  {
    // Test atomic variable increment (+=)
    ThreadGroup* variable_increment_group=new ThreadGroup("variable increment group");
    for(int i=0;i<np;i++){
      char buf[100];
      sprintf(buf, "worker %d", i);
      new Thread(new AtomicVariableIncrementTest(counter, count, i, np), strdup(buf), variable_increment_group);
    }
    variable_increment_group->join();

    AtomicCounter& counter_ref = *counter;
    int result = counter_ref;
    // A general arithmetic series with common difference d is computed as:
    // (n * (first + last)) / 2
    int per_thread_result = (count * (count-1)) / 2;
    int expected_result = np * per_thread_result;
    printf("adding result = %d (expected value = %d)\n", result, expected_result);
    if (result != expected_result) {
      return -1;
    }
  }

  return 0;
}

AtomicIncrementTest::AtomicIncrementTest(AtomicCounter* counter,
                                 int count, int proc, int np)
    : counter(counter), count(count), proc(proc), np(np)
{
}

void AtomicIncrementTest::run()
{
    for(int i=0;i<count;i++){
      AtomicCounter& ref_counter = *counter;
      ref_counter++;
    }
}

AtomicVariableIncrementTest::AtomicVariableIncrementTest(AtomicCounter* counter,
                                                         int count, int proc, int np)
    : counter(counter), count(count), proc(proc), np(np)
{
}

void AtomicVariableIncrementTest::run()
{
  AtomicCounter& ref_counter = *counter;
  for(int i=0;i<count;i++){
    ref_counter += i;
  }
}


