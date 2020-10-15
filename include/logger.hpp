#ifndef LOGGER_H
#define LOGGER_H

#include <list>
#include <stdarg.h>
#include <memory>
#include <chrono>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <unistd.h>

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
    void* data;
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

    int AddStream
    (
        const std::ostream & i_stream
      , LogLevel i_level
    );

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

  protected:
    int ProcessEvent
    (
        const LoggerEvent & i_event
    );

    int FlushQueue();

    int StartWorker();

    int StopWorker();

    static void Worker
    (
        Logger * i_parent
    );

    LogLevel level;
    bool quiet_flag;
    std::list<std::shared_ptr<LoggerCallback>> callbacks;

    bool async_flag;
    bool worker_run;
    std::mutex events_mtx;
    std::condition_variable events_cv;
    std::queue<LoggerEvent> events;
    std::thread worker;
  };
}

#endif //LOGGER_H
