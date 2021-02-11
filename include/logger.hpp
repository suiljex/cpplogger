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
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

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

  class HandlerInterface
  {
  public:
    HandlerInterface() = default;

    virtual ~HandlerInterface() = default;

    int HandleEvent(const LoggerEvent &i_event);

    LogLevel GetLogLevel() const;

    void SetLogLevel(LogLevel i_level);

    bool IsEnabled() const;

    void Enable();

    void Disable();

  protected:
    virtual int HandlerFunction(const LoggerEvent &i_event) = 0;

    LogLevel log_level = LogLevel::LOG_TRACE;

    bool flag_enabled = true;
  };

  typedef std::shared_ptr<HandlerInterface> tHandler;

  class Logger
  {
  public:
    Logger();

    ~Logger();

    bool IsEnabled() const;

    void Enable();

    void Disable();

    bool IsAsync() const
    {
      return flag_async;
    }

    void AsyncMode()
    {
      if (flag_async == true)
      {
        return;
      }

      worker_active = true;
      //is_signaled = false;
      worker_thread = std::thread(QueueWorker, this);

      flag_async = true;
    }

    void SyncMode()
    {
      if (flag_async == false)
      {
        return;
      }

      worker_active = false;

      //worker_mtx.lock();
      //is_signaled = true;
      worker_cv.notify_one();
      //worker_mtx.unlock();

      worker_thread.join();

      while (events_queue.empty() == false)
      {
        ProcessEvent(events_queue.front());
        events_queue.pop();
      }

      flag_async = false;
    }

    std::size_t GetHandlersCount();

    int AddHandler
    (
      const tHandler &i_handler
    );

    tHandler GetHandlerByIndex
    (
      std::size_t i_index
    );

    int DelHandler
    (
      const tHandler &i_handler
    );

    int DelHandlerByIndex
    (
      std::size_t i_index
    );

    int Log
    (
      LogLevel i_level, const std::string &i_data
    );

    int LogFmt
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

    static void QueueWorker
    (
      Logger * d_logger
    );

    bool flag_enabled = true;
    bool flag_async = false;

    std::queue<LoggerEvent> events_queue;
    std::mutex queue_mtx;

    std::thread worker_thread;
    std::mutex worker_mtx;
    std::condition_variable worker_cv;
    //bool is_signaled;
    //std::atomic<bool> is_signaled;

    std::atomic<bool> worker_active;

    std::list<tHandler> handlers;
    std::mutex handlers_mtx;
  };
}

#endif //LOGGER_H
