#ifndef LLARP_DHT_KADEMLIA_HPP
#define LLARP_DHT_KADEMLIA_HPP

#include <dht/key.hpp>

namespace llarp
{
  namespace dht
  {
    struct XorMetric
    {
      const Key_t us;

      XorMetric(const Key_t& ourKey) : us(ourKey)
      {
      }

      bool
      operator()(const Key_t& left, const Key_t& right) const
      {
        return (us ^ left) < (us ^ right);
      }
    };
  }  // namespace dht
}  // namespace llarp
#endif
