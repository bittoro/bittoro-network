#include <dht/explorenetworkjob.hpp>

#include <dht/context.hpp>
#include <dht/messages/findrouter.hpp>
#include <router/abstractrouter.hpp>

namespace llarp
{
  namespace dht
  {
    void
    ExploreNetworkJob::Start(const TXOwner &peer)
    {
      parent->DHTSendTo(peer.node.as_array(), new FindRouterMessage(peer.txid));
    }

    void
    ExploreNetworkJob::SendReply()
    {
      llarp::LogDebug("got ", valuesFound.size(), " routers from exploration");

      auto router = parent->GetRouter();
      using std::placeholders::_1;
      for(const auto &pk : valuesFound)
      {
        // lookup router
        parent->LookupRouter(
            pk,
            std::bind(&AbstractRouter::HandleDHTLookupForExplore, router, pk,
                      _1));
      }
    }
  }  // namespace dht
}  // namespace llarp
