#ifndef LLARP_DNS_QUESTION_HPP
#define LLARP_DNS_QUESTION_HPP

#include <dns/serialize.hpp>
#include <dns/name.hpp>
#include <net/net_int.hpp>

namespace llarp
{
  namespace dns
  {
    using QType_t  = uint16_t;
    using QClass_t = uint16_t;

    struct Question : public Serialize
    {
      Question() = default;
      Question(Question&& other);
      Question(const Question& other);
      bool
      Encode(llarp_buffer_t* buf) const override;

      bool
      Decode(llarp_buffer_t* buf) override;

      std::ostream&
      print(std::ostream& stream, int level, int spaces) const;

      bool
      operator==(const Question& other) const
      {
        return qname == other.qname && qtype == other.qtype
            && qclass == other.qclass;
      }

      Name_t qname;
      QType_t qtype;
      QClass_t qclass;

      /// determine if we match a name
      bool
      IsName(const std::string& other) const;

      /// return qname with no trailing .
      std::string
      Name() const;

      /// determine if we are using this TLD
      bool
      HasTLD(const std::string& tld) const;
    };

    inline std::ostream&
    operator<<(std::ostream& out, const Question& q)
    {
      q.print(out, -1, -1);
      return out;
    }
  }  // namespace dns
}  // namespace llarp

#endif
