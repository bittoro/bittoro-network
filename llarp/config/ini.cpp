#include <config/ini.hpp>

#include <util/logging/logger.hpp>

#include <cctype>
#include <fstream>
#include <list>
#include <iostream>

namespace llarp
{
  bool
  ConfigParser::LoadFile(string_view fname)
  {
    std::string name{fname};
    {
      std::ifstream f(name, std::ios::in | std::ios::binary);
      if(!f.is_open())
        return false;
      f.seekg(0, std::ios::end);
      m_Data.resize(f.tellg());
      f.seekg(0, std::ios::beg);
      if(m_Data.size() == 0)
        return false;
      f.read(m_Data.data(), m_Data.size());
    }
    m_FileName = name;
    return Parse();
  }

  bool
  ConfigParser::LoadFromStr(string_view str)
  {
    m_Data.resize(str.size());
    std::copy(str.begin(), str.end(), m_Data.begin());
    m_FileName = "<anonymous string>";
    return Parse();
  }

  void
  ConfigParser::Clear()
  {
    m_Config.clear();
    m_Data.clear();
  }

  static bool
  whitespace(char ch)
  {
    return std::isspace(static_cast< unsigned char >(ch)) != 0;
  }

  bool
  ConfigParser::Parse()
  {
    std::list< String_t > lines;
    {
      auto itr = m_Data.begin();
      // split into lines
      while(itr != m_Data.end())
      {
        auto beg = itr;
        while(itr != m_Data.end() && *itr != '\n' && *itr != '\r')
          ++itr;
        lines.emplace_back(std::addressof(*beg), std::distance(beg, itr));
        if(itr == m_Data.end())
          break;
        ++itr;
      }
    }

    String_t sectName;
    size_t lineno = 0;
    for(const auto& line : lines)
    {
      lineno++;
      String_t realLine;
      auto comment = line.find_first_of(';');
      if(comment == String_t::npos)
        comment = line.find_first_of('#');
      if(comment == String_t::npos)
        realLine = line;
      else
        realLine = line.substr(0, comment);
      // blank or commented line?
      if(realLine.size() == 0)
        continue;
      // find delimiters
      auto sectOpenPos = realLine.find_first_of('[');
      auto sectClosPos = realLine.find_first_of(']');
      auto kvDelim     = realLine.find_first_of('=');
      if(sectOpenPos != String_t::npos && sectClosPos != String_t::npos
         && kvDelim == String_t::npos)
      {
        // section header

        // clamp whitespaces
        ++sectOpenPos;
        while(whitespace(realLine[sectOpenPos]) && sectOpenPos != sectClosPos)
          ++sectOpenPos;
        --sectClosPos;
        while(whitespace(realLine[sectClosPos]) && sectClosPos != sectOpenPos)
          --sectClosPos;
        // set section name
        sectName = realLine.substr(sectOpenPos, sectClosPos);
      }
      else if(kvDelim != String_t::npos)
      {
        // key value pair
        String_t::size_type k_start = 0;
        String_t::size_type k_end   = kvDelim;
        String_t::size_type v_start = kvDelim + 1;
        String_t::size_type v_end   = realLine.size() - 1;
        // clamp whitespaces
        while(whitespace(realLine[k_start]) && k_start != kvDelim)
          ++k_start;
        while(whitespace(realLine[k_end - 1]) && k_end != k_start)
          --k_end;
        while(whitespace(realLine[v_start]) && v_start != v_end)
          ++v_start;
        while(whitespace(realLine[v_end]))
          --v_end;

        // sect.k = v
        String_t k = realLine.substr(k_start, k_end - k_start);
        String_t v = realLine.substr(v_start, 1 + (v_end - v_start));
        if(k.size() == 0 || v.size() == 0)
        {
          LogError(m_FileName, " invalid line (", lineno, "): '", line, "'");
          return false;
        }
        Section_t& sect = m_Config[sectName];
        LogDebug(m_FileName, ": ", sectName, ".", k, "=", v);
        sect.emplace(k, v);
      }
      else  // malformed?
      {
        LogError(m_FileName, " invalid line (", lineno, "): '", line, "'");
        return false;
      }
    }
    return true;
  }

  void
  ConfigParser::IterAll(
      std::function< void(const String_t&, const Section_t&) > visit)
  {
    for(const auto& item : m_Config)
      visit(item.first, item.second);
  }

  bool
  ConfigParser::VisitSection(
      const char* name,
      std::function< bool(const Section_t& sect) > visit) const
  {
    auto itr = m_Config.find(name);
    if(itr == m_Config.end())
      return false;
    return visit(itr->second);
  }
}  // namespace llarp
