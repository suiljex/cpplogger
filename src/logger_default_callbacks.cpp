#include "logger_default_callbacks.hpp"

namespace slx
{
  CallbackFilename::CallbackFilename(const std::string &i_filename)
    : CallbackInterface(), file(i_filename)
  {

  }

  int CallbackFilename::CallbackFunction(const LoggerEvent &i_event)
  {
    if (file.is_open() == false)
    {
      return 1;
    }

    file << Logger::FormatTimestamp("%Y-%m-%d %H:%M:%S", i_event.time)
         << " " << std::setw(5) << g_log_level_strings.at(i_event.level)
         << " : ";
    file << i_event.data << std::endl;

    return 0;
  }

  CallbackStream::CallbackStream(std::ostream & i_stream)
    : out(i_stream)
  {

  }

  int CallbackStream::CallbackFunction(const LoggerEvent &i_event)
  {
    out << Logger::FormatTimestamp("%Y-%m-%d %H:%M:%S", i_event.time)
        << " " << std::setw(5) << g_log_level_strings.at(i_event.level)
        << " : ";
    out << i_event.data << std::endl;

    return 0;
  }

  CallbackFILE::CallbackFILE(FILE *i_file)
    : file(i_file)
  {

  }

  int CallbackFILE::CallbackFunction(const LoggerEvent &i_event)
  {
    if (file == nullptr)
    {
      return 1;
    }

    fprintf(file, "%s %-5s : %s\n", Logger::FormatTimestamp("%Y-%m-%d %H:%M:%S", i_event.time).c_str(),
            g_log_level_strings.at(i_event.level).c_str(), i_event.data.c_str());
    return 0;
  }
}