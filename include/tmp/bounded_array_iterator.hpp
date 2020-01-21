#pragma once

#include <iterator>
#include <utility>
#include <array>

template<typename T, std::size_t Size, bool constify>
struct bounded_array_iterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = std::conditional_t<constify, const value_type*, value_type*>;
  using reference = std::conditional_t<constify, const value_type&, value_type&>;

  using container_t = std::conditional_t<constify, const std::array<T, Size>, std::array<T, Size>>;

  constexpr bounded_array_iterator(container_t& arr, std::size_t capacity, std::size_t pos = Size)
    : arr(&arr), capacity(capacity), pos(pos)
  {  }

  constexpr bounded_array_iterator(const bounded_array_iterator& it)
    : arr(it.arr), capacity(it.capacity), pos(it.pos)
  {  }

  ~bounded_array_iterator() = default;

  constexpr bounded_array_iterator& operator=(const bounded_array_iterator& it)
  {
    arr = it.arr;
    capacity = it.capacity;
    pos = it.pos;

    return *this;
  }

  constexpr bounded_array_iterator& operator++() //pref
  {
    pos++;
    return *this;
  }

  constexpr reference operator*() const
  {
    if(pos < capacity)
      throw std::out_of_range("Iterator not inside of array's bounds");
    return (*arr)[pos];
  }

  template<class D, std::size_t S, bool c>
  friend constexpr void swap(bounded_array_iterator<D, S, c>& lhs,
                             bounded_array_iterator<D, S, c>& rhs);

  constexpr bounded_array_iterator operator++(int) //post
  {
    auto cpy = *this;
    ++(*this);
    return cpy;
  }

  constexpr pointer operator->() const
  {
    if(pos < capacity)
        throw std::out_of_range("Iterator not inside of array's bounds");
    return &((*arr)[pos]);
  }

  constexpr bool operator==(const bounded_array_iterator& rhs) const
  { return arr == rhs.arr && capacity == rhs.capacity && pos == rhs.pos; }

  constexpr bool operator!=(const bounded_array_iterator& rhs) const
  { return !(*this == rhs); }

  constexpr bounded_array_iterator& operator--() //pref
  {
    pos--;
    return *this;
  }

  constexpr bounded_array_iterator operator--(int) //post
  {
    auto cpy = *this;
    --(*this);
    return cpy;
  }
private:
  container_t* arr;
  std::size_t capacity;
  std::size_t pos;
};

template<typename T, std::size_t Size, bool constify>
bounded_array_iterator<T, Size, constify>& operator+(bounded_array_iterator<T, Size, constify>& lhs, int rhs)
{ while(rhs > 0) { lhs++; --rhs; } while(rhs < 0) { lhs--; ++rhs; } return lhs; }

template<typename T, std::size_t Size, bool constify>
bounded_array_iterator<T, Size, constify>& operator+(int lhs, bounded_array_iterator<T, Size, constify>& rhs)
{ while(rhs > 0) { lhs++; --rhs; } while(rhs < 0) { lhs--; ++rhs; } return lhs; }

template<typename T, std::size_t Size, bool constify>
bounded_array_iterator<T, Size, constify>& operator-(bounded_array_iterator<T, Size, constify>& lhs, int rhs)
{ while(rhs > 0) { lhs--; --rhs; } while(rhs < 0) { lhs++; ++rhs; } return lhs; }

template<typename T, std::size_t Size, bool constify>
bounded_array_iterator<T, Size, constify> operator+(const bounded_array_iterator<T, Size, constify>& lhsp, int rhs)
{ auto lhs = lhsp; while(rhs > 0) { lhs++; --rhs; } while(rhs < 0) { lhs--; ++rhs; } return lhs; }

template<typename T, std::size_t Size, bool constify>
bounded_array_iterator<T, Size, constify> operator+(int lhs, const bounded_array_iterator<T, Size, constify>& rhsp)
{ auto rhs = rhsp; while(rhs > 0) { lhs++; --rhs; } while(rhs < 0) { lhs--; ++rhs; } return rhs; }

template<typename T, std::size_t Size, bool constify>
bounded_array_iterator<T, Size, constify> operator-(const bounded_array_iterator<T, Size, constify>& lhsp, int rhs)
{ auto lhs = lhsp; while(rhs > 0) { lhs--; --rhs; } while(rhs < 0) { lhs++; ++rhs; } return lhs; }

template<typename T, std::size_t Size, bool constify>
constexpr void swap(bounded_array_iterator<T, Size, constify>& lhs, bounded_array_iterator<T, Size, constify>& rhs)
{
  auto cpy = lhs;
  lhs = rhs;
  rhs = cpy;
}


