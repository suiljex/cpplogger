#ifndef LOGLIB_LOGGER_DEFAULT_HANDLERS_HPP
#define LOGLIB_LOGGER_DEFAULT_HANDLERS_HPP

#include <fstream>

#include "logger.hpp"

namespace slx
{
  class HandlerFilename : public HandlerInterface
  {
  public:
    HandlerFilename(const std::string & i_filename);

    ~HandlerFilename() override = default;

  protected:
    int HandlerFunction(const LoggerEvent &i_event) override;

    std::fstream file;
  };

  class HandlerStream : public HandlerInterface
  {
  public:
    HandlerStream(std::ostream & i_stream);

    ~HandlerStream() override = default;

  protected:
    int HandlerFunction(const LoggerEvent &i_event) override;

    std::ostream & out;
  };

  class HandlerFILE : public HandlerInterface
  {
  public:
    HandlerFILE(FILE * i_file);

    ~HandlerFILE() override = default;

  protected:
    int HandlerFunction(const LoggerEvent &i_event) override;

    FILE * file;
  };
}

#endif //LOGLIB_LOGGER_DEFAULT_HANDLERS_HPP
