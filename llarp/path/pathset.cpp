#include <path/pathset.hpp>

#include <dht/messages/pubintro.hpp>
#include <path/path.hpp>
#include <routing/dht_message.hpp>
#include <router/abstractrouter.hpp>

namespace llarp
{
  namespace path
  {
    PathSet::PathSet(size_t num) : numPaths(num)
    {
    }

    bool
    PathSet::ShouldBuildMore(llarp_time_t now) const
    {
      (void)now;
      const auto building = NumInStatus(ePathBuilding);
      if(building >= numPaths)
        return false;
      const auto established = NumInStatus(ePathEstablished);
      return established < numPaths;
    }

    bool
    PathSet::ShouldBuildMoreForRoles(llarp_time_t now, PathRole roles) const
    {
      Lock_t l(&m_PathsMutex);
      const size_t required = MinRequiredForRoles(roles);
      size_t has            = 0;
      for(const auto& item : m_Paths)
      {
        if(item.second->SupportsAnyRoles(roles))
        {
          if(!item.second->ExpiresSoon(now))
            ++has;
        }
      }
      return has < required;
    }

    size_t
    PathSet::MinRequiredForRoles(PathRole roles) const
    {
      (void)roles;
      return 0;
    }

    size_t
    PathSet::NumPathsExistingAt(llarp_time_t futureTime) const
    {
      size_t num = 0;
      Lock_t l(&m_PathsMutex);
      for(const auto& item : m_Paths)
      {
        if(item.second->IsReady() && !item.second->Expired(futureTime))
          ++num;
      }
      return num;
    }

    void
    PathSet::TickPaths(AbstractRouter* r)
    {
      const auto now = llarp::time_now_ms();
      Lock_t l(&m_PathsMutex);
      for(auto& item : m_Paths)
      {
        item.second->Tick(now, r);
      }
    }

