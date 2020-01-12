#include <path/ihophandler.hpp>

namespace llarp
{
  namespace path
  {
    // handle data in upstream direction
    bool
    IHopHandler::HandleUpstream(const llarp_buffer_t& X, const TunnelNonce& Y,
                                AbstractRouter*)
    {
      if(m_UpstreamQueue == nullptr)
        m_UpstreamQueue = std::make_shared< TrafficQueue_t >();
      m_UpstreamQueue->emplace_back();
      auto& pkt = m_UpstreamQueue->back();
      pkt.first.resize(X.sz);
      std::copy_n(X.base, X.sz, pkt.first.begin());
      pkt.second = Y;
      return true;
    }

    // handle data in downstream direction
    bool
    IHopHandler::HandleDownstream(const llarp_buffer_t& X, const TunnelNonce& Y,
                                  AbstractRouter*)
    {
      if(m_DownstreamQueue == nullptr)
        m_DownstreamQueue = std::make_shared< TrafficQueue_t >();
      m_DownstreamQueue->emplace_back();
      auto& pkt = m_DownstreamQueue->back();
      pkt.first.resize(X.sz);
      std::copy_n(X.base, X.sz, pkt.first.begin());
      pkt.second = Y;
      return true;
    }
  }  // namespace path
}  // namespace llarp
