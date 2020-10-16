#ifndef LOGGER_H
#define LOGGER_H

#include <list>
#include <stdarg.h>
#include <unistd.h>
#include <memory>
#include <chrono>
#include <map>

namespace slx
{
  enum LogLevel
  {
    LOG_USE_DEFAULT = -1
    , LOG_TRACE = 0
    , LOG_DEBUG
    , LOG_INFO
    , LOG_WARN
    , LOG_ERROR
    , LOG_FATAL
  };

  struct LogLevelParam
  {
    const char * level_string;
    const char * level_color;
  };

  extern const std::map<LogLevel, LogLevelParam> LogLevelParams;

  struct LoggerEvent
  {
    std::time_t time;
    std::string data;
    LogLevel level;
  };

  typedef void (*LoggerCallbackFn)(const LoggerEvent & event, void * data);

  struct LoggerCallback
  {
    LoggerCallbackFn callback_fn;
    void * data;
    LogLevel level;
  };

  class Logger
  {
  public:
    Logger();

    ~Logger();

    int SetLevel
    (
        LogLevel i_level
    );

    int GetLevel();

    int SetQuietFlag
    (
        bool i_quiet_flag
    );

    bool GetQuietFlag();

    int SetAsyncFlag
    (
        bool i_async_flag
    );

    bool GetAsyncFlag();

    std::size_t GetCallBacksCount();

    int  AddCallback
    (
        const std::shared_ptr<LoggerCallback> & i_callback
    );

    std::weak_ptr<LoggerCallback> CreateCallback
    (
        LoggerCallbackFn i_callback_fn
      , void* i_data, LogLevel i_level
    );

    std::weak_ptr<LoggerCallback> GetCallbackByIndex
    (
        std::size_t i_index
    );

    int DelCallback
    (
        const std::weak_ptr<LoggerCallback> & i_callback
    );

    int DelCallbackByIndex
    (
        std::size_t i_index
    );

    int Log
    (
        LogLevel i_level
      , const std::string & i_data
    );

    int LogFormat
    (
        LogLevel i_level
      , const char *fmt
      , ...
    );

    static std::string FormatTimestamp
    (
        const char * i_fmt
      , std::time_t i_ts
    );

    static std::string FormatTimestamp
    (
        const char * i_fmt
      , const std::tm * i_tm
    );

    static std::string FormatData
    (
        const char * i_fmt
      , ...
    );

    static std::string FormatData
    (
        const char *fmt
      , va_list args
    );

    static std::shared_ptr<LoggerCallback> CreateDefalutCallbackStream
    (
        const std::ostream & i_out
      , LogLevel i_level
    );

    static std::shared_ptr<LoggerCallback> CreateDefalutCallbackFilename
    (
        const char * i_file
      , LogLevel i_level
    );

    static std::shared_ptr<LoggerCallback> CreateDefalutCallbackFILE
    (
        FILE * i_file
      , LogLevel i_level
    );

  protected:
    int ProcessEvent
    (
        const LoggerEvent & i_event
    );

    LogLevel level;
    bool quiet_flag;
    std::list<std::shared_ptr<LoggerCallback>> callbacks;
  };
}

#endif //LOGGER_H
