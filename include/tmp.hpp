#pragma once

#include <type_traits>
#include <optional>
#include <variant>
#include <cstdint>
#include <cassert>
#include <array>

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

//// MAGICY GET
// allows to compress std::get<T0>(std::get<T1>(...(std::get<Tn>(variant))...)) calls into just one get call

namespace one
{
namespace detail
{

template<typename T, typename... Args>
constexpr bool is_in_there(const std::variant<Args...>& v)
{ return (std::is_same_v<T, Args> || ...); }

template<typename T, typename... Args>
constexpr bool is_in_there(std::variant<Args...>&& v)
{ return (std::is_same_v<T, Args> || ...); }

template<typename T>
constexpr inline static bool is_variant = false;
template<typename... Args>
constexpr inline static bool is_variant<std::variant<Args...>> = true;


template<std::size_t N>
struct num { static const constexpr auto value = N; };

template <class F, std::size_t... Is>
constexpr void for_(F&& func, std::index_sequence<Is...>)
{ (func(num<Is>{}), ...); }

template <std::size_t N, typename F>
constexpr void for_(F&& func)
{ for_(std::forward<F>(func), std::make_index_sequence<N>()); }

template<typename T, typename D>
constexpr std::optional<std::variant<std::monostate, T>> magical_get(D&&)
{ return std::monostate{}; }

template<typename T, int n = 0, typename... Args>
constexpr std::optional<std::variant<std::monostate, T>> magical_get(std::variant<Args...>&& w)
{
  if constexpr(is_in_there<T>(w) && n == 0)
    return std::get<T>(w);
  else
  {
    std::variant<std::monostate, T*> v;
    for_<sizeof...(Args)>([&w, &v](auto i)
    {
      if(auto x = std::get_if<i.value>(&w))
      {
        auto res = magical_get<T>(*x);

        if(res != std::nullopt && std::holds_alternative<T>(*res))
          v = *res;
      }
    });
    return v;
  }
}

template<typename T, typename D>
constexpr std::optional<std::variant<std::monostate, const T*>> magical_get(const D&)
{ return std::monostate{}; }

template<typename T, typename... Args>
constexpr std::optional<std::variant<std::monostate, const T*>> magical_get(const std::variant<Args...>& w)
{
  if constexpr(is_in_there<T>(w))
    return &std::get<T>(w);
  else
  {
    std::variant<std::monostate, const T*> v;
    for_<sizeof...(Args)>([&w, &v](auto i)
    {
      if(auto x = std::get_if<i.value>(&w))
      {
        auto res = magical_get<T>(*x);

        if(res != std::nullopt && std::holds_alternative<const T*>(*res))
          v = *res;
      }
    });
    return v;
  }
}

template<typename T, typename D>
constexpr std::optional<std::variant<std::monostate, T*>> magical_get(D&)
{ return std::monostate{}; }

template<typename T, typename... Args>
constexpr std::optional<std::variant<std::monostate, T*>> magical_get(std::variant<Args...>& w)
{
  if constexpr(is_in_there<T>(w))
    return &std::get<T>(w);
  else
  {
    std::variant<std::monostate, T*> v;
    for_<sizeof...(Args)>([&w, &v](auto i)
    {
      if(auto x = std::get_if<i.value>(&w))
      {
        auto res = magical_get<T>(*x);

        if(res != std::nullopt && std::holds_alternative<T*>(*res))
          v = *res;
      }
    });
    return v;
  }
}
}

template<typename T, typename... Args>
constexpr bool holds_alternative(std::variant<Args...>&& w)
{
  auto x = detail::magical_get<T>(w);

  return x != std::nullopt;
}

template<typename T, typename... Args>
constexpr bool holds_alternative(std::variant<Args...>& w)
{
  auto x = detail::magical_get<T>(w);

  return x != std::nullopt;
}

template<typename T, typename... Args>
constexpr bool holds_alternative(const std::variant<Args...>& w)
{
  auto x = detail::magical_get<T>(w);

  return x != std::nullopt;
}

template<typename T, typename... Args>
constexpr T get(std::variant<Args...>&& w)
{
  auto x = detail::magical_get<T>(w);

  assert(x != std::nullopt);
  assert(std::holds_alternative<T*>(*x));

  return std::move(*(std::get<T*>(*x)));
}

template<typename T, typename... Args>
constexpr T& get(std::variant<Args...>& w)
{
  auto x = detail::magical_get<T>(w);

  assert(x != std::nullopt);
  assert(std::holds_alternative<T*>(*x));
  
  return *(std::get<T*>(*x));
}

template<typename T, typename... Args>
constexpr const T& get(const std::variant<Args...>& w)
{
  auto x = detail::magical_get<T>(w);

  assert(x != std::nullopt);
  assert(std::holds_alternative<T*>(*x));
  
  return *(std::get<T*>(*x));
}

}


