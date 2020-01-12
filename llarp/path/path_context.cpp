#include <path/path_context.hpp>

#include <messages/relay_commit.hpp>
#include <path/path.hpp>
#include <router/abstractrouter.hpp>
#include <router/i_outbound_message_handler.hpp>

namespace llarp
{
  namespace path
  {
    static constexpr llarp_time_t DefaultPathBuildLimit = 500;

    PathContext::PathContext(AbstractRouter* router)
        : m_Router(router)
        , m_AllowTransit(false)
        , m_PathLimits(DefaultPathBuildLimit)
    {
    }

    void
    PathContext::AllowTransit()
    {
      m_AllowTransit = true;
    }

    bool
    PathContext::AllowingTransit() const
    {
      return m_AllowTransit;
    }

    std::shared_ptr< thread::ThreadPool >
    PathContext::Worker()
    {
      return m_Router->threadpool();
    }

    bool
    PathContext::CheckPathLimitHitByIP(const llarp::Addr& ip)
    {
      llarp::Addr remote = ip;
      // set port to zero
      remote.port(0);
      // try inserting remote address by ip into decaying hash set
      // if it cannot insert it has hit a limit
      return not m_PathLimits.Insert(remote);
    }

    std::shared_ptr< Logic >
    PathContext::logic()
    {
      return m_Router->logic();
    }

    const SecretKey&
    PathContext::EncryptionSecretKey()
    {
      return m_Router->encryption();
    }

    bool
    PathContext::HopIsUs(const RouterID& k) const
    {
      return std::equal(m_Router->pubkey(), m_Router->pubkey() + PUBKEYSIZE,
                        k.begin());
    }

    PathContext::EndpointPathPtrSet
    PathContext::FindOwnedPathsWithEndpoint(const RouterID& r)
    {
      EndpointPathPtrSet found;
      m_OurPaths.ForEach([&](const Path_ptr& p) {
        if(p->Endpoint() == r && p->IsReady())
          found.insert(p);
      });
      return found;
    }

    bool
    PathContext::ForwardLRCM(const RouterID& nextHop,
                             const std::array< EncryptedFrame, 8 >& frames,
                             SendStatusHandler handler)
    {
      if(handler == nullptr)
      {
        LogError("Calling ForwardLRCM without passing result handler");
        return false;
      }

      auto msg = std::make_shared< const LR_CommitMessage >(frames);

      LogDebug("forwarding LRCM to ", nextHop);

      m_Router->SendToOrQueue(nextHop, msg.get(), handler);

      return true;
    }
    template < typename Lock_t, typename Map_t, typename Key_t,
               typename CheckValue_t, typename GetFunc_t >
    HopHandler_ptr
    MapGet(Map_t& map, const Key_t& k, CheckValue_t check, GetFunc_t get)
    {
      Lock_t lock(&map.first);
      auto range = map.second.equal_range(k);
      for(auto i = range.first; i != range.second; ++i)
      {
        if(check(i->second))
          return get(i->second);
      }
      return nullptr;
    }

    template < typename Lock_t, typename Map_t, typename Key_t,
               typename CheckValue_t >
    bool
    MapHas(Map_t& map, const Key_t& k, CheckValue_t check)
    {
      Lock_t lock(&map.first);
      auto range = map.second.equal_range(k);
      for(auto i = range.first; i != range.second; ++i)
      {
        if(check(i->second))
          return true;
      }
      return false;
    }

    template < typename Lock_t, typename Map_t, typename Key_t,
               typename Value_t >
    void
    MapPut(Map_t& map, const Key_t& k, const Value_t& v)
    {
      Lock_t lock(&map.first);
      map.second.emplace(k, v);
    }

    template < typename Lock_t, typename Map_t, typename Visit_t >
    void
    MapIter(Map_t& map, Visit_t v)
    {
      Lock_t lock(map.first);
      for(const auto& item : map.second)
        v(item);
    }

    template < typename Lock_t, typename Map_t, typename Key_t,
               typename Check_t >
    void
    MapDel(Map_t& map, const Key_t& k, Check_t check)
    {
      Lock_t lock(map.first);
      auto range = map.second.equal_range(k);
      for(auto i = range.first; i != range.second;)
      {
        if(check(i->second))
          i = map.second.erase(i);
        else
          ++i;
      }
    }

    void
    PathContext::AddOwnPath(PathSet_ptr set, Path_ptr path)
    {
      set->AddPath(path);
      MapPut< SyncOwnedPathsMap_t::Lock_t >(m_OurPaths, path->TXID(), path);
      MapPut< SyncOwnedPathsMap_t::Lock_t >(m_OurPaths, path->RXID(), path);
    }

    bool
    PathContext::HasTransitHop(const TransitHopInfo& info)
    {
      return MapHas< SyncTransitMap_t::Lock_t >(
          m_TransitPaths, info.txID,
          [info](const std::shared_ptr< TransitHop >& hop) -> bool {
            return info == hop->info;
          });
    }

