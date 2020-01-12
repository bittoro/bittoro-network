#ifndef LLARP_SERVICE_PROTOCOL_HPP
#define LLARP_SERVICE_PROTOCOL_HPP

#include <crypto/encrypted.hpp>
#include <crypto/types.hpp>
#include <dht/message.hpp>
#include <routing/message.hpp>
#include <service/identity.hpp>
#include <service/info.hpp>
#include <service/intro.hpp>
#include <service/handler.hpp>
#include <util/bencode.hpp>
#include <util/time.hpp>
#include <path/pathset.hpp>

#include <vector>

struct llarp_threadpool;

namespace llarp
{
  class Logic;

  namespace path
  {
    /// forward declare
    struct Path;
  }  // namespace path

  namespace service
  {
    constexpr std::size_t MAX_PROTOCOL_MESSAGE_SIZE = 2048 * 2;

    using ProtocolType = uint64_t;

    constexpr ProtocolType eProtocolControl   = 0UL;
    constexpr ProtocolType eProtocolTrafficV4 = 1UL;
    constexpr ProtocolType eProtocolTrafficV6 = 2UL;

    /// inner message
    struct ProtocolMessage
    {
      ProtocolMessage(const ConvoTag& tag);
      ProtocolMessage();
      ~ProtocolMessage();
      ProtocolType proto  = eProtocolTrafficV4;
      llarp_time_t queued = 0;
      std::vector< byte_t > payload;
      Introduction introReply;
      ServiceInfo sender;
      IDataHandler* handler = nullptr;
      ConvoTag tag;
      uint64_t seqno   = 0;
      uint64_t version = LLARP_PROTO_VERSION;

      bool
      DecodeKey(const llarp_buffer_t& key, llarp_buffer_t* val);

      bool
      BEncode(llarp_buffer_t* buf) const;

      void
      PutBuffer(const llarp_buffer_t& payload);

      static void
      ProcessAsync(path::Path_ptr p, PathID_t from,
                   std::shared_ptr< ProtocolMessage > self);

      bool
      operator<(const ProtocolMessage& other) const
      {
        return seqno < other.seqno;
      }
    };

    /// outer message
    struct ProtocolFrame final : public routing::IMessage
    {
      using Encrypted_t = Encrypted< 2048 >;
      PQCipherBlock C;
      Encrypted_t D;
      uint64_t R;
      KeyExchangeNonce N;
      Signature Z;
      PathID_t F;
      service::ConvoTag T;

      ProtocolFrame(const ProtocolFrame& other)
          : routing::IMessage()
          , C(other.C)
          , D(other.D)
          , R(other.R)
          , N(other.N)
          , Z(other.Z)
          , F(other.F)
          , T(other.T)
      {
        S       = other.S;
        version = other.version;
      }

      ProtocolFrame() : routing::IMessage()
      {
        Clear();
      }

      ~ProtocolFrame() override;

      bool
      operator==(const ProtocolFrame& other) const;

      bool
      operator!=(const ProtocolFrame& other) const
      {
        return !(*this == other);
      }

      ProtocolFrame&
      operator=(const ProtocolFrame& other);

      bool
      EncryptAndSign(const ProtocolMessage& msg, const SharedSecret& sharedkey,
                     const Identity& localIdent);

      bool
      Sign(const Identity& localIdent);

      bool
      AsyncDecryptAndVerify(
          std::shared_ptr< Logic > logic, path::Path_ptr fromPath,
          const std::shared_ptr< llarp::thread::ThreadPool >& worker,
          const Identity& localIdent, IDataHandler* handler) const;

      bool
      DecryptPayloadInto(const SharedSecret& sharedkey,
                         ProtocolMessage& into) const;

      bool
      DecodeKey(const llarp_buffer_t& key, llarp_buffer_t* val) override;

      bool
      BEncode(llarp_buffer_t* buf) const override;

      bool
      BDecode(llarp_buffer_t* buf)
      {
        return bencode_decode_dict(*this, buf);
      }

      void
      Clear() override
      {
        C.Zero();
        D.Clear();
        F.Zero();
        T.Zero();
        N.Zero();
        Z.Zero();
        R       = 0;
        version = LLARP_PROTO_VERSION;
      }

      bool
      Verify(const ServiceInfo& from) const;

      bool
      HandleMessage(routing::IMessageHandler* h,
                    AbstractRouter* r) const override;
    };
  }  // namespace service
}  // namespace llarp

#endif
