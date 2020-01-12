#ifndef LLARP_IWP_SESSION_HPP
#define LLARP_IWP_SESSION_HPP

#include <link/session.hpp>
#include <iwp/linklayer.hpp>
#include <iwp/message_buffer.hpp>
#include <unordered_set>
#include <deque>

namespace llarp
{
  namespace iwp
  {
    /// packet crypto overhead size
    static constexpr size_t PacketOverhead = HMACSIZE + TUNNONCESIZE;
    /// creates a packet with plaintext size + wire overhead + random pad
    ILinkSession::Packet_t
    CreatePacket(Command cmd, size_t plainsize, size_t min_pad = 16,
                 size_t pad_variance = 16);

    struct Session : public ILinkSession,
                     public std::enable_shared_from_this< Session >
    {
      /// Time how long we try delivery for
      static constexpr llarp_time_t DeliveryTimeout = 500;
      /// Time how long we wait to recieve a message
      static constexpr llarp_time_t ReceivalTimeout = (DeliveryTimeout * 8) / 5;
      /// How long to keep a replay window for
      static constexpr llarp_time_t ReplayWindow = (ReceivalTimeout * 3) / 2;
      /// How often to acks RX messages
      static constexpr llarp_time_t ACKResendInterval = DeliveryTimeout / 2;
      /// How often to retransmit TX fragments
      static constexpr llarp_time_t TXFlushInterval = (DeliveryTimeout / 5) * 4;
      /// How often we send a keepalive
      static constexpr llarp_time_t PingInterval = 5000;
      /// How long we wait for a session to die with no tx from them
      static constexpr llarp_time_t SessionAliveTimeout = PingInterval * 5;
      /// maximum number of messages we can ack in a multiack
      static constexpr std::size_t MaxACKSInMACK = 1024 / sizeof(uint64_t);

      /// outbound session
      Session(LinkLayer* parent, const RouterContact& rc,
              const AddressInfo& ai);
      /// inbound session
      Session(LinkLayer* parent, const Addr& from);

      ~Session() = default;

      std::shared_ptr< ILinkSession >
      BorrowSelf() override
      {
        return shared_from_this();
      }

      void
      Pump() override;

      void
      Tick(llarp_time_t now) override;

      bool
      SendMessageBuffer(ILinkSession::Message_t msg,
                        CompletionHandler resultHandler) override;

      void
      Send_LL(const byte_t* buf, size_t sz);

      void EncryptAndSend(ILinkSession::Packet_t);

      void
      Start() override;

      void
      Close() override;

      bool Recv_LL(ILinkSession::Packet_t) override;

      bool
      SendKeepAlive() override;

      bool
      IsEstablished() const override;

      bool
      TimedOut(llarp_time_t now) const override;

      PubKey
      GetPubKey() const override
      {
        return m_RemoteRC.pubkey;
      }

      Addr
      GetRemoteEndpoint() const override
      {
        return m_RemoteAddr;
      }

      RouterContact
      GetRemoteRC() const override
      {
        return m_RemoteRC;
      }

      size_t
      SendQueueBacklog() const override
      {
        return m_TXMsgs.size();
      }

      ILinkLayer*
      GetLinkLayer() const override
      {
        return m_Parent;
      }

      bool
      RenegotiateSession() override;

      bool
      ShouldPing() const override;

      util::StatusObject
      ExtractStatus() const override;

     private:
      enum class State
      {
        /// we have no data recv'd
        Initial,
        /// we are in introduction phase
        Introduction,
        /// we sent our LIM
        LinkIntro,
        /// handshake done and LIM has been obtained
        Ready,
        /// we are closed now
        Closed
      };
      State m_State;
      /// are we inbound session ?
      const bool m_Inbound;
      /// parent link layer
      LinkLayer* const m_Parent;
      const llarp_time_t m_CreatedAt;
      const Addr m_RemoteAddr;

      AddressInfo m_ChosenAI;
      /// remote rc
      RouterContact m_RemoteRC;
      /// session key
      SharedSecret m_SessionKey;
      /// session token
      AlignedBuffer< 24 > token;

      PubKey m_ExpectedIdent;
      PubKey m_RemoteOnionKey;

      llarp_time_t m_LastTX = 0;
      llarp_time_t m_LastRX = 0;

      uint64_t m_TXRate = 0;
      uint64_t m_RXRate = 0;

      uint64_t m_CurrentTX = 0;
      uint64_t m_CurrentRX = 0;

      llarp_time_t m_ResetRatesAt = 0;

      uint64_t m_TXID = 0;

      bool
      ShouldResetRates(llarp_time_t now) const;

      void
      ResetRates();

      std::unordered_map< uint64_t, InboundMessage > m_RXMsgs;
      std::unordered_map< uint64_t, OutboundMessage > m_TXMsgs;

      /// maps rxid to time recieved
      std::unordered_map< uint64_t, llarp_time_t > m_ReplayFilter;
      /// set of rx messages to send in next round of multiacks
      std::unordered_set< uint64_t > m_SendMACKs;

      using CryptoQueue_t   = std::vector< Packet_t >;
      using CryptoQueue_ptr = std::shared_ptr< CryptoQueue_t >;
      CryptoQueue_ptr m_EncryptNext;
      CryptoQueue_ptr m_DecryptNext;

      void
      EncryptWorker(CryptoQueue_ptr msgs);

      void
      DecryptWorker(CryptoQueue_ptr msgs);

      void
      HandlePlaintext(CryptoQueue_ptr msgs);

      void
      HandleGotIntro(Packet_t pkt);

      void
      HandleGotIntroAck(Packet_t pkt);

      void
      HandleCreateSessionRequest(Packet_t pkt);

      void
      HandleAckSession(Packet_t pkt);

      void
      HandleSessionData(Packet_t pkt);

      bool
      DecryptMessageInPlace(Packet_t& pkt);

      void
      SendMACK();

      void
      GenerateAndSendIntro();

      bool
      GotInboundLIM(const LinkIntroMessage* msg);

      bool
      GotOutboundLIM(const LinkIntroMessage* msg);

      bool
      GotRenegLIM(const LinkIntroMessage* msg);

      void
      SendOurLIM(ILinkSession::CompletionHandler h = nullptr);

      void
      HandleXMIT(Packet_t msg);

      void
      HandleDATA(Packet_t msg);

      void
      HandleACKS(Packet_t msg);

      void
      HandleNACK(Packet_t msg);

      void
      HandlePING(Packet_t msg);

      void
      HandleCLOS(Packet_t msg);

      void
      HandleMACK(Packet_t msg);
    };
  }  // namespace iwp
}  // namespace llarp

#endif
