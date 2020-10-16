#include "logger.hpp"

#include <ostream>
#include <fstream>
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


  static void DefaultLoggerCallbackStream(
      const LoggerEvent & i_event
    , void * i_data)
  {
    if (i_data == nullptr)
    {
      return;
    }

    std::ostream & out = *reinterpret_cast<std::ostream *>(i_data);

    out << Logger::FormatTimestamp("%Y-%m-%d %H:%M:%S", i_event.time)
        << " " << std::setw(5) << LogLevelParams.at(i_event.level).level_string
        << " : ";
    out << i_event.data << std::endl;
  }

  static void DefaultLoggerCallbackFilename(
      const LoggerEvent & i_event
    , void * i_data)
  {
    if (i_data == nullptr)
    {
      return;
    }

    std::ofstream fout(reinterpret_cast<char *>(i_data));
    if (fout.is_open() == false)
    {
      return;
    }

    fout << Logger::FormatTimestamp("%Y-%m-%d %H:%M:%S", i_event.time)
        << " " << std::setw(5) << LogLevelParams.at(i_event.level).level_string
        << " : ";
    fout << i_event.data << std::endl;
  }

  static void DefaultLoggerCallbackFILE(
      const LoggerEvent & i_event
    , void * i_data)
  {
    if (i_data == nullptr)
    {
      return;
    }

    FILE * file = reinterpret_cast<FILE *>(i_data);
    if (file == nullptr)
    {
      return;
    }

    fprintf(file
            , "%s %-5s : %s\n"
            , Logger::FormatTimestamp("%Y-%m-%d %H:%M:%S", i_event.time).c_str()
            , LogLevelParams.at(i_event.level).level_string
            , i_event.data.c_str());
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
    data = FormatData(i_fmt, vargs);
    va_end(vargs);

    return this->Log(i_level, data);
  }

  std::string Logger::FormatTimestamp(const char * i_fmt, std::time_t i_ts)
  {
    return FormatTimestamp(i_fmt, localtime(&i_ts));
  }

  std::string Logger::FormatTimestamp(const char * i_fmt, const std::tm * i_tm)
  {
    std::vector<char> buffer(64);
    std::size_t res;
    res = std::strftime(buffer.data(), buffer.size(), i_fmt, i_tm);
    while (res == 0)
    {
      buffer.resize(buffer.size() * 2);
      res = std::strftime(buffer.data(), buffer.size(), i_fmt, i_tm);
    }
    buffer[res] = '\0';
    return std::string(buffer.data());
  }

  std::string Logger::FormatData(const char * i_fmt, ...)
  {
    va_list vargs;
    std::string data;
    va_start(vargs, i_fmt);
    data = FormatData(i_fmt, vargs);
    va_end(vargs);
    return data;
  }

  std::string Logger::FormatData(const char * i_fmt, va_list i_args)
  {
    std::vector<char> buffer(1024);
    va_list tesm_args;
    va_copy(tesm_args, i_args);

    int res = vsnprintf(buffer.data(), buffer.size(), i_fmt, tesm_args);
    if (res < 0)
    {
      return std::string();
    }

    if ((res >= 0) && (res < static_cast<int>(buffer.size())))
    {
      va_end(tesm_args);
      return std::string(buffer.data());
    }

    buffer.resize(static_cast<size_t>(res) + 1);
    res = vsnprintf(buffer.data(), buffer.size(), i_fmt, tesm_args);
    va_end(tesm_args);

    if ((res >= 0) && (res < static_cast<int>(buffer.size())))
    {
      return std::string(buffer.data());
    }
    return std::string();
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

  std::shared_ptr<LoggerCallback> Logger::CreateDefalutCallbackStream(
      const std::ostream & i_out
    , LogLevel i_level)
  {
    return std::shared_ptr<LoggerCallback>(
            new LoggerCallback{DefaultLoggerCallbackStream
          , reinterpret_cast<void *>(const_cast<std::ostream *>(&i_out))
          , i_level});
  }

  std::shared_ptr<LoggerCallback> Logger::CreateDefalutCallbackFilename(
      const char * i_file
    , LogLevel i_level)
  {
    return std::shared_ptr<LoggerCallback>(
            new LoggerCallback{DefaultLoggerCallbackFilename
          , reinterpret_cast<void *>(const_cast<char *>(i_file))
          , i_level});
  }

  std::shared_ptr<LoggerCallback> Logger::CreateDefalutCallbackFILE(
      FILE * i_file
    , LogLevel i_level)
  {
    return std::shared_ptr<LoggerCallback>(
            new LoggerCallback{DefaultLoggerCallbackFILE
          , reinterpret_cast<void *>(i_file)
          , i_level});
  }
}
