#pragma once

#include <tuple>

template<typename T0, typename T1>
struct cxpair
{
  using first_type = T0;
  using second_type = T1;

  constexpr cxpair()
      : first(), second()
  {  }

  constexpr cxpair(first_type&& first, second_type&& second)
      : first(first), second(second)
  {  }

  constexpr cxpair(std::tuple<first_type, second_type>&& tup)
      : first(std::get<0>(tup)), second(std::get<1>(tup))
  {  }

  constexpr cxpair(cxpair&& other)
      : first(other.first), second(other.second)
  {  }

  constexpr cxpair(const cxpair& other)
      : first(other.first), second(other.second)
  {  }

  constexpr cxpair& operator=(cxpair<T0, T1>&& other)
  { first = other.first; second = other.second; return *this; }

  constexpr cxpair& operator=(const cxpair<T0, T1>& other)
  { first = other.first; second = other.second; return *this; }

  constexpr cxpair& operator==(const cxpair<T0, T1>& other)
  { return first == other.first && second == other.second; }

  constexpr cxpair& operator!=(const cxpair<T0, T1>& other)
  { return first != other.first || second != other.second; }

  constexpr cxpair& operator<=(const cxpair<T0, T1>& other)
  { return first <= other.first && second <= other.second; }

  constexpr cxpair& operator<(const cxpair<T0, T1>& other)
  { return first < other.first && second < other.second; }

  constexpr cxpair& operator>(const cxpair<T0, T1>& other)
  { return first > other.first && second > other.second; }

  constexpr cxpair& operator>=(const cxpair<T0, T1>& other)
  { return first >= other.first && second >= other.second; }

  T0 first;
  T1 second;
};

