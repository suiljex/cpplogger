#include "logger.hpp"

#include <string>
#include <cstdarg>
#include <vector>
#include <ctime>
#include <map>

namespace slx
{
  extern const std::map<LoggerEvent::Level, std::string> g_log_level_strings
    {
      {LoggerEvent::Level::TRACE,     std::string{"TRACE"}},
      {LoggerEvent::Level::DEBUG,     std::string{"DEBUG"}},
      {LoggerEvent::Level::INFO,      std::string{"INFO"}},
      {LoggerEvent::Level::WARN,      std::string{"WARN"}},
      {LoggerEvent::Level::ERROR,     std::string{"ERROR"}},
      {LoggerEvent::Level::FATAL,     std::string{"FATAL"}}
    };

  BinarySemaphore::BinarySemaphore(bool i_val)
    : notified(i_val)
  {

  }

  void BinarySemaphore::Notify()
  {
    std::unique_lock<std::mutex> lck(mtx);
    notified = true;
    cv.notify_one();
  }

  void BinarySemaphore::Wait()
  {
    std::unique_lock<std::mutex> lck(mtx);
    while (notified == false)
    {
      cv.wait(lck);
    }

    notified = false;
  }

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

  LoggerEvent::Level HandlerInterface::GetLogLevel() const
  {
    return log_level;
  }

  void HandlerInterface::SetLogLevel(LoggerEvent::Level i_level)
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

  Logger::Logger(const Logger::Mode & i_mode)
    : worker_active(false)
  {
    SetMode(i_mode);
  }

  Logger::~Logger()
  {
    SetMode(Logger::Mode::DISABLED);
  }

  Logger::Mode Logger::GetMode() const
  {
    return mode;
  }

  void Logger::SetMode(const Logger::Mode &i_mode)
  {
    if (mode == i_mode)
    {
      return;
    }

    if (i_mode == Logger::Mode::ASYNC && mode != Logger::Mode::ASYNC)
    {
      worker_active = true;
      worker_thread = std::thread(QueueWorker, this);
    }
    else if (i_mode != Logger::Mode::ASYNC && mode == Logger::Mode::ASYNC)
    {
      worker_active = false;
      worker_sem.Notify();
      worker_thread.join();

      while (events_queue.empty() == false)
      {
        ProcessEvent(events_queue.front());
        events_queue.pop();
      }
    }
    mode = i_mode;
  }

  std::size_t Logger::GetHandlersCount()
  {
    std::unique_lock<std::mutex> handlers_lock(handlers_mtx);

    return handlers.size();
  }

  tHandler Logger::GetHandlerByIndex(std::size_t i_index)
  {
    std::unique_lock<std::mutex> handlers_lock(handlers_mtx);

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

  Logger::ReturnCodes Logger::AddHandler(const tHandler & i_handler)
  {
    std::unique_lock<std::mutex> handlers_lock(handlers_mtx);

    for (auto handler : handlers)
    {
      if (i_handler.get() == handler.get())
      {
        return ERROR_HANDLER_NOT_UNIQUE;
      }
    }

    handlers.push_back(i_handler);
    return RET_SUCCESS;
  }

  Logger::ReturnCodes Logger::DelHandler(const tHandler & i_handler)
  {
    std::unique_lock<std::mutex> handlers_lock(handlers_mtx);

    for (auto it = handlers.begin(); it != handlers.end(); ++it)
    {
      if (i_handler.get() == (*it).get())
      {
        handlers.erase(it);
        return RET_SUCCESS;
      }
    }

    return ERROR_HANDLER_NOT_FOUND;
  }

  Logger::ReturnCodes Logger::DelHandlerByIndex(std::size_t i_index)
  {
    std::unique_lock<std::mutex> handlers_lock(handlers_mtx);

    if (i_index < handlers.size())
    {
      std::size_t count = 0;
      for (auto it = handlers.begin(); it != handlers.end(); ++it, ++count)
      {
        if (count == i_index)
        {
          handlers.erase(it);
          return RET_SUCCESS;
        }
      }
    }

    return ERROR_HANDLER_NOT_FOUND;
  }

  Logger::ReturnCodes Logger::Log(LoggerEvent::Level i_level, const std::string &i_data)
  {
    LoggerEvent event;
    event.level = i_level;
    event.data = i_data;
    event.time = std::time(nullptr);

    if (mode == Logger::Mode::SYNC)
    {
      ProcessEvent(event);
    }
    else if (mode == Logger::Mode::ASYNC)
    {
      queue_mtx.lock();
      events_queue.push(event);
      queue_mtx.unlock();

      worker_sem.Notify();
    }

    return RET_SUCCESS;
  }

  Logger::ReturnCodes Logger::LogFmt(LoggerEvent::Level i_level, const char *i_fmt, ...)
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

  Logger::ReturnCodes Logger::ProcessEvent(const LoggerEvent & i_event)
  {
    std::unique_lock<std::mutex> handlers_lock(handlers_mtx);

    for (auto & handler : this->handlers)
    {
      handler->HandleEvent(i_event);
    }

    return RET_SUCCESS;
  }

  void Logger::QueueWorker(Logger *d_logger)
  {
    while (d_logger->worker_active == true)
    {
      d_logger->worker_sem.Wait();

      LoggerEvent temp_event;

      d_logger->queue_mtx.lock();
      while (d_logger->events_queue.empty() == false)
      {
        temp_event = d_logger->events_queue.front();
        d_logger->events_queue.pop();

        d_logger->queue_mtx.unlock();

        d_logger->ProcessEvent(temp_event);

        d_logger->queue_mtx.lock();
      }
      d_logger->queue_mtx.unlock();
    }
  }
}
