#ifndef LLARP_DHT_MESSAGES_GOT_INTRO_HPP
#define LLARP_DHT_MESSAGES_GOT_INTRO_HPP

#include <dht/message.hpp>
#include <service/intro_set.hpp>
#include <util/copy_or_nullptr.hpp>

#include <vector>

namespace llarp
{
  namespace dht
  {
    /// acknowledgement to PublishIntroMessage or reply to FindIntroMessage
    struct GotIntroMessage : public IMessage
    {
      /// the found introsets
      std::vector< service::IntroSet > I;
      /// txid
      uint64_t T = 0;
      /// the key of a router closer in keyspace if iterative lookup
      std::unique_ptr< Key_t > K;

      GotIntroMessage(const Key_t& from) : IMessage(from)
      {
      }

      GotIntroMessage(const GotIntroMessage& other)
          : IMessage(other.From)
          , I(other.I)
          , T(other.T)
          , K(copy_or_nullptr(other.K))
      {
        version = other.version;
      }

      /// for iterative reply
      GotIntroMessage(const Key_t& from, const Key_t& closer, uint64_t txid)
          : IMessage(from), T(txid), K(new Key_t(closer))
      {
      }

      /// for recursive reply
      GotIntroMessage(std::vector< service::IntroSet > results, uint64_t txid);

      ~GotIntroMessage() override = default;

      bool
      BEncode(llarp_buffer_t* buf) const override;

      bool
      DecodeKey(const llarp_buffer_t& key, llarp_buffer_t* val) override;

      bool
      HandleMessage(llarp_dht_context* ctx,
                    std::vector< IMessage::Ptr_t >& replies) const override;
    };

    struct RelayedGotIntroMessage final : public GotIntroMessage
    {
      RelayedGotIntroMessage() : GotIntroMessage({})
      {
      }

      bool
      HandleMessage(llarp_dht_context* ctx,
                    std::vector< IMessage::Ptr_t >& replies) const override;
    };

    using GotIntroMessage_constptr = std::shared_ptr< const GotIntroMessage >;
  }  // namespace dht
}  // namespace llarp
#endif
