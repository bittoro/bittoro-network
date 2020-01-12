#ifndef LLARP_UTIL_OSTREAM_LOGGER_HPP
#define LLARP_UTIL_OSTREAM_LOGGER_HPP

#include <util/logging/logstream.hpp>
#include <iostream>

namespace llarp
{
  struct OStreamLogStream : public ILogStream
  {
    OStreamLogStream(bool withColours, std::ostream& out);

    ~OStreamLogStream() override = default;

    void
    PreLog(std::stringstream& s, LogLevel lvl, const char* fname, int lineno,
           const std::string& nodename) const override;

    void
    Print(LogLevel lvl, const char* tag, const std::string& msg) override;

    void
    PostLog(std::stringstream& ss) const override;

    void Tick(llarp_time_t) override
    {
    }

   private:
    bool m_withColours;
    std::ostream& m_Out;
  };
}  // namespace llarp

#endif
