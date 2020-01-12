#ifndef LLARP_UTIL_LOG_STREAM_HPP
#define LLARP_UTIL_LOG_STREAM_HPP
#include <util/logging/loglevel.hpp>

#include <util/time.hpp>

#include <memory>
#include <string>
#include <sstream>

namespace llarp
{
  /// logger stream interface
  struct ILogStream
  {
    virtual ~ILogStream() = default;

    virtual void
    PreLog(std::stringstream& out, LogLevel lvl, const char* fname, int lineno,
           const std::string& nodename) const = 0;

    virtual void
    Print(LogLevel lvl, const char* filename, const std::string& msg) = 0;

    virtual void
    PostLog(std::stringstream& out) const = 0;

    virtual void
    AppendLog(LogLevel lvl, const char* fname, int lineno,
              const std::string& nodename, const std::string msg)
    {
      std::stringstream ss;
      PreLog(ss, lvl, fname, lineno, nodename);
      ss << msg;
      PostLog(ss);
      Print(lvl, fname, ss.str());
    }

    /// called every end of event loop tick
    virtual void
    Tick(llarp_time_t now) = 0;
  };

  using ILogStream_ptr = std::unique_ptr< ILogStream >;

}  // namespace llarp
#endif
