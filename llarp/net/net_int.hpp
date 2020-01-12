#ifndef LLARP_NET_INT_HPP
#define LLARP_NET_INT_HPP

// for addrinfo
#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#define inet_aton(x, y) inet_pton(AF_INET, x, y)
#endif

#include <net/net.h>

#include <cstdlib>  // for itoa
#include <iostream>
#include <util/endian.hpp>
#include <vector>

#include <absl/numeric/int128.h>
#include <absl/hash/hash.h>

namespace llarp
{
  template < typename UInt_t >
  struct huint_t
  {
    UInt_t h;

    constexpr huint_t operator&(huint_t x) const
    {
      return huint_t{UInt_t{h & x.h}};
    }

    constexpr huint_t
    operator|(huint_t x) const
    {
      return huint_t{UInt_t{h | x.h}};
    }

    constexpr huint_t
    operator-(huint_t x) const
    {
      return huint_t{UInt_t{h - x.h}};
    }

    constexpr huint_t
    operator+(huint_t x) const
    {
      return huint_t{UInt_t{h + x.h}};
    }

    constexpr huint_t
    operator^(huint_t x) const
    {
      return huint_t{UInt_t{h ^ x.h}};
    }

    constexpr huint_t
    operator~() const
    {
      return huint_t{UInt_t{~h}};
    }

    constexpr huint_t
    operator<<(int n) const
    {
      UInt_t v{h};
      v <<= n;
      return huint_t{v};
    }

    inline huint_t
    operator++()
    {
      ++h;
      return *this;
    }

    inline huint_t
    operator--()
    {
      --h;
      return *this;
    }

    constexpr bool
    operator<(huint_t x) const
    {
      return h < x.h;
    }

    constexpr bool
    operator!=(huint_t x) const
    {
      return h != x.h;
    }

    constexpr bool
    operator==(huint_t x) const
    {
      return h == x.h;
    }

    using Hash = absl::Hash< huint_t< UInt_t > >;

    template < typename H >
    friend H
    AbslHashValue(H hash, const huint_t< UInt_t >& i)
    {
      return H::combine(std::move(hash), i.h);
    }

    using V6Container = std::vector< uint8_t >;
    void
    ToV6(V6Container& c);

    std::string
    ToString() const;

    bool
    FromString(const std::string&);

    friend std::ostream&
    operator<<(std::ostream& out, const huint_t& i)
    {
      return out << i.ToString();
    }
  };

  using huint32_t  = huint_t< uint32_t >;
  using huint16_t  = huint_t< uint16_t >;
  using huint128_t = huint_t< absl::uint128 >;

  template < typename UInt_t >
  struct nuint_t
  {
    UInt_t n;

    constexpr nuint_t operator&(nuint_t x) const
    {
      return nuint_t{UInt_t(n & x.n)};
    }

    constexpr nuint_t
    operator|(nuint_t x) const
    {
      return nuint_t{UInt_t(n | x.n)};
    }

    constexpr nuint_t
    operator^(nuint_t x) const
    {
      return nuint_t{UInt_t(n ^ x.n)};
    }

    constexpr nuint_t
    operator~() const
    {
      return nuint_t{UInt_t(~n)};
    }

    inline nuint_t
    operator++()
    {
      ++n;
      return *this;
    }
    inline nuint_t
    operator--()
    {
      --n;
      return *this;
    }

    constexpr bool
    operator<(nuint_t x) const
    {
      return n < x.n;
    }

    constexpr bool
    operator==(nuint_t x) const
    {
      return n == x.n;
    }

    struct Hash
    {
      inline size_t
      operator()(nuint_t x) const
      {
        return std::hash< UInt_t >{}(x.n);
      }
    };

    using V6Container = std::vector< uint8_t >;
    void
    ToV6(V6Container& c);

    std::string
    ToString() const;

    friend std::ostream&
    operator<<(std::ostream& out, const nuint_t& i)
    {
      return out << i.ToString();
    }
  };

  using nuint32_t  = nuint_t< uint32_t >;
  using nuint16_t  = nuint_t< uint16_t >;
  using nuint128_t = nuint_t< absl::uint128 >;

  static inline nuint32_t
  xhtonl(huint32_t x)
  {
    return nuint32_t{htonl(x.h)};
  }

  static inline huint32_t
  xntohl(nuint32_t x)
  {
    return huint32_t{ntohl(x.n)};
  }

  static inline nuint16_t
  xhtons(huint16_t x)
  {
    return nuint16_t{htons(x.h)};
  }

  static inline huint16_t
  xntohs(nuint16_t x)
  {
    return huint16_t{ntohs(x.n)};
  }
}  // namespace llarp

#endif
