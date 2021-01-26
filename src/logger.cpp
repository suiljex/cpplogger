#include "logger.hpp"

#include <string>
#include <cstdarg>
#include <vector>
#include <ctime>
#include <map>

namespace slx
{
  extern const std::map<LogLevel, std::string> g_log_level_strings
    {
      {LogLevel::LOG_TRACE, std::string{"TRACE"}},
      {LogLevel::LOG_DEBUG, std::string{"DEBUG"}},
      {LogLevel::LOG_INFO,  std::string{"INFO"}},
      {LogLevel::LOG_WARN,  std::string{"WARN"}},
      {LogLevel::LOG_ERROR, std::string{"ERROR"}},
      {LogLevel::LOG_FATAL, std::string{"FATAL"}}
    };

  int CallbackInterface::Exec(const LoggerEvent &i_event)
  {
    if (flag_active == false)
    {
      return 0;
    }

    if (level > i_event.level)
    {
      return 0;
    }

    return CallbackFunction(i_event);
  }

  LogLevel CallbackInterface::GetLevel() const
  {
    return level;
  }

  void CallbackInterface::SetLevel(LogLevel i_level)
  {
    level = i_level;
  }

  bool CallbackInterface::GetFlagActive() const
  {
    return flag_active;
  }

  void CallbackInterface::SetFlagActive(bool i_flag)
  {
    flag_active = i_flag;
  }

  int Logger::SetQuietFlag(bool i_quiet_flag)
  {
    flag_quiet = i_quiet_flag;
    return 0;
  }

  bool Logger::GetQuietFlag() const
  {
    return flag_quiet;
  }

  std::size_t Logger::GetCallBacksCount()
  {
    return callbacks.size();
  }

  int Logger::AddCallback(const tCallback & i_callback)
  {
    callbacks.push_back(i_callback);
    return 0;
  }

  tCallback Logger::GetCallbackByIndex(std::size_t i_index)
  {
    if (i_index < callbacks.size())
    {
      std::size_t count = 0;
      for (auto it = callbacks.begin(); it != callbacks.end(); ++it, ++count)
      {
        if (count == i_index)
        {
          return *it;
        }
      }
    }

    return tCallback();
  }

  int Logger::DelCallback(const tCallback & i_callback)
  {
    for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
    {
      if (i_callback.get() == (*it).get())
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

  int Logger::Log(LogLevel i_level, const std::string &i_data)
  {
    LoggerEvent event;
    event.level = i_level;
    event.data = i_data;
    event.time = std::time(nullptr);

    ProcessEvent(event);

    return 0;
  }

  int Logger::LogFormat(LogLevel i_level, const char *i_fmt, ...)
  {
    va_list vargs;
    std::string data;
    va_start(vargs, i_fmt);
    data = FormatData(i_fmt, vargs);
    va_end(vargs);

    return this->Log(i_level, data);
  }

  std::string Logger::FormatTimestamp(const char *i_fmt, std::time_t i_ts)
  {
    return FormatTimestamp(i_fmt, localtime(&i_ts));
  }

  std::string Logger::FormatTimestamp(const char *i_fmt, const std::tm *i_tm)
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

  std::string Logger::FormatData(const char *i_fmt, ...)
  {
    va_list vargs;
    std::string data;
    va_start(vargs, i_fmt);
    data = FormatData(i_fmt, vargs);
    va_end(vargs);
    return data;
  }

  std::string Logger::FormatData(const char *i_fmt, va_list i_args)
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
    if (flag_quiet == true)
    {
      return 0;
    }

    for (auto & callback : this->callbacks)
    {
      callback->Exec(i_event);
    }

    return 0;
  }
}
