#include <link/server.hpp>
#include <ev/ev.hpp>
#include <crypto/crypto.hpp>
#include <config/key_manager.hpp>
#include <memory>
#include <util/fs.hpp>
#include <utility>
#include <unordered_set>

constexpr uint64_t LINK_LAYER_TICK_INTERVAL = 100;

namespace llarp
{
  static constexpr size_t MaxSessionsPerKey = 16;

  ILinkLayer::ILinkLayer(std::shared_ptr< KeyManager > keyManager,
                         GetRCFunc getrc, LinkMessageHandler handler,
                         SignBufferFunc signbuf,
                         SessionEstablishedHandler establishedSession,
                         SessionRenegotiateHandler reneg,
                         TimeoutHandler timeout, SessionClosedHandler closed,
                         PumpDoneHandler pumpDone)
      : HandleMessage(std::move(handler))
      , HandleTimeout(std::move(timeout))
      , Sign(std::move(signbuf))
      , GetOurRC(std::move(getrc))
      , SessionEstablished(std::move(establishedSession))
      , SessionClosed(std::move(closed))
      , SessionRenegotiate(std::move(reneg))
      , PumpDone(std::move(pumpDone))
      , m_RouterEncSecret(keyManager->encryptionKey)
      , m_SecretKey(keyManager->transportKey)
  {
  }

  ILinkLayer::~ILinkLayer() = default;

  bool
  ILinkLayer::HasSessionTo(const RouterID& id)
  {
    ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
    return m_AuthedLinks.find(id) != m_AuthedLinks.end();
  }

