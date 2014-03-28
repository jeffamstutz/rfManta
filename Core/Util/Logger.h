//
//  Logger.h
//
//  author: Carson Brownlee (brownlee@cs.NOutahSPAM.edu)
//  a threaded logging class, keeps an organized indented list
//  of timing informaiton.  Saved out to .mlog files once LOGSCLOSE is called
//
//  NOTE: currently to enable, define LOGGING_ON and enable LogManager.  If undefined, all calls to logs
//  are disabled through the preprocessor to avoid overhead
//


#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <list>
#include <cassert>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>

#include <Core/Thread/Time.h>
#include <Core/Thread/Mutex.h>


namespace Manta
{
//#define LOGGING_ON

#ifdef LOGGING_ON
#define LOGSTART(x) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->Start(x);}
#define LOGSTOP(x) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->Stop(x);}
#define LOGSTARTACCUM(x) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->StartAccum(x);}
#define LOGSTOPACCUM(x) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->StopAccum(x);}
#define LOGMSG(x) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->Message(x);}
#define LOGSTARTC(x,r,g,b) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->Start(x,r,g,b);}
#define LOGSCLOSE() {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->CloseLogs();}
#define LOGSTARTACCUMC(x, r, g, b) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->StartAccum(x, r, g, b);}

#else

#define LOGSTART(x) {}
#define LOGSTOP(x) {}
#define LOGSTARTACCUM(x) {}
#define LOGSTOPACCUM(x) {}
//#define LOGMSG(x) {}
  #define LOGMSG(x) {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->GetLogger()->Message(x);}
#define LOGSTARTC(x,r,g,b) {}
#define LOGSTARTACCUMC(x,r,g,b) {}
//#define LOGSCLOSE(x) {}
  #define LOGSCLOSE() {if (LogManager::GetSingleton()->Enabled()) LogManager::GetSingleton()->CloseLogs();}
#endif


  class LogEvent
  {
    public:
      LogEvent(std::string name, float r = 1.0, float g = 1.0, float b = 1.0)
        : _name(name), _accum(0.0), _r(r), _g(g), _b(b)
      {
        _start = _end = _last = Time::currentSeconds();
      }
      void Pause()
      {
        _accum += Time::currentSeconds() - _last;
      }
      void Resume()
      {
        _last = Time::currentSeconds();
      }
      void Stop()
      {
        _end = Time::currentSeconds();
      }
      void Add(double time)
      {
        _accum += time;
      }
      double GetTime() const
      {
        return _end - _last + _accum;
      }
      double GetStart() const { return _start; }
      double GetEnd() const { return _end; }
      bool operator==(const LogEvent& e) const
      {
        return (e._name == _name);
      }
      std::string _name;
      double _start, _end, _accum, _last;
      float _r,_g,_b;
  };

  std::ostream& operator<< (std::ostream &out, const Manta::LogEvent& e);
  //{
  //    out << "< \"" << e._name << "\" start: " << e._start << " stop: " << e._end << " seconds: " << e.GetTime() << " color: " << e._r << " " << e._g << " " << e._b << ">";
  //     return out;
  //}


  class Logger
  {
    public:
      Logger()
      {}
      void Start(std::string event, float r = 1.0f, float g = 1.0f, float b = 1.0f)
      {
        _events.push_back(LogEvent(event, r, g, b));
        _map[event] = &_events.back();
      }
      void Stop(std::string event)
      {
        LogEvent* e = _map[event];
        if (!e)
          return;
        e->Stop();
        assert(_events.size());
        std::list<LogEvent>::iterator itr = _events.end();
        //find e in _events
        do
        {
          itr--;
          if (&(*itr) == e)
            break;
        } while (itr != _events.begin());
        int dist = std::distance(itr,_events.end());
        LogEvent* accum = _mapAccum[event];
        if (accum)
          accum->Add(e->GetTime());
        else
        {
          for(int i = 0; i < dist; i++)
            _out << "\t";
          _out << *e << std::endl;
        }
        //delete e;
        _events.erase(itr);
        _map[event] = 0;
      }
      void Flush()
      {
        _out.flush();
      }
      void StartAccum(std::string event, float r = 1, float g=1, float b=1)
      {
        _accumEvents.push_back(LogEvent(event,r,g,b));
        _mapAccum[event] = &_accumEvents.back();
      }
      void StopAccum(std::string event)
      {
        LogEvent* e = _mapAccum[event];
        assert(e);
        assert(_accumEvents.size());
        std::list<LogEvent>::iterator itr = _accumEvents.end();
        //find e in _accumEvents
        do
        {
          itr--;
          if (&(*itr) == e)
            break;
        } while (itr != _accumEvents.begin());
        int dist = std::distance(_accumEvents.begin(), itr);
        for(int i = 0; i < dist; i++)
          _out << "\t";
        _out << *e << std::endl;
        _accumEvents.erase(itr);
        _mapAccum[event] = 0;

      }
      void Message(std::string msg)
      {
        _out << "< Message: " << Time::currentSeconds() << " \"" << msg << "\" >" << std::endl;
      }

      std::stringstream _out;
      //std::ostream* _out;
      std::list<LogEvent> _events, _accumEvents;
      std::map<std::string, LogEvent*> _map, _mapAccum;
  };

  class  LogManager
  {
    public:
      static LogManager* __singleton;
      static LogManager* GetSingleton();
      LogManager();
      ~LogManager() { CloseLogs(); }

      bool Enabled() { return _enabled; }
      void SetEnabled(bool st) { _enabled = st; }
      void AddLogger(std::string);
      Logger* GetLogger();
      void CloseLogs();  //Not threadsafe
      void FlushLogs();

    protected:

      std::map<std::string, Logger*> _logs;
      std::map<std::string, std::ofstream*> _outs;
      std::map<std::string, std::ofstream*> _fouts;
      std::string _namePrefix;
      bool _enabled;
      Mutex _mutex;
  };
}


#endif
