#ifndef LLARP_STR_HPP
#define LLARP_STR_HPP

#include <util/string_view.hpp>

namespace llarp
{
  bool
  StrEq(const char *s1, const char *s2);

  bool
  IsFalseValue(string_view str);

  struct CaselessCmp
  {
    bool
    operator()(string_view lhs, string_view rhs) const;
  };

  bool
  IsTrueValue(string_view str);

}  // namespace llarp

#endif
