#ifndef LOGLIB_LOGGER_DEFAULT_CALLBACKS_HPP
#define LOGLIB_LOGGER_DEFAULT_CALLBACKS_HPP

#include <fstream>

#include "logger.hpp"

namespace slx
{
  class CallbackFilename : public CallbackInterface
  {
  public:
    CallbackFilename(const std::string & i_filename);

    ~CallbackFilename() override = default;

  protected:
    int CallbackFunction(const LoggerEvent &i_event) override;

    std::fstream file;
  };

  class CallbackStream : public CallbackInterface
  {
  public:
    CallbackStream(std::ostream & i_stream);

    ~CallbackStream() override = default;

  protected:
    int CallbackFunction(const LoggerEvent &i_event) override;

    std::ostream & out;
  };

  class CallbackFILE : public CallbackInterface
  {
  public:
    CallbackFILE(FILE * i_file);

    ~CallbackFILE() override = default;

  protected:
    int CallbackFunction(const LoggerEvent &i_event) override;

    FILE * file;
  };
}

#endif //LOGLIB_LOGGER_DEFAULT_CALLBACKS_HPP
