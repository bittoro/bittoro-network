#include <profiling.hpp>

#include <fstream>
#include <util/fs.hpp>

namespace llarp
{
  bool
  RouterProfile::BEncode(llarp_buffer_t* buf) const
  {
    if(!bencode_start_dict(buf))
      return false;

    if(!BEncodeWriteDictInt("g", connectGoodCount, buf))
      return false;
    if(!BEncodeWriteDictInt("p", pathSuccessCount, buf))
      return false;
    if(!BEncodeWriteDictInt("s", pathFailCount, buf))
      return false;
    if(!BEncodeWriteDictInt("t", connectTimeoutCount, buf))
      return false;
    if(!BEncodeWriteDictInt("u", lastUpdated, buf))
      return false;
    if(!BEncodeWriteDictInt("v", version, buf))
      return false;

    return bencode_end(buf);
  }

  bool
  RouterProfile::DecodeKey(const llarp_buffer_t& k, llarp_buffer_t* buf)
  {
    bool read = false;
    if(!BEncodeMaybeReadDictInt("g", connectGoodCount, read, k, buf))
      return false;
    if(!BEncodeMaybeReadDictInt("t", connectTimeoutCount, read, k, buf))
      return false;
    if(!BEncodeMaybeReadDictInt("u", lastUpdated, read, k, buf))
      return false;
    if(!BEncodeMaybeReadDictInt("v", version, read, k, buf))
      return false;
    if(!BEncodeMaybeReadDictInt("s", pathFailCount, read, k, buf))
      return false;
    if(!BEncodeMaybeReadDictInt("p", pathSuccessCount, read, k, buf))
      return false;
    return read;
  }

  void
  RouterProfile::Decay()
  {
    connectGoodCount /= 2;
    connectTimeoutCount /= 2;
    pathSuccessCount /= 2;
    pathFailCount /= 2;
    lastDecay = llarp::time_now_ms();
  }

  void
  RouterProfile::Tick()
  {
    // 15 seconds
    static constexpr llarp_time_t updateInterval = 15 * 1000;
    const auto now                               = llarp::time_now_ms();
    if(lastDecay < now && now - lastDecay > updateInterval)
      Decay();
  }

  bool
  RouterProfile::IsGood(uint64_t chances) const
  {
    if(connectTimeoutCount > chances)
      return connectTimeoutCount < connectGoodCount
          && (pathSuccessCount * chances) > pathFailCount;
    return (pathSuccessCount * chances) > pathFailCount;
  }

  static bool constexpr checkIsGood(uint64_t fails, uint64_t success,
                                    uint64_t chances)
  {
    if(fails > 0 && (fails + success) >= chances)
      return (success / fails) > 1;
    if(success == 0)
      return fails < chances;
    return true;
  }

  bool
  RouterProfile::IsGoodForConnect(uint64_t chances) const
  {
    return checkIsGood(connectTimeoutCount, connectGoodCount, chances);
  }

  bool
  RouterProfile::IsGoodForPath(uint64_t chances) const
  {
    return checkIsGood(pathFailCount, pathSuccessCount, chances);
  }

  Profiling::Profiling() : m_DisableProfiling(false)
  {
  }

  void
  Profiling::Disable()
  {
    m_DisableProfiling.store(true);
  }

  void
  Profiling::Enable()
  {
    m_DisableProfiling.store(false);
  }

  bool
  Profiling::IsBadForConnect(const RouterID& r, uint64_t chances)
  {
    if(m_DisableProfiling.load())
      return false;
    lock_t lock(&m_ProfilesMutex);
    auto itr = m_Profiles.find(r);
    if(itr == m_Profiles.end())
      return false;
    return !itr->second.IsGoodForConnect(chances);
  }

  bool
  Profiling::IsBadForPath(const RouterID& r, uint64_t chances)
  {
    if(m_DisableProfiling.load())
      return false;
    lock_t lock(&m_ProfilesMutex);
    auto itr = m_Profiles.find(r);
    if(itr == m_Profiles.end())
      return false;
    return !itr->second.IsGoodForPath(chances);
  }

  bool
  Profiling::IsBad(const RouterID& r, uint64_t chances)
  {
    if(m_DisableProfiling.load())
      return false;
    lock_t lock(&m_ProfilesMutex);
    auto itr = m_Profiles.find(r);
    if(itr == m_Profiles.end())
      return false;
    return !itr->second.IsGood(chances);
  }

