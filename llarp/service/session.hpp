#ifndef LLARP_SERVICE_SESSION_HPP
#define LLARP_SERVICE_SESSION_HPP

#include <crypto/types.hpp>
#include <path/path.hpp>
#include <service/info.hpp>
#include <service/intro.hpp>
#include <util/status.hpp>
#include <util/types.hpp>

namespace llarp
{
  namespace service
  {
    struct Session
    {
      /// the intro we have
      Introduction replyIntro;
      SharedSecret sharedKey;
      ServiceInfo remote;
      /// the intro they have
      Introduction intro;
      /// the intro remoet last sent on
      Introduction lastInboundIntro;
      llarp_time_t lastUsed = 0;
      uint64_t seqno        = 0;
      bool inbound          = false;

      util::StatusObject
      ExtractStatus() const;

      bool
      IsExpired(llarp_time_t now,
                llarp_time_t lifetime = (path::default_lifetime * 2)) const;
    };

  }  // namespace service

}  // namespace llarp

#endif
