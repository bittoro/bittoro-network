#ifndef LLARP_RELAY_COMMIT_HPP
#define LLARP_RELAY_COMMIT_HPP

#include <crypto/encrypted_frame.hpp>
#include <crypto/types.hpp>
#include <messages/link_message.hpp>
#include <path/path_types.hpp>
#include <pow.hpp>

#include <array>
#include <memory>
#include <utility>

namespace llarp
{
  // forward declare
  struct AbstractRouter;
  namespace path
  {
    struct PathContext;
  }

  struct LR_CommitRecord
  {
    PubKey commkey;
    RouterID nextHop;
    TunnelNonce tunnelNonce;
    PathID_t txid, rxid;

    std::unique_ptr< RouterContact > nextRC;
    std::unique_ptr< PoW > work;
    uint64_t version  = 0;
    uint64_t lifetime = 0;

    bool
    BDecode(llarp_buffer_t *buf);

    bool
    BEncode(llarp_buffer_t *buf) const;

    bool
    operator==(const LR_CommitRecord &other) const;

   private:
    bool
    OnKey(llarp_buffer_t *buffer, llarp_buffer_t *key);
  };

  struct LR_CommitMessage : public ILinkMessage
  {
    std::array< EncryptedFrame, 8 > frames;

    LR_CommitMessage(std::array< EncryptedFrame, 8 > _frames)
        : ILinkMessage(), frames(std::move(_frames))
    {
    }

    LR_CommitMessage() = default;

    ~LR_CommitMessage() override = default;

    void
    Clear() override;

    bool
    DecodeKey(const llarp_buffer_t &key, llarp_buffer_t *buf) override;

    bool
    BEncode(llarp_buffer_t *buf) const override;

    bool
    HandleMessage(AbstractRouter *router) const override;

    bool
    AsyncDecrypt(llarp::path::PathContext *context) const;

    const char *
    Name() const override
    {
      return "RelayCommit";
    }
  };
}  // namespace llarp

#endif
