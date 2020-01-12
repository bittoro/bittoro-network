#include "lokinet-config.hpp"
#include <fstream>
#include <list>
#include <iostream>

namespace lokinet
{
  namespace bootserv
  {
    const char* Config::DefaultPath = "/usr/local/etc/lokinet-bootserv.ini";

    bool
    Config::LoadFile(const char* fname)
    {
      {
        std::ifstream f(fname);
        if(!f.is_open())
          return false;
        f.seekg(0, std::ios::end);
        m_Data.resize(f.tellg());
        f.seekg(0, std::ios::beg);
        if(m_Data.size() == 0)
          return false;
        f.read(m_Data.data(), m_Data.size());
      }
      return Parse();
    }

    void
    Config::Clear()
    {
      m_Config.clear();
      m_Data.clear();
    }

    bool
    Config::Parse()
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
          lines.emplace_back(std::addressof(*beg), (itr - beg));
          if(itr == m_Data.end())
            break;
          ++itr;
        }
      }

      String_t sectName;

      for(const auto& line : lines)
      {
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
          while(std::isspace(realLine[sectOpenPos])
                && sectOpenPos != sectClosPos)
            ++sectOpenPos;
          --sectClosPos;
          while(std::isspace(realLine[sectClosPos])
                && sectClosPos != sectOpenPos)
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
          while(std::isspace(realLine[k_start]) && k_start != kvDelim)
            ++k_start;
          while(std::isspace(realLine[k_end]) && k_end != k_start)
            --k_end;
          while(std::isspace(realLine[v_start]) && v_start != v_end)
            ++v_start;
          while(std::isspace(realLine[v_end]))
            --v_end;

          // sect.k = v
          String_t k      = realLine.substr(k_start, k_end);
          String_t v      = realLine.substr(v_start, v_end);
          Section_t& sect = m_Config[sectName];
          sect[k]         = v;
        }
        else  // malformed?
          return false;
      }
      return true;
    }

    bool
    Config::VisitSection(
        const char* name,
        std::function< bool(const Section_t& sect) > visit) const
    {
      auto itr = m_Config.find(name);
      if(itr == m_Config.end())
        return false;
      return visit(itr->second);
    }

  }  // namespace bootserv
}  // namespace lokinet
