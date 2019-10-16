#pragma once

// template hacks

#include <type_traits>

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

namespace detail
{
  template<class L, class R>
  inline L* scast(R* r) {
    static_assert(std::is_base_of<R, L>::value, "R is not a base type of L");
    assert((!r || dynamic_cast<L*>(r)) && "cast not possible" );
    return static_cast<L*>(r);
  }

  template<class L, class R>
  inline const L* scast(const R* r) { return const_cast<const L*>(scast<L, R>(const_cast<R*>(r))); }

  template<class L, class R>
  inline L* dcast(R* u) {
    static_assert(std::is_base_of<R, L>::value, "R is not a base type of L");
    return dynamic_cast<L*>(u);
  }

  template<class L, class R>
  inline const L* dcast(const R* r) { return const_cast<const L*>(dcast<L, R>(const_cast<R*>(r))); }

  template<class L, class R>
  inline L bcast(const R& from) {
    static_assert(sizeof(R) == sizeof(L), "size mismatch for bitcast");
    L to;
    memcpy(&to, &from, sizeof(L));
    return to;
  }
}

template<class D>
class runtime_cast {
public:
  template<class T>
  T* as()
  { return detail::scast<T>(static_cast<D*>(this)); }

  template<class T>
  T* isa()
  { return detail::dcast<T>(static_cast<D*>(this)); }

  template<class T>
  const T* as() const
  { return detail::scast<T>(static_cast<const D*>(this)); }

  template<class T>
  const T* isa() const
  { return detail::dcast<T>(static_cast<const D*>(this)); }
};