    void
    PathSet::ExpirePaths(llarp_time_t now, AbstractRouter* router)
    {
      Lock_t l(&m_PathsMutex);
      if(m_Paths.size() == 0)
        return;
      auto itr = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->Expired(now))
        {
          router->outboundMessageHandler().QueueRemoveEmptyPath(
              itr->second->TXID());
          itr = m_Paths.erase(itr);
        }
        else
          ++itr;
      }
    }

    Path_ptr
    PathSet::GetEstablishedPathClosestTo(RouterID id, PathRole roles) const
    {
      Lock_t l(&m_PathsMutex);
      Path_ptr path = nullptr;
      AlignedBuffer< 32 > dist;
      AlignedBuffer< 32 > to = id;
      dist.Fill(0xff);
      for(const auto& item : m_Paths)
      {
        if(!item.second->IsReady())
          continue;
        if(!item.second->SupportsAnyRoles(roles))
          continue;
        AlignedBuffer< 32 > localDist = item.second->Endpoint() ^ to;
        if(localDist < dist)
        {
          dist = localDist;
          path = item.second;
        }
      }
      return path;
    }

    Path_ptr
    PathSet::GetNewestPathByRouter(RouterID id, PathRole roles) const
    {
      Lock_t l(&m_PathsMutex);
      Path_ptr chosen = nullptr;
      auto itr        = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->IsReady() && itr->second->SupportsAnyRoles(roles))
        {
          if(itr->second->Endpoint() == id)
          {
            if(chosen == nullptr)
              chosen = itr->second;
            else if(chosen->intro.expiresAt < itr->second->intro.expiresAt)
              chosen = itr->second;
          }
        }
        ++itr;
      }
      return chosen;
    }

    Path_ptr
    PathSet::GetPathByRouter(RouterID id, PathRole roles) const
    {
      Lock_t l(&m_PathsMutex);
      Path_ptr chosen = nullptr;
      auto itr        = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->IsReady() && itr->second->SupportsAnyRoles(roles))
        {
          if(itr->second->Endpoint() == id)
          {
            if(chosen == nullptr)
              chosen = itr->second;
            else if(chosen->intro.latency > itr->second->intro.latency)
              chosen = itr->second;
          }
        }
        ++itr;
      }
      return chosen;
    }

    Path_ptr
    PathSet::GetByEndpointWithID(RouterID ep, PathID_t id) const
    {
      Lock_t l(&m_PathsMutex);
      auto itr = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->IsEndpoint(ep, id))
        {
          return itr->second;
        }
        ++itr;
      }
      return nullptr;
    }

    Path_ptr
    PathSet::GetPathByID(PathID_t id) const
    {
      Lock_t l(&m_PathsMutex);
      auto itr = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->RXID() == id)
          return itr->second;
        ++itr;
      }
      return nullptr;
    }

    size_t
    PathSet::AvailablePaths(PathRole roles) const
    {
      Lock_t l(&m_PathsMutex);
      size_t count = 0;
      auto itr     = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->Status() == ePathEstablished
           && itr->second->SupportsAnyRoles(roles))
          ++count;
        ++itr;
      }
      return count;
    }

    size_t
    PathSet::NumInStatus(PathStatus st) const
    {
      Lock_t l(&m_PathsMutex);
      size_t count = 0;
      auto itr     = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->Status() == st)
          ++count;
        ++itr;
      }
      return count;
    }

    void
    PathSet::AddPath(Path_ptr path)
    {
      Lock_t l(&m_PathsMutex);
      const auto upstream = path->Upstream();  // RouterID
      const auto RXID     = path->RXID();      // PathID
      if(not m_Paths.emplace(std::make_pair(upstream, RXID), path).second)
      {
        LogError(Name(),
                 " failed to add own path, duplicate info wtf? upstream=",
                 upstream, " rxid=", RXID);
      }
    }

    void
    PathSet::RemovePath(Path_ptr path)
    {
      Lock_t l(&m_PathsMutex);
      m_Paths.erase({path->Upstream(), path->RXID()});
    }

    Path_ptr
    PathSet::GetByUpstream(RouterID remote, PathID_t rxid) const
    {
      Lock_t l(&m_PathsMutex);
      auto itr = m_Paths.find({remote, rxid});
      if(itr == m_Paths.end())
        return nullptr;
      return itr->second;
    }

    bool
    PathSet::GetCurrentIntroductionsWithFilter(
        std::set< service::Introduction >& intros,
        std::function< bool(const service::Introduction&) > filter) const
    {
      intros.clear();
      size_t count = 0;
      Lock_t l(&m_PathsMutex);
      auto itr = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->IsReady() && filter(itr->second->intro))
        {
          intros.insert(itr->second->intro);
          ++count;
        }
        ++itr;
      }
      return count > 0;
    }

    bool
    PathSet::GetCurrentIntroductions(
        std::set< service::Introduction >& intros) const
    {
      intros.clear();
      size_t count = 0;
      Lock_t l(&m_PathsMutex);
      auto itr = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->IsReady())
        {
          intros.insert(itr->second->intro);
          ++count;
        }
        ++itr;
      }
      return count > 0;
    }

    void
    PathSet::HandlePathBuildTimeout(Path_ptr p)
    {
      LogWarn(Name(), " path build ", p->HopsString(), " timed out");
      m_BuildStats.timeouts++;
    }

    void
    PathSet::HandlePathBuildFailed(Path_ptr p)
    {
      LogWarn(Name(), " path build ", p->HopsString(), " failed");
      m_BuildStats.fails++;
    }

    void
    PathSet::PathBuildStarted(Path_ptr p)
    {
      LogInfo(Name(), " path build ", p->HopsString(), " started");
      m_BuildStats.attempts++;
    }

    util::StatusObject
    BuildStats::ExtractStatus() const
    {
      return util::StatusObject{{"success", success},
                                {"attempts", attempts},
                                {"timeouts", timeouts},
                                {"fails", fails}};
    }

    std::string
    BuildStats::ToString() const
    {
      std::stringstream ss;
      ss << (SuccsessRatio() * 100.0) << " percent success ";
      ss << "(success=" << success << " ";
      ss << "attempts=" << attempts << " ";
      ss << "timeouts=" << timeouts << " ";
      ss << "fails=" << fails << ")";
      return ss.str();
    }

    double
    BuildStats::SuccsessRatio() const
    {
      if(attempts)
        return double(success) / double(attempts);
      return 0.0;
    }

    bool
    PathSet::GetNewestIntro(service::Introduction& intro) const
    {
      intro.Clear();
      bool found = false;
      Lock_t l(&m_PathsMutex);
      auto itr = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->IsReady()
           && itr->second->intro.expiresAt > intro.expiresAt)
        {
          intro = itr->second->intro;
          found = true;
        }
        ++itr;
      }
      return found;
    }

    Path_ptr
    PathSet::PickRandomEstablishedPath(PathRole roles) const
    {
      std::vector< Path_ptr > established;
      Lock_t l(&m_PathsMutex);
      auto itr = m_Paths.begin();
      while(itr != m_Paths.end())
      {
        if(itr->second->IsReady() && itr->second->SupportsAnyRoles(roles))
          established.push_back(itr->second);
        ++itr;
      }
      auto sz = established.size();
      if(sz)
      {
        return established[randint() % sz];
      }

      return nullptr;
    }

    void
    PathSet::UpstreamFlush(AbstractRouter* r)
    {
      ForEachPath([r](const Path_ptr& p) { p->FlushUpstream(r); });
    }

    void
    PathSet::DownstreamFlush(AbstractRouter* r)
    {
      ForEachPath([r](const Path_ptr& p) { p->FlushDownstream(r); });
    }

  }  // namespace path
}  // namespace llarp
