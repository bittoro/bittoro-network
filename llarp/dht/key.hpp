#ifndef LLARP_DHT_KEY_HPP
#define LLARP_DHT_KEY_HPP

#include <util/aligned.hpp>
#include <router_id.hpp>

#include <array>

namespace llarp
{
  namespace dht
  {
    struct Key_t : public AlignedBuffer< 32 >
    {
      explicit Key_t(const byte_t* buf) : AlignedBuffer< SIZE >(buf)
      {
      }

      explicit Key_t(const Data& data) : AlignedBuffer< SIZE >(data)
      {
      }

      explicit Key_t(const AlignedBuffer< SIZE >& data)
          : AlignedBuffer< SIZE >(data)
      {
      }

      Key_t() : AlignedBuffer< SIZE >()
      {
      }

      /// get snode address string
      std::string
      SNode() const
      {
        const RouterID rid{as_array()};
        return rid.ToString();
      }

      std::string
      ToString() const
      {
        return SNode();
      }

      Key_t
      operator^(const Key_t& other) const
      {
        Key_t dist;
        std::transform(begin(), end(), other.begin(), dist.begin(),
                       std::bit_xor< byte_t >());
        return dist;
      }

      bool
      operator==(const Key_t& other) const
      {
        return as_array() == other.as_array();
      }

      bool
      operator!=(const Key_t& other) const
      {
        return as_array() != other.as_array();
      }

      bool
      operator<(const Key_t& other) const
      {
        return as_array() < other.as_array();
      }

      bool
      operator>(const Key_t& other) const
      {
        return as_array() > other.as_array();
      }
    };
  }  // namespace dht
}  // namespace llarp

#endif
