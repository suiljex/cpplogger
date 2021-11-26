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
  //! Бинарный семафор
  /*!
   Реализация простейшего бинарного семафора для передачи сообщений потоку обработчику
   В стандартной библиотеке семафоры появились только в c++20
  */
  class BinarySemaphore
  {
  public:
    explicit BinarySemaphore(bool i_val = false);

    void Notify();

    void Wait();

  private:
    std::mutex mtx;
    std::condition_variable cv;
    bool notified;
  };

  struct LoggerEvent
  {
    //! Уровни важности событий
    enum class Level
    {
      TRACE = 0
      , DEBUG
      , INFO
      , WARN
      , ERROR
      , FATAL
    };

    //! Время события
    std::time_t time;

    //! Данные события
    /*
      Строка, которую необходимо залогировать
    */
    std::string data;

    //! Уровень важности события
    LoggerEvent::Level level;
  };

  typedef LoggerEvent::Level LogLVL;

  //! Текстовые константы для каждого уровня.
  extern const std::map<LoggerEvent::Level, std::string> g_log_level_strings;

  //! Абстрактный класс для описания интерфейса обработков событий
  /*!
    Потомки класса должны реализовывать метод HandlerFunction.
  */
  class HandlerInterface
  {
  public:
    HandlerInterface() = default;

    virtual ~HandlerInterface() = default;

    //! Обработать событие
    /*!
      Обрабатывает событие. Если событие ниже уровнем, чем log_level или обработчик неактивен, то событие игнорируется
      Иначе вызывается метод HandlerFunction.
      \return 0 Успех
    */
    int HandleEvent(const LoggerEvent &i_event);

    //! Получить уровень обработки событий
    /*!
      \return уровень обработки событий
    */
    LoggerEvent::Level GetLogLevel() const;

    //! Установить уровень обработки событий
    /*!
      После вызова этого метода обработчик будет орабатывать события с уровнем важности не ниже i_level
      \param i_level уровень обработки событий
    */
    void SetLogLevel(LoggerEvent::Level i_level);

    //! Получись статус обработчика
    /*!
      \return true обработчик активен
      \return false обработчик неактивен
    */
    bool IsEnabled() const;

    //! Активировать обработчик событий
    /*!
      После вызова этого метода обработчик будет орабатывать все поступающие события
    */
    void Enable();

    //! Деактивировать обработчик событий
    /*!
      После вызова этого метода обработчик будет игнорировать все поступающие события
    */
    void Disable();

  protected:
    //! Метод обработки события
    /*!
      Содержит логику обработки события. Переопределяется в дочерних классах.
      \param i_event Событие
      \return 0 Успех
    */
    virtual int HandlerFunction(const LoggerEvent &i_event) = 0;

    //! Уровень обработки событий
    LoggerEvent::Level log_level = LoggerEvent::Level::TRACE;

    //! Флаг, контролирующий, активен ли обработчик
    bool flag_enabled = true;
  };

  typedef std::shared_ptr<HandlerInterface> tHandler;

  //! Класс реализующий логгер
  class Logger
  {
  public:
    //! Режимы работы логгера
    enum class Mode
    {
      DISABLED = 0 //! Выключен
      , SYNC       //! Синхронный режим. События обратываются сразу
      , ASYNC      //! Асинхронный режим. События добавляются в очередь, которую обрабатывает отдельный поток
    };

    enum ReturnCode
    {
      RET_SUCCESS = 0
      , ERROR_HANDLER_NOT_UNIQUE
      , ERROR_HANDLER_NOT_FOUND
    };

    //! Конструктор
    /*!
      Задает ражим работы логгера. По умолчанию синхронный режим.
      /param i_mode Режим работы логгера
    */
    explicit Logger(const Logger::Mode & i_mode = Logger::Mode::SYNC);

    //! Деструктор
    /*!
      Завершает все операции. Останавливает поток обработки очереди.
    */
    ~Logger();

    //! Получить режим работы
    /*!
      \return режим работы
    */
    Logger::Mode GetMode() const;

    //! Установить режим работы
    /*!
      При смене режима с асинхронного на какой-либо другой, ожидается завершение обработки очереди.
      \param i_mode режим работы
    */
    void SetMode(const Logger::Mode & i_mode);

    //! Получить количество обработчиков
    /*!
      \return количество обработчиков
    */
    std::size_t GetHandlersCount();

    //! Получить обработчик по индексу
    /*!
      \param i_index Индекс оработчика
      \return Обработчик
      \return shared_ptr(nullptr) Если обработчик не найден
    */
    tHandler GetHandlerByIndex(std::size_t i_index);

    //! Добавить обработчик
    /*!
      Добавляет новый обработчик, если его еще нет.
      \param i_handler обработчик
      \return RET_SUCCESS Успех
      \return ERROR_HANDLER_NOT_UNIQUE Обработчик уже есть
    */
    ReturnCode AddHandler(const tHandler &i_handler);

    //! Удалить обработчик
    /*!
      \param i_handler обработчик
      \return RET_SUCCESS Успех
      \return ERROR_HANDLER_NOT_FOUND Обработчик не найден
    */
    ReturnCode DelHandler(const tHandler &i_handler);

    //! Удалить обработчик по индексу
    /*!
      \param i_index индекс обработчика
      \return RET_SUCCESS Успех
      \return ERROR_HANDLER_NOT_FOUND Обработчик не найден
    */
    ReturnCode DelHandlerByIndex(std::size_t i_index);

    //! Залогировать сообщение
    /*!
      \param i_level Уровень сообщения
      \param i_data Сообщение для логирования
      \return RET_SUCCESS Успех
    */
    ReturnCode Log(LoggerEvent::Level i_level, const std::string &i_data);

    //! Залогировать сообщение с форматом
    /*!
      Формат аналогичен printf.
      Внутри себя вызывает метод Log
      \param i_level Уровень сообщения
      \param i_fmt Строка формата
      \param ... Опциональные параметры
      \return RET_SUCCESS Успех
    */
    ReturnCode LogFmt(LoggerEvent::Level i_level, const char *i_fmt, ...);

    //! Отфоматировать метку времени
    /*!
      Формат аналогичен std::strftime.
      Внутри себя вызывает метод FormatTimestamp(const char *, const std::tm *)
      \param i_fmt Строка формата
      \param i_ts Мтека времени
      \return строка с отформатированной меткой времени
    */
    static std::string FormatTimestamp(const char *i_fmt, std::time_t i_ts);

    //! Отфоматировать метку времени
    /*!
      Формат аналогичен std::strftime.
      \param i_fmt Строка формата
      \param i_tm Мтека времени
      \return строка с отформатированной меткой времени
    */
    static std::string FormatTimestamp(const char *i_fmt, const std::tm *i_tm);

    //! Отформатировать сообщение
    /*!
      Формат аналогичен printf.
      Внутри себя вызывает метод FormatData(const char *, va_list)
      \param i_fmt Строка формата
      \param ... Опциональные параметры
      \return строка с отформатированным сообщением
    */
    static std::string FormatData(const char *i_fmt, ...);

    //! Отформатировать сообщение
    /*!
      Формат аналогичен printf.
      \param i_level Уровень сообщения
      \param i_fmt Строка формата
      \param ... Опциональные параметры
      \return строка с отформатированным сообщением
    */
    static std::string FormatData(const char *i_fmt, va_list i_args);

  protected:
    //! Обработать событие
    /*!
      Обрабатывает событие путем вызова всех обработчкиов
      \param i_event Событие
      \return RET_SUCCESS Успех
    */
    ReturnCode ProcessEvent(const LoggerEvent &i_event);

    //! Функция для потока-обработчика очереди
    /*!
      В цикле ожидает сигнала от семафора worker_sem
      Если очередь events_queue не пуста, обрабатывает каждый ее элемент методом ProcessEvent
      Если worker_active == false завержает работу
      \param d_logger Указатель на собственный объект класса
    */
    static void QueueWorker(Logger * d_logger);

    //! Режим работы логгера
    Logger::Mode mode = Logger::Mode::DISABLED;

    //! Очередь событий
    std::queue<LoggerEvent> events_queue;
    //! Мютекс для синхронизации доступа к очереди events_queue
    std::mutex queue_mtx;

    //! Поток обработки очереди events_queue
    std::thread worker_thread;
    //! Семафор для передачии сообщений потоку worker_thread
    BinarySemaphore worker_sem;
    //! Контроль работы потока
    std::atomic<bool> worker_active;

    //! Список обработчиков событий логгера
    std::list<tHandler> handlers;
    //! Мютекс для синхронизации доступа к списку handlers
    std::mutex handlers_mtx;
  };
}

#endif //LOGGER_H
