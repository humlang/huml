#pragma once

#include <type_traits>
#include <cstdint>

struct nonesuch {
  ~nonesuch() = delete;
  nonesuch(nonesuch const&) = delete;
  void operator=(nonesuch const&) = delete;
};

namespace detail
{
  template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
  struct detector
  {
    using value_t = std::false_type;
    using type = Default;
  };

  template <class Default, template<class...> class Op, class... Args>
  struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
  {
    using value_t = std::true_type;
    using type = Op<Args...>;
  };

}
template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template<typename T>
using has_method_empty_trait = decltype(std::declval<T>().empty());

template<typename T>
constexpr bool has_method_empty()
{ return is_detected<has_method_empty_trait, T>::value; }


template<typename T, typename H = std::uint_fast32_t>
constexpr H hash_string(T str)
{
  if constexpr(has_method_empty<T>())
  {
    // probably a std::string like thing
    if(str.empty()) return 0;
  }
  else
  {
    // probably a c style string thing    (could also use a trait and throw error if this interface is also not provided)
    if(!str) return 0;
  }
  H hash = str[0];
  for(auto* p = &str[0]; p && *p != '\0'; p++)
    hash ^= (hash * 31) + (*p);
  return hash;
}

