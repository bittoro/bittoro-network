#ifndef LLARP_ROUTER_ID_HPP
#define LLARP_ROUTER_ID_HPP

#include <util/aligned.hpp>
#include <util/status.hpp>

namespace llarp
{
  struct RouterID : public AlignedBuffer< 32 >
  {
    static constexpr size_t SIZE = 32;

    using Data = std::array< byte_t, SIZE >;

    RouterID()
    {
    }

    RouterID(const byte_t* buf) : AlignedBuffer< SIZE >(buf)
    {
    }

    RouterID(const Data& data) : AlignedBuffer< SIZE >(data)
    {
    }

    util::StatusObject
    ExtractStatus() const;

    std::string
    ToString() const;

    bool
    FromString(const std::string& str);

    RouterID&
    operator=(const byte_t* ptr)
    {
      std::copy(ptr, ptr + SIZE, begin());
      return *this;
    }

    friend std::ostream&
    operator<<(std::ostream& out, const RouterID& id)
    {
      return out << id.ToString();
    }

    using Hash = AlignedBuffer< SIZE >::Hash;
  };

  inline bool
  operator==(const RouterID& lhs, const RouterID& rhs)
  {
    return lhs.as_array() == rhs.as_array();
  }

}  // namespace llarp

#endif