    HopHandler_ptr
    PathContext::GetByUpstream(const RouterID& remote, const PathID_t& id)
    {
      auto own = MapGet< SyncOwnedPathsMap_t::Lock_t >(
          m_OurPaths, id,
          [](const Path_ptr) -> bool {
            // TODO: is this right?
            return true;
          },
          [](Path_ptr p) -> HopHandler_ptr { return p; });
      if(own)
        return own;

      return MapGet< SyncTransitMap_t::Lock_t >(
          m_TransitPaths, id,
          [remote](const std::shared_ptr< TransitHop >& hop) -> bool {
            return hop->info.upstream == remote;
          },
          [](const std::shared_ptr< TransitHop >& h) -> HopHandler_ptr {
            return h;
          });
    }

    bool
    PathContext::TransitHopPreviousIsRouter(const PathID_t& path,
                                            const RouterID& otherRouter)
    {
      SyncTransitMap_t::Lock_t lock(&m_TransitPaths.first);
      auto itr = m_TransitPaths.second.find(path);
      if(itr == m_TransitPaths.second.end())
        return false;
      return itr->second->info.downstream == otherRouter;
    }

    HopHandler_ptr
    PathContext::GetByDownstream(const RouterID& remote, const PathID_t& id)
    {
      return MapGet< SyncTransitMap_t::Lock_t >(
          m_TransitPaths, id,
          [remote](const std::shared_ptr< TransitHop >& hop) -> bool {
            return hop->info.downstream == remote;
          },
          [](const std::shared_ptr< TransitHop >& h) -> HopHandler_ptr {
            return h;
          });
    }

    PathSet_ptr
    PathContext::GetLocalPathSet(const PathID_t& id)
    {
      auto& map = m_OurPaths;
      SyncOwnedPathsMap_t::Lock_t lock(&map.first);
      auto itr = map.second.find(id);
      if(itr != map.second.end())
      {
        return itr->second->m_PathSet->GetSelf();
      }
      return nullptr;
    }

    const byte_t*
    PathContext::OurRouterID() const
    {
      return m_Router->pubkey();
    }

    AbstractRouter*
    PathContext::Router()
    {
      return m_Router;
    }

    TransitHop_ptr
    PathContext::GetPathForTransfer(const PathID_t& id)
    {
      const RouterID us(OurRouterID());
      auto& map = m_TransitPaths;
      {
        SyncTransitMap_t::Lock_t lock(&map.first);
        auto range = map.second.equal_range(id);
        for(auto i = range.first; i != range.second; ++i)
        {
          if(i->second->info.upstream == us)
            return i->second;
        }
      }
      return nullptr;
    }

    void
    PathContext::PumpUpstream()
    {
      m_TransitPaths.ForEach([&](auto& ptr) { ptr->FlushUpstream(m_Router); });
      m_OurPaths.ForEach([&](auto& ptr) { ptr->FlushUpstream(m_Router); });
    }

    void
    PathContext::PumpDownstream()
    {
      m_TransitPaths.ForEach(
          [&](auto& ptr) { ptr->FlushDownstream(m_Router); });
      m_OurPaths.ForEach([&](auto& ptr) { ptr->FlushDownstream(m_Router); });
    }

    void
    PathContext::PutTransitHop(std::shared_ptr< TransitHop > hop)
    {
      MapPut< SyncTransitMap_t::Lock_t >(m_TransitPaths, hop->info.txID, hop);
      MapPut< SyncTransitMap_t::Lock_t >(m_TransitPaths, hop->info.rxID, hop);
    }

    void
    PathContext::ExpirePaths(llarp_time_t now)
    {
      // decay limits
      m_PathLimits.Decay(now);

      {
        SyncTransitMap_t::Lock_t lock(&m_TransitPaths.first);
        auto& map = m_TransitPaths.second;
        auto itr  = map.begin();
        while(itr != map.end())
        {
          if(itr->second->Expired(now))
          {
            m_Router->outboundMessageHandler().QueueRemoveEmptyPath(itr->first);
            itr = map.erase(itr);
          }
          else
            ++itr;
        }
      }
      {
        SyncOwnedPathsMap_t::Lock_t lock(&m_OurPaths.first);
        auto& map = m_OurPaths.second;
        auto itr  = map.begin();
        while(itr != map.end())
        {
          if(itr->second->Expired(now))
          {
            itr = map.erase(itr);
          }
          else
            ++itr;
        }
      }
    }

    routing::MessageHandler_ptr
    PathContext::GetHandler(const PathID_t& id)
    {
      routing::MessageHandler_ptr h = nullptr;
      auto pathset                  = GetLocalPathSet(id);
      if(pathset)
      {
        h = pathset->GetPathByID(id);
      }
      if(h)
        return h;
      const RouterID us(OurRouterID());
      auto& map = m_TransitPaths;
      {
        SyncTransitMap_t::Lock_t lock(&map.first);
        auto range = map.second.equal_range(id);
        for(auto i = range.first; i != range.second; ++i)
        {
          if(i->second->info.upstream == us)
            return i->second;
        }
      }
      return nullptr;
    }

    void PathContext::RemovePathSet(PathSet_ptr)
    {
    }
  }  // namespace path
}  // namespace llarp
