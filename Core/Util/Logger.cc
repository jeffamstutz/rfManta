#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <Core/Thread/Thread.h>
#include <Core/Thread/ThreadGroup.h>
#include "Logger.h"

//#include <UseMPI.h>
#ifdef USE_MPI
//#include <mpi.h>
#endif

using namespace Manta;

std::ostream& Manta::operator<< (std::ostream &out, const Manta::LogEvent& e)
{
     out << "< \"" << e._name << "\" start: " << e._start << " stop: " << e._end << " seconds: " << e.GetTime() << " color: " << e._r << " " << e._g << " " << e._b << ">";
     return out;
}


LogManager* LogManager::__singleton = NULL;

LogManager* LogManager::GetSingleton()
{
  if (!__singleton)
  {
    static Mutex mutex("logmanager");
    mutex.lock();
    if (!__singleton)
    {
    __singleton = new LogManager();
    //ThreadGroup* Thread::getThreadGroup();
    std::stringstream name;
#ifdef USE_MPI
    int rank = MPI::COMM_WORLD.Get_rank();
    name << "rank_" << rank << "_";
#endif
    name << "thread_" << Thread::self()->getThreadName();
    __singleton->AddLogger(name.str());
    }
    mutex.unlock();
  }
  return __singleton;
}

LogManager::LogManager()
  : _namePrefix("log_"),  _enabled(false), _mutex("LogManager")
{
  char* str = getenv("MANTA_LOGS");
  if (str)
  {
    _namePrefix = std::string(str);
  }
}

void LogManager::AddLogger(std::string name)
{
  //_mutex.lock();
  _logs[name] = new Logger();
  //_mutex.unlock();
}

Logger* LogManager::GetLogger()
{
  //stl map is not thread-safe for reads and writes even when working with distinct entries.  A mutex would be too costly so for now it is assumed that all threads are running when logmanager is constructed

  std::stringstream sname;
#ifdef USE_MPI
  int rank = 0;
   rank = MPI::COMM_WORLD.Get_rank();
   sname << "rank_" << rank << "_";
   //sprintf(tname, "rank_%d", rank)
#endif
  sname << "thread_" << Thread::self()->getThreadName();
  std::string name;
  name = sname.str();
  if (!_logs[name])
  {
    //stl not thread safe
  _mutex.lock();
      AddLogger(name);
  _mutex.unlock();
  }
  assert(_logs[name]);
  _mutex.lock();
  Logger* logger = _logs[name];
  _mutex.unlock();
  return logger;
}

void LogManager::CloseLogs()
{
 // _mutex.lock();
  //map<string, ofstream*>::iterator itr2 = _fouts.begin();
  //for(map<string, ofstream*>::iterator itr = _outs.begin(); itr != _outs.end() && itr2 != _fouts.end(); itr++, itr2++)
  //{
 //   *(itr2->second) << itr->second->str();
  //  itr2->second->flush();
  //  itr2->second->close();
  //}
  for(std::map<std::string, Logger*>::iterator log = _logs.begin(); log != _logs.end(); log++)
  {
    std::stringstream ss;
    ss << _namePrefix << log->first << ".mlog";
    //cout << "writing log: " << ss.str() << endl;
    std::ofstream fout(ss.str().c_str());
    assert(fout);
    assert(log->second);
    fout << log->second->_out.str();
    fout.close();
//    delete log->second;
//    _logs.erase(log);
  }
 // _logs.erase(_logs.begin(), _logs.end());
 // _mutex.unlock();
}

void LogManager::FlushLogs()
{
  for(std::map<std::string, std::ofstream*>::iterator itr = _outs.begin(); itr != _outs.end(); itr++)
  {
    itr->second->flush();
  }
}
