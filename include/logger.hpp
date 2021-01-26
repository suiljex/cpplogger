#ifndef LOGGER_H
#define LOGGER_H

#include <list>
#include <cstdarg>
#include <unistd.h>
#include <memory>
#include <chrono>
#include <map>
#include <fstream>
#include <iomanip>

namespace slx
{
  enum class LogLevel
  {
    LOG_TRACE = 0
    , LOG_DEBUG
    , LOG_INFO
    , LOG_WARN
    , LOG_ERROR
    , LOG_FATAL
  };

  extern const std::map<LogLevel, std::string> g_log_level_strings;

  struct LoggerEvent
  {
    std::time_t time;
    std::string data;
    LogLevel level;
  };

  class CallbackInterface
  {
  public:
    CallbackInterface() = default;

    virtual ~CallbackInterface() = default;

    int Exec(const LoggerEvent &i_event);

    LogLevel GetLevel() const;

    void SetLevel(LogLevel i_level);

    bool GetFlagActive() const;

    void SetFlagActive(bool i_flag);

  protected:
    virtual int CallbackFunction(const LoggerEvent &i_event) = 0;

    LogLevel level = LogLevel::LOG_TRACE;

    bool flag_active = true;
  };

  typedef std::shared_ptr<CallbackInterface> tCallback;

  class Logger
  {
  public:
    Logger() = default;

    ~Logger() = default;

    int SetQuietFlag
    (
      bool i_quiet_flag
    );

    bool GetQuietFlag() const;

    std::size_t GetCallBacksCount();

    int AddCallback
    (
      const tCallback &i_callback
    );

    tCallback GetCallbackByIndex
    (
      std::size_t i_index
    );

    int DelCallback
    (
      const tCallback &i_callback
    );

    int DelCallbackByIndex
    (
      std::size_t i_index
    );

    int Log
    (
      LogLevel i_level, const std::string &i_data
    );

    int LogFormat
    (
      LogLevel i_level, const char *fmt, ...
    );

    static std::string FormatTimestamp
    (
      const char *i_fmt, std::time_t i_ts
    );

    static std::string FormatTimestamp
    (
      const char *i_fmt, const std::tm *i_tm
    );

    static std::string FormatData
    (
      const char *i_fmt, ...
    );

    static std::string FormatData
    (
      const char *fmt, va_list args
    );

  protected:
    int ProcessEvent
    (
      const LoggerEvent &i_event
    );

    bool flag_quiet = false;

    std::list<tCallback> callbacks;
  };
}

#endif //LOGGER_H
