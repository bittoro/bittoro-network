#include <dht/localrouterlookup.hpp>

#include <dht/context.hpp>
#include <dht/messages/gotrouter.hpp>

#include <path/path_context.hpp>
#include <router/abstractrouter.hpp>
#include <routing/dht_message.hpp>
#include <util/logging/logger.hpp>

namespace llarp
{
  namespace dht
  {
    LocalRouterLookup::LocalRouterLookup(const PathID_t &path, uint64_t txid,
                                         const RouterID &_target,
                                         AbstractContext *ctx)
        : RecursiveRouterLookup(TXOwner{ctx->OurKey(), txid}, _target, ctx,
                                nullptr)
        , localPath(path)
    {
    }

    void
    LocalRouterLookup::SendReply()
    {
      auto path = parent->GetRouter()->pathContext().GetByUpstream(
          parent->OurKey().as_array(), localPath);
      if(!path)
      {
        llarp::LogWarn(
            "did not send reply for relayed dht request, no such local path "
            "for pathid=",
            localPath);
        return;
      }
      if(valuesFound.size())
      {
        RouterContact found;
        for(const auto &rc : valuesFound)
        {
          if(rc.OtherIsNewer(found))
            found = rc;
        }
        valuesFound.clear();
        valuesFound.emplace_back(found);
      }
      routing::DHTMessage msg;
      msg.M.emplace_back(new GotRouterMessage(parent->OurKey(), whoasked.txid,
                                              valuesFound, true));
      if(!path->SendRoutingMessage(msg, parent->GetRouter()))
      {
        llarp::LogWarn(
            "failed to send routing message when informing result of dht "
            "request, pathid=",
            localPath);
      }
    }
  }  // namespace dht
}  // namespace llarp