  void
  Profiling::Tick()
  {
    lock_t lock(&m_ProfilesMutex);
    std::for_each(m_Profiles.begin(), m_Profiles.end(),
                  [](auto& item) { item.second.Tick(); });
  }

  void
  Profiling::MarkConnectTimeout(const RouterID& r)
  {
    lock_t lock(&m_ProfilesMutex);
    m_Profiles[r].connectTimeoutCount += 1;
    m_Profiles[r].lastUpdated = llarp::time_now_ms();
  }

  void
  Profiling::MarkConnectSuccess(const RouterID& r)
  {
    lock_t lock(&m_ProfilesMutex);
    m_Profiles[r].connectGoodCount += 1;
    m_Profiles[r].lastUpdated = llarp::time_now_ms();
  }

  void
  Profiling::ClearProfile(const RouterID& r)
  {
    lock_t lock(&m_ProfilesMutex);
    m_Profiles.erase(r);
  }

  void
  Profiling::MarkHopFail(const RouterID& r)
  {
    lock_t lock(&m_ProfilesMutex);
    m_Profiles[r].pathFailCount += 1;
    m_Profiles[r].lastUpdated = llarp::time_now_ms();
  }

  void
  Profiling::MarkPathFail(path::Path* p)
  {
    lock_t lock(&m_ProfilesMutex);
    size_t idx = 0;
    for(const auto& hop : p->hops)
    {
      // don't mark first hop as failure because we are connected to it directly
      if(idx)
      {
        m_Profiles[hop.rc.pubkey].pathFailCount += 1;
        m_Profiles[hop.rc.pubkey].lastUpdated = llarp::time_now_ms();
      }
      ++idx;
    }
  }

  void
  Profiling::MarkPathSuccess(path::Path* p)
  {
    lock_t lock(&m_ProfilesMutex);
    const auto sz = p->hops.size();
    for(const auto& hop : p->hops)
    {
      m_Profiles[hop.rc.pubkey].pathSuccessCount += sz;
      m_Profiles[hop.rc.pubkey].lastUpdated = llarp::time_now_ms();
    }
  }

  bool
  Profiling::Save(const char* fname)
  {
    absl::ReaderMutexLock lock(&m_ProfilesMutex);
    size_t sz = (m_Profiles.size() * (RouterProfile::MaxSize + 32 + 8)) + 8;

    std::vector< byte_t > tmp(sz, 0);
    llarp_buffer_t buf(tmp);
    auto res = BEncodeNoLock(&buf);
    if(res)
    {
      buf.sz               = buf.cur - buf.base;
      const fs::path fpath = std::string(fname);
      auto optional_f =
          util::OpenFileStream< std::ofstream >(fpath, std::ios::binary);
      if(!optional_f)
        return false;
      auto& f = optional_f.value();
      if(f.is_open())
      {
        f.write((char*)buf.base, buf.sz);
        m_LastSave = llarp::time_now_ms();
      }
    }
    return res;
  }

  bool
  Profiling::BEncode(llarp_buffer_t* buf) const
  {
    absl::ReaderMutexLock lock(&m_ProfilesMutex);
    return BEncodeNoLock(buf);
  }

  bool
  Profiling::BEncodeNoLock(llarp_buffer_t* buf) const
  {
    if(!bencode_start_dict(buf))
      return false;

    auto itr = m_Profiles.begin();
    while(itr != m_Profiles.end())
    {
      if(!itr->first.BEncode(buf))
        return false;
      if(!itr->second.BEncode(buf))
        return false;
      ++itr;
    }
    return bencode_end(buf);
  }

  bool
  Profiling::DecodeKey(const llarp_buffer_t& k, llarp_buffer_t* buf)
  {
    if(k.sz != 32)
      return false;
    RouterProfile profile;
    if(!bencode_decode_dict(profile, buf))
      return false;
    RouterID pk = k.base;
    return m_Profiles.emplace(pk, profile).second;
  }

  bool
  Profiling::Load(const char* fname)
  {
    lock_t lock(&m_ProfilesMutex);
    m_Profiles.clear();
    if(!BDecodeReadFromFile(fname, *this))
    {
      llarp::LogWarn("failed to load router profiles from ", fname);
      return false;
    }
    m_LastSave = llarp::time_now_ms();
    return true;
  }

  bool
  Profiling::ShouldSave(llarp_time_t now) const
  {
    auto dlt = now - m_LastSave;
    return dlt > 60000;
  }
}  // namespace llarp
