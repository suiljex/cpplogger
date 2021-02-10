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

  int HandlerInterface::HandleEvent(const LoggerEvent &i_event)
  {
    if (flag_enabled == false)
    {
      return 0;
    }

    if (log_level > i_event.level)
    {
      return 0;
    }

    return HandlerFunction(i_event);
  }

  LogLevel HandlerInterface::GetLogLevel() const
  {
    return log_level;
  }

  void HandlerInterface::SetLogLevel(LogLevel i_level)
  {
    log_level = i_level;
  }

  bool HandlerInterface::IsEnabled() const
  {
    return flag_enabled;
  }

  void HandlerInterface::Enable()
  {
    flag_enabled = true;
  }

  void HandlerInterface::Disable()
  {
    flag_enabled = false;
  }

  bool Logger::IsEnabled() const
  {
    return flag_enabled;
  }

  void Logger::Enable()
  {
    flag_enabled = true;
  }

  void Logger::Disable()
  {
    flag_enabled = false;
  }

  std::size_t Logger::GetHandlersCount()
  {
    return handlers.size();
  }

  int Logger::AddHandler(const tHandler & i_handler)
  {
    handlers.push_back(i_handler);
    return 0;
  }

  tHandler Logger::GetHandlerByIndex(std::size_t i_index)
  {
    if (i_index < handlers.size())
    {
      std::size_t count = 0;
      for (auto it = handlers.begin(); it != handlers.end(); ++it, ++count)
      {
        if (count == i_index)
        {
          return *it;
        }
      }
    }

    return tHandler();
  }

  int Logger::DelHandler(const tHandler & i_handler)
  {
    for (auto it = handlers.begin(); it != handlers.end(); ++it)
    {
      if (i_handler.get() == (*it).get())
      {
        handlers.erase(it);
        return 0;
      }
    }

    return 1;
  }

  int Logger::DelHandlerByIndex(std::size_t i_index)
  {
    if (i_index < handlers.size())
    {
      std::size_t count = 0;
      for (auto it = handlers.begin(); it != handlers.end(); ++it, ++count)
      {
        if (count == i_index)
        {
          handlers.erase(it);
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

  int Logger::LogFmt(LogLevel i_level, const char *fmt, ...)
  {
    va_list vargs;
    std::string data;
    va_start(vargs, fmt);
    data = FormatData(fmt, vargs);
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
    if (flag_enabled == false)
    {
      return 0;
    }

    for (auto & handler : this->handlers)
    {
      handler->HandleEvent(i_event);
    }

    return 0;
  }
}
