#include "logger.hpp"

#include <ostream>
#include <iomanip>
#include <string>
#include <stdarg.h>
#include <vector>
#include <chrono>
#include <ctime>
#include <map>

namespace slx {
  const std::map<LogLevel, LogLevelParam> LogLevelParams
  {
    {LogLevel::LOG_TRACE, LogLevelParam{"TRACE", "\x1b[94m"}}
    , {LogLevel::LOG_DEBUG, LogLevelParam{"DEBUG", "\x1b[36m"}}
    , {LogLevel::LOG_INFO,  LogLevelParam{"INFO",  "\x1b[32m"}}
    , {LogLevel::LOG_WARN,  LogLevelParam{"WARN",  "\x1b[33m"}}
    , {LogLevel::LOG_ERROR, LogLevelParam{"ERROR", "\x1b[31m"}}
    , {LogLevel::LOG_FATAL, LogLevelParam{"FATAL", "\x1b[35m"}}
  };

  static std::string format(const char *fmt, va_list args)
  {
    std::vector<char> v(1024);
    while (true)
    {
      va_list args2;
      va_copy(args2, args);
      int res = vsnprintf(v.data(), v.size(), fmt, args2);
      if ((res >= 0) && (res < static_cast<int>(v.size())))
      {
        va_end(args2);
        return std::string(v.data());
      }
      size_t size;
      if (res < 0)
      {
        size = v.size() * 2;
      }
      else
      {
        size = static_cast<size_t>(res) + 1;
      }
      v.clear();
      v.resize(size);
      va_end(args2);
    }
  }

  static void StreamCallbackFn(const LoggerEvent & event, void * data)
  {
    if (data == nullptr)
    {
      return;
    }

    char time_buf[64];
    time_buf[std::strftime(time_buf
                           , sizeof(time_buf)
                           , "%Y-%m-%d %H:%M:%S"
                           , localtime(&event.time))] = '\0';

    std::ostream & out = *reinterpret_cast<std::ostream *>(data);
    out << time_buf
        << " " << std::setw(5) << LogLevelParams.at(event.level).level_string
        << " : ";
    out << event.data << std::endl;
  }

  Logger::Logger()
    : level(LogLevel::LOG_TRACE)
    , quiet_flag(false)
  {

  }

  Logger::~Logger()
  {

  }

  int Logger::SetLevel(LogLevel i_level)
  {
    level = i_level;
    return 0;
  }

  int Logger::GetLevel()
  {
    return level;
  }

  int Logger::SetQuietFlag(bool i_quiet_flag)
  {
    quiet_flag = i_quiet_flag;
    return 0;
  }

  bool Logger::GetQuietFlag()
  {
    return quiet_flag;
  }

  std::size_t Logger::GetCallBacksCount()
  {
    return callbacks.size();
  }

  int Logger::AddStream(const std::ostream & i_stream, LogLevel i_level)
  {
    CreateCallback(StreamCallbackFn
                   , reinterpret_cast<void *>(const_cast<std::ostream *>(&i_stream))
                   , i_level);
    return 0;
  }

  int Logger::AddCallback(const std::shared_ptr<LoggerCallback> & i_callback)
  {
    callbacks.push_back(i_callback);
    return 0;
  }

  std::weak_ptr<LoggerCallback> Logger::CreateCallback(
      LoggerCallbackFn i_callback_fn
      , void * i_data
      , LogLevel i_level)
  {
    std::shared_ptr<LoggerCallback> new_callback(
          new LoggerCallback{i_callback_fn
                             , i_data
                             , i_level});
    callbacks.push_back(new_callback);
    return std::weak_ptr<LoggerCallback>(new_callback);
  }

  std::weak_ptr<LoggerCallback> Logger::GetCallbackByIndex(std::size_t i_index)
  {
    if (i_index < callbacks.size())
    {
      std::size_t count = 0;
      for (auto it = callbacks.begin(); it != callbacks.end(); ++it, ++count)
      {
        if (count == i_index)
        {
          return std::weak_ptr<LoggerCallback>(*it);
        }
      }
    }

    return std::weak_ptr<LoggerCallback>();
  }

  int Logger::DelCallback(const std::weak_ptr<LoggerCallback> & i_callback)
  {
    if (i_callback.expired() == true)
    {
      return 1;
    }

    for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
    {
      if (i_callback.lock().get() == (*it).get())
      {
        callbacks.erase(it);
        return 0;
      }
    }

    return 1;
  }

  int Logger::DelCallbackByIndex(std::size_t i_index)
  {
    if (i_index < callbacks.size())
    {
      std::size_t count = 0;
      for (auto it = callbacks.begin(); it != callbacks.end(); ++it, ++count)
      {
        if (count == i_index)
        {
          callbacks.erase(it);
          return 0;
        }
      }
    }

    return 1;
  }

  int Logger::Log(LogLevel i_level, const std::string & i_data)
  {
    LoggerEvent event;
    event.level = i_level;
    event.data = i_data;
    event.time = std::time(nullptr);

    ProcessEvent(event);

    return 0;
  }

  int Logger::LogFormat(LogLevel i_level, const char * i_fmt, ...)
  {
    va_list vargs;
    std::string data;
    va_start(vargs, i_fmt);
    data = format(i_fmt, vargs);
    va_end(vargs);

    return this->Log(i_level, data);
  }

  int Logger::ProcessEvent(const LoggerEvent & i_event)
  {
    for (auto it = this->callbacks.begin(); it != this->callbacks.end(); ++it)
    {
      if ((it->get()->level != LOG_USE_DEFAULT
           && i_event.level >= it->get()->level)
          ||(it->get()->level == LOG_USE_DEFAULT
             && i_event.level >= this->level && this->quiet_flag == false))
      {
        it->get()->callback_fn(i_event, it->get()->data);
      }
    }

    return 0;
  }
}
