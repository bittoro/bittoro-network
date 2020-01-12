#ifndef LLARP_SERVICE_LOOKUP_HPP
#define LLARP_SERVICE_LOOKUP_HPP

#include <routing/message.hpp>
#include <service/intro_set.hpp>
#include <path/pathset.hpp>

#include <set>

namespace llarp
{
  // forward declare
  namespace path
  {
    struct Path;
  }

  namespace service
  {
    struct ILookupHolder;

    constexpr size_t MaxConcurrentLookups = size_t(16);

    struct IServiceLookup
    {
      IServiceLookup()          = delete;
      virtual ~IServiceLookup() = default;

      /// handle lookup result
      virtual bool
      HandleResponse(__attribute__((unused))
                     const std::set< IntroSet >& results)
      {
        return false;
      }

      /// determine if this request has timed out
      bool
      IsTimedOut(llarp_time_t now, llarp_time_t timeout = 60000) const
      {
        if(now <= m_created)
          return false;
        return now - m_created > timeout;
      }

      /// build request message for service lookup
      virtual std::shared_ptr< routing::IMessage >
      BuildRequestMessage() = 0;

      /// build a new request message and send it via a path
      bool
      SendRequestViaPath(path::Path_ptr p, AbstractRouter* r);

      ILookupHolder* m_parent;
      uint64_t txid;
      const std::string name;
      RouterID endpoint;

      util::StatusObject
      ExtractStatus() const
      {
        auto now = time_now_ms();
        util::StatusObject obj{{"txid", txid},
                               {"endpoint", endpoint.ToHex()},
                               {"name", name},
                               {"timedOut", IsTimedOut(now)},
                               {"createdAt", m_created}};
        return obj;
      }

     protected:
      IServiceLookup(ILookupHolder* parent, uint64_t tx, std::string name);

      llarp_time_t m_created;
    };

    struct ILookupHolder
    {
      virtual void
      PutLookup(IServiceLookup* l, uint64_t txid) = 0;
    };

  }  // namespace service
}  // namespace llarp

#endif
