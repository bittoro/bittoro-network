#ifndef LLARP_DNS_STRING_HPP
#define LLARP_DNS_STRING_HPP

#include <string>

struct llarp_buffer_t;

namespace llarp
{
  namespace dns
  {
    using name_t = std::string;

    /// decode name from buffer
    bool
    decode_name(llarp_buffer_t* buf, name_t& name);

    /// encode name to buffer
    bool
    encode_name(llarp_buffer_t* buf, const name_t& name);

  }  // namespace dns
}  // namespace llarp

#endif
