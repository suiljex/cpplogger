#include "logger_default_handlers.hpp"

namespace slx
{
  HandlerFilename::HandlerFilename(const std::string &i_filename)
    : HandlerInterface(), file(i_filename)
  {

  }

  int HandlerFilename::HandlerFunction(const LoggerEvent &i_event)
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

  HandlerStream::HandlerStream(std::ostream & i_stream)
    : out(i_stream)
  {

  }

  int HandlerStream::HandlerFunction(const LoggerEvent &i_event)
  {
    out << Logger::FormatTimestamp("%Y-%m-%d %H:%M:%S", i_event.time)
        << " " << std::setw(5) << g_log_level_strings.at(i_event.level)
        << " : ";
    out << i_event.data << std::endl;

    return 0;
  }

  HandlerFILE::HandlerFILE(FILE *i_file)
    : file(i_file)
  {

  }

  int HandlerFILE::HandlerFunction(const LoggerEvent &i_event)
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