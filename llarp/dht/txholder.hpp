#ifndef LLARP_DHT_TXHOLDER
#define LLARP_DHT_TXHOLDER

#include <dht/tx.hpp>
#include <dht/txowner.hpp>
#include <util/time.hpp>
#include <util/status.hpp>

#include <memory>
#include <unordered_map>

namespace llarp
{
  namespace dht
  {
    template < typename K, typename V, typename K_Hash,
               llarp_time_t requestTimeoutMS = 30000UL >
    struct TXHolder
    {
      using TXPtr = std::unique_ptr< TX< K, V > >;
      // tx who are waiting for a reply for each key
      std::unordered_multimap< K, TXOwner, K_Hash > waiting;
      // tx timesouts by key
      std::unordered_map< K, llarp_time_t, K_Hash > timeouts;
      // maps remote peer with tx to handle reply from them
      std::unordered_map< TXOwner, TXPtr, TXOwner::Hash > tx;

      const TX< K, V >*
      GetPendingLookupFrom(const TXOwner& owner) const;

      util::StatusObject
      ExtractStatus() const
      {
        util::StatusObject obj{};
        std::vector< util::StatusObject > txObjs, timeoutsObjs, waitingObjs;
        std::transform(tx.begin(), tx.end(), std::back_inserter(txObjs),
                       [](const auto& item) -> util::StatusObject {
                         return util::StatusObject{
                             {"owner", item.first.ExtractStatus()},
                             {"tx", item.second->ExtractStatus()}};
                       });
        obj["tx"] = txObjs;
        std::transform(
            timeouts.begin(), timeouts.end(), std::back_inserter(timeoutsObjs),
            [](const auto& item) -> util::StatusObject {
              return util::StatusObject{{"time", item.second},
                                        {"target", item.first.ToString()}};
            });
        obj["timeouts"] = timeoutsObjs;
        std::transform(waiting.begin(), waiting.end(),
                       std::back_inserter(waitingObjs),
                       [](const auto& item) -> util::StatusObject {
                         return util::StatusObject{
                             {"target", item.first.ToString()},
                             {"whoasked", item.second.ExtractStatus()}};
                       });
        obj["waiting"] = waitingObjs;
        return obj;
      }

      bool
      HasLookupFor(const K& target) const
      {
        return timeouts.find(target) != timeouts.end();
      }

      bool
      HasPendingLookupFrom(const TXOwner& owner) const
      {
        return GetPendingLookupFrom(owner) != nullptr;
      }

      void
      NewTX(const TXOwner& askpeer, const TXOwner& whoasked, const K& k,
            TX< K, V >* t);

      /// mark tx as not fond
      void
      NotFound(const TXOwner& from, const std::unique_ptr< Key_t >& next);

      void
      Found(const TXOwner& from, const K& k, const std::vector< V >& values)
      {
        Inform(from, k, values, true);
      }

      /// inform all watches for key of values found
      void
      Inform(TXOwner from, K key, std::vector< V > values,
             bool sendreply = false, bool removeTimeouts = true);

      void
      Expire(llarp_time_t now);
    };

    template < typename K, typename V, typename K_Hash,
               llarp_time_t requestTimeoutMS >
    const TX< K, V >*
    TXHolder< K, V, K_Hash, requestTimeoutMS >::GetPendingLookupFrom(
        const TXOwner& owner) const
    {
      auto itr = tx.find(owner);
      if(itr == tx.end())
      {
        return nullptr;
      }

      return itr->second.get();
    }

    template < typename K, typename V, typename K_Hash,
               llarp_time_t requestTimeoutMS >
    void
    TXHolder< K, V, K_Hash, requestTimeoutMS >::NewTX(const TXOwner& askpeer,
                                                      const TXOwner& whoasked,
                                                      const K& k, TX< K, V >* t)
    {
      (void)whoasked;
      tx.emplace(askpeer, std::unique_ptr< TX< K, V > >(t));
      auto count = waiting.count(k);
      waiting.emplace(k, askpeer);

      auto itr = timeouts.find(k);
      if(itr == timeouts.end())
      {
        timeouts.emplace(k, time_now_ms() + requestTimeoutMS);
      }
      if(count == 0)
      {
        t->Start(askpeer);
      }
    }

    template < typename K, typename V, typename K_Hash,
               llarp_time_t requestTimeoutMS >
    void
    TXHolder< K, V, K_Hash, requestTimeoutMS >::NotFound(
        const TXOwner& from, const std::unique_ptr< Key_t >& next)
    {
      auto txitr = tx.find(from);
      if(txitr == tx.end())
      {
        return;
      }

      // ask for next peer
      if(!txitr->second->AskNextPeer(from.node, next))
        Inform(from, txitr->second->target, {}, true, true);
    }

    template < typename K, typename V, typename K_Hash,
               llarp_time_t requestTimeoutMS >
    void
    TXHolder< K, V, K_Hash, requestTimeoutMS >::Inform(TXOwner from, K key,
                                                       std::vector< V > values,
                                                       bool sendreply,
                                                       bool removeTimeouts)
    {
      auto range = waiting.equal_range(key);
      auto itr   = range.first;
      while(itr != range.second)
      {
        auto txitr = tx.find(itr->second);
        if(txitr != tx.end())
        {
          for(const auto& value : values)
          {
            txitr->second->OnFound(from.node, value);
          }
          if(sendreply)
          {
            txitr->second->SendReply();
            tx.erase(txitr);
          }
        }
        ++itr;
      }

      if(sendreply)
      {
        waiting.erase(key);
      }

      if(removeTimeouts)
      {
        timeouts.erase(key);
      }
    }

    template < typename K, typename V, typename K_Hash,
               llarp_time_t requestTimeoutMS >
    void
    TXHolder< K, V, K_Hash, requestTimeoutMS >::Expire(llarp_time_t now)
    {
      auto itr = timeouts.begin();
      while(itr != timeouts.end())
      {
        if(now > itr->second && now - itr->second >= requestTimeoutMS)
        {
          Inform(TXOwner{}, itr->first, {}, true, false);
          itr = timeouts.erase(itr);
        }
        else
        {
          ++itr;
        }
      }
    }
  }  // namespace dht
}  // namespace llarp

#endif
