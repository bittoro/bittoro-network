#ifndef LLARP_SERVICE_ASYNC_KEY_EXCHANGE_HPP
#define LLARP_SERVICE_ASYNC_KEY_EXCHANGE_HPP

#include <crypto/types.hpp>
#include <service/identity.hpp>
#include <service/protocol.hpp>

namespace llarp
{
  class Logic;

  namespace service
  {
    struct AsyncKeyExchange
        : public std::enable_shared_from_this< AsyncKeyExchange >
    {
      std::shared_ptr< Logic > logic;
      SharedSecret sharedKey;
      ServiceInfo m_remote;
      const Identity& m_LocalIdentity;
      ProtocolMessage msg;
      Introduction intro;
      const PQPubKey introPubKey;
      Introduction remoteIntro;
      std::function< void(std::shared_ptr< ProtocolFrame >) > hook;
      IDataHandler* handler;
      ConvoTag tag;

      AsyncKeyExchange(std::shared_ptr< Logic > l, ServiceInfo r,
                       const Identity& localident,
                       const PQPubKey& introsetPubKey,
                       const Introduction& remote, IDataHandler* h,
                       const ConvoTag& t, ProtocolType proto);

      static void
      Result(std::shared_ptr< AsyncKeyExchange > user,
             std::shared_ptr< ProtocolFrame > frame);

      /// given protocol message make protocol frame
      static void
      Encrypt(std::shared_ptr< AsyncKeyExchange > user,
              std::shared_ptr< ProtocolFrame > frame);
    };

  }  // namespace service
}  // namespace llarp

#endif