  void
  ILinkLayer::ForEachSession(std::function< void(const ILinkSession*) > visit,
                             bool randomize) const
  {
    std::vector< std::shared_ptr< ILinkSession > > sessions;
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      if(m_AuthedLinks.size() == 0)
        return;
      const size_t sz = randint() % m_AuthedLinks.size();
      auto itr        = m_AuthedLinks.begin();
      auto begin      = itr;
      if(randomize)
      {
        std::advance(itr, sz);
        begin = itr;
      }
      while(itr != m_AuthedLinks.end())
      {
        sessions.emplace_back(itr->second);
        ++itr;
      }
      if(randomize)
      {
        itr = m_AuthedLinks.begin();
        while(itr != begin)
        {
          sessions.emplace_back(itr->second);
          ++itr;
        }
      }
    }
    for(const auto& session : sessions)
      visit(session.get());
  }

  bool
  ILinkLayer::VisitSessionByPubkey(const RouterID& pk,
                                   std::function< bool(ILinkSession*) > visit)
  {
    std::shared_ptr< ILinkSession > session;
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      auto itr = m_AuthedLinks.find(pk);
      if(itr == m_AuthedLinks.end())
        return false;
      session = itr->second;
    }
    return visit(session.get());
  }

  void
  ILinkLayer::ForEachSession(std::function< void(ILinkSession*) > visit)
  {
    std::vector< std::shared_ptr< ILinkSession > > sessions;
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      auto itr = m_AuthedLinks.begin();
      while(itr != m_AuthedLinks.end())
      {
        sessions.emplace_back(itr->second);
        ++itr;
      }
    }
    for(const auto& s : sessions)
      visit(s.get());
  }

  bool
  ILinkLayer::Configure(llarp_ev_loop_ptr loop, const std::string& ifname,
                        int af, uint16_t port)
  {
    m_Loop         = loop;
    m_udp.user     = this;
    m_udp.recvfrom = nullptr;
    m_udp.tick     = &ILinkLayer::udp_tick;
    if(ifname == "*")
    {
      if(!AllInterfaces(af, m_ourAddr))
        return false;
    }
    else if(!GetIFAddr(ifname, m_ourAddr, af))
      m_ourAddr = Addr(ifname);
    m_ourAddr.port(port);
    return llarp_ev_add_udp(m_Loop.get(), &m_udp, m_ourAddr) != -1;
  }

  void
  ILinkLayer::Pump()
  {
    std::unordered_set< RouterID, RouterID::Hash > closedSessions;
    std::vector< std::shared_ptr< ILinkSession > > closedPending;
    auto _now = Now();
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      auto itr = m_AuthedLinks.begin();
      while(itr != m_AuthedLinks.end())
      {
        if(not itr->second->TimedOut(_now))
        {
          itr->second->Pump();
          ++itr;
        }
        else
        {
          llarp::LogInfo("session to ", RouterID(itr->second->GetPubKey()),
                         " timed out");
          itr->second->Close();
          closedSessions.emplace(itr->first);
          itr = m_AuthedLinks.erase(itr);
        }
      }
    }
    {
      ACQUIRE_LOCK(Lock_t l, m_PendingMutex);

      auto itr = m_Pending.begin();
      while(itr != m_Pending.end())
      {
        if(not itr->second->TimedOut(_now))
        {
          itr->second->Pump();
          ++itr;
        }
        else
        {
          LogInfo("pending session at ", itr->first, " timed out");
          // defer call so we can acquire mutexes later
          closedPending.emplace_back(std::move(itr->second));
          itr = m_Pending.erase(itr);
        }
      }
    }
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      for(const auto& r : closedSessions)
      {
        if(m_AuthedLinks.count(r) == 0)
        {
          SessionClosed(r);
        }
      }
    }
    for(const auto& pending : closedPending)
    {
      HandleTimeout(pending.get());
    }
  }

  bool
  ILinkLayer::MapAddr(const RouterID& pk, ILinkSession* s)
  {
    ACQUIRE_LOCK(Lock_t l_authed, m_AuthedLinksMutex);
    ACQUIRE_LOCK(Lock_t l_pending, m_PendingMutex);
    llarp::Addr addr = s->GetRemoteEndpoint();
    auto itr         = m_Pending.find(addr);
    if(itr != m_Pending.end())
    {
      if(m_AuthedLinks.count(pk) > MaxSessionsPerKey)
      {
        LogWarn("too many session for ", pk);
        s->Close();
        return false;
      }
      m_AuthedLinks.emplace(pk, itr->second);
      itr = m_Pending.erase(itr);
      return true;
    }
    return false;
  }

  bool
  ILinkLayer::PickAddress(const RouterContact& rc,
                          llarp::AddressInfo& picked) const
  {
    std::string OurDialect = Name();
    for(const auto& addr : rc.addrs)
    {
      if(addr.dialect == OurDialect)
      {
        picked = addr;
        return true;
      }
    }
    return false;
  }

  util::StatusObject
  ILinkLayer::ExtractStatus() const
  {
    std::vector< util::StatusObject > pending, established;

    {
      ACQUIRE_LOCK(Lock_t l, m_PendingMutex);
      std::transform(m_Pending.cbegin(), m_Pending.cend(),
                     std::back_inserter(pending),
                     [](const auto& item) -> util::StatusObject {
                       return item.second->ExtractStatus();
                     });
    }
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      std::transform(m_AuthedLinks.cbegin(), m_AuthedLinks.cend(),
                     std::back_inserter(established),
                     [](const auto& item) -> util::StatusObject {
                       return item.second->ExtractStatus();
                     });
    }

    return {{"name", Name()},
            {"rank", uint64_t(Rank())},
            {"addr", m_ourAddr.ToString()},
            {"sessions",
             util::StatusObject{{"pending", pending},
                                {"established", established}}}};
  }

  bool
  ILinkLayer::TryEstablishTo(RouterContact rc)
  {
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      if(m_AuthedLinks.count(rc.pubkey) >= MaxSessionsPerKey)
        return false;
    }
    llarp::AddressInfo to;
    if(!PickAddress(rc, to))
      return false;
    const llarp::Addr addr(to);
    {
      ACQUIRE_LOCK(Lock_t l, m_PendingMutex);
      if(m_Pending.count(addr) >= MaxSessionsPerKey)
        return false;
    }
    std::shared_ptr< ILinkSession > s = NewOutboundSession(rc, to);
    if(PutSession(s))
    {
      s->Start();
      return true;
    }
    return false;
  }

  bool
  ILinkLayer::Start(std::shared_ptr< Logic > l,
                    std::shared_ptr< thread::ThreadPool > worker)
  {
    m_Worker = worker;
    m_Logic  = l;
    ScheduleTick(LINK_LAYER_TICK_INTERVAL);
    return true;
  }

  void
  ILinkLayer::Tick(llarp_time_t now)
  {
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      auto itr = m_AuthedLinks.begin();
      while(itr != m_AuthedLinks.end())
      {
        itr->second->Tick(now);
        ++itr;
      }
    }

    {
      ACQUIRE_LOCK(Lock_t l, m_PendingMutex);
      auto itr = m_Pending.begin();
      while(itr != m_Pending.end())
      {
        itr->second->Tick(now);
        ++itr;
      }
    }
    {
      // decay recently closed list
      auto itr = m_RecentlyClosed.begin();
      while(itr != m_RecentlyClosed.end())
      {
        if(itr->second >= now)
          itr = m_RecentlyClosed.erase(itr);
        else
          ++itr;
      }
    }
  }

  void
  ILinkLayer::Stop()
  {
    if(m_Logic && tick_id)
      m_Logic->remove_call(tick_id);
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      auto itr = m_AuthedLinks.begin();
      while(itr != m_AuthedLinks.end())
      {
        itr->second->Close();
        ++itr;
      }
    }
    {
      ACQUIRE_LOCK(Lock_t l, m_PendingMutex);
      auto itr = m_Pending.begin();
      while(itr != m_Pending.end())
      {
        itr->second->Close();
        ++itr;
      }
    }
  }

  void
  ILinkLayer::CloseSessionTo(const RouterID& remote)
  {
    static constexpr llarp_time_t CloseGraceWindow = 500;
    const auto now                                 = Now();
    ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
    RouterID r = remote;
    llarp::LogInfo("Closing all to ", r);
    auto range = m_AuthedLinks.equal_range(r);
    auto itr   = range.first;
    while(itr != range.second)
    {
      itr->second->Close();
      m_RecentlyClosed.emplace(itr->second->GetRemoteEndpoint(),
                               now + CloseGraceWindow);
      itr = m_AuthedLinks.erase(itr);
    }
  }

  void
  ILinkLayer::KeepAliveSessionTo(const RouterID& remote)
  {
    ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
    auto range = m_AuthedLinks.equal_range(remote);
    auto itr   = range.first;
    while(itr != range.second)
    {
      if(itr->second->ShouldPing())
        itr->second->SendKeepAlive();
      ++itr;
    }
  }

  bool
  ILinkLayer::SendTo(const RouterID& remote, const llarp_buffer_t& buf,
                     ILinkSession::CompletionHandler completed)
  {
    std::shared_ptr< ILinkSession > s;
    {
      ACQUIRE_LOCK(Lock_t l, m_AuthedLinksMutex);
      auto range = m_AuthedLinks.equal_range(remote);
      auto itr   = range.first;
      // pick lowest backlog session
      size_t min = std::numeric_limits< size_t >::max();

      while(itr != range.second)
      {
        const auto backlog = itr->second->SendQueueBacklog();
        if(backlog < min)
        {
          s   = itr->second;
          min = backlog;
        }
        ++itr;
      }
    }
    ILinkSession::Message_t pkt(buf.sz);
    std::copy_n(buf.base, buf.sz, pkt.begin());
    return s && s->SendMessageBuffer(std::move(pkt), completed);
  }

  bool
  ILinkLayer::GetOurAddressInfo(llarp::AddressInfo& addr) const
  {
    addr.dialect = Name();
    addr.pubkey  = TransportPubKey();
    addr.rank    = Rank();
    addr.port    = m_ourAddr.port();
    addr.ip      = *m_ourAddr.addr6();
    return true;
  }

  const byte_t*
  ILinkLayer::TransportPubKey() const
  {
    return llarp::seckey_topublic(TransportSecretKey());
  }

  const SecretKey&
  ILinkLayer::TransportSecretKey() const
  {
    return m_SecretKey;
  }

  bool
  ILinkLayer::PutSession(const std::shared_ptr< ILinkSession >& s)
  {
    static constexpr size_t MaxSessionsPerEndpoint = 5;
    ACQUIRE_LOCK(Lock_t lock, m_PendingMutex);
    llarp::Addr addr = s->GetRemoteEndpoint();
    if(m_Pending.count(addr) >= MaxSessionsPerEndpoint)
      return false;
    m_Pending.emplace(addr, s);
    return true;
  }

  void
  ILinkLayer::OnTick()
  {
    auto now = Now();
    Tick(now);
    ScheduleTick(LINK_LAYER_TICK_INTERVAL);
  }

  void
  ILinkLayer::ScheduleTick(uint64_t interval)
  {
    tick_id =
        m_Logic->call_later(interval, std::bind(&ILinkLayer::OnTick, this));
  }

  void
  ILinkLayer::udp_tick(llarp_udp_io* udp)
  {
    ILinkLayer* link = static_cast< ILinkLayer* >(udp->user);
    auto pkts        = std::make_shared< llarp_pkt_list >();
    llarp_ev_udp_recvmany(&link->m_udp, pkts.get());
    auto logic = link->logic();
    if(logic == nullptr)
      return;
    LogicCall(logic, [pkts, link]() {
      auto itr = pkts->begin();
      while(itr != pkts->end())
      {
        if(link->m_RecentlyClosed.find(itr->remote)
           == link->m_RecentlyClosed.end())
        {
          link->RecvFrom(itr->remote, std::move(itr->pkt));
        }
        ++itr;
      }
      link->Pump();
    });
  }

}  // namespace llarp
