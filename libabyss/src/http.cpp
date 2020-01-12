#include <abyss/http.hpp>
#include <algorithm>
namespace abyss
{
  namespace http
  {
    bool
    HeaderReader::ProcessHeaderLine(string_view line, bool& done)
    {
      if(line.size() == 0)
      {
        done = true;
        return true;
      }
      auto idx = line.find_first_of(':');
      if(idx == string_view::npos)
        return false;
      string_view header = line.substr(0, idx);
      string_view val    = line.substr(1 + idx);
      // to lowercase
      std::string lowerHeader;
      auto itr = header.begin();
      while(itr != header.end())
      {
        lowerHeader += ::tolower(*itr);
        ++itr;
      }
      if(ShouldProcessHeader(string_view(lowerHeader)))
      {
        val = val.substr(val.find_first_not_of(' '));
        Header.Headers.emplace(lowerHeader.c_str(),
                               llarp::string_view_string(val));
      }
      return true;
    }
  }  // namespace http
}  // namespace abyss
