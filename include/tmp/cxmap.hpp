#pragma once

#include <tmp/bounded_array_iterator.hpp>
#include <tmp/cxpair.hpp>

#include <cassert>
#include <cstring>

template<typename Key, typename Value, class Compare = std::less<Key>, std::size_t Size = 2>
struct map
{
  using key_type = Key;
  using mapped_type = Value;
  using value_type = cxpair<Key, Value>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using key_compare = Compare;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = bounded_array_iterator<value_type, Size, false>;
  using const_iterator = bounded_array_iterator<value_type, Size, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr map(value_type(&&p)[Size])
    : map(std::begin(p), std::end(p))
  { }

  template<typename Itr>
  constexpr map(Itr begin, const Itr &end, Compare comp = Compare {})
  {
    while(begin != end)
    {
      push(begin);
      ++begin;
    }
  }

  constexpr map(map&& other)
    : data(other.data), capacity(other.capacity), cmp(other.cmp)
  {  }

  constexpr map(const map& other)
    : data(other.data), capacity(other.capacity), cmp(other.cmp)
  {  }

  constexpr map& operator=(map&& other)
  {
    data = other.data;
    capacity = other.capacity;
    cmp = other.cmp;

    return *this;
  }

  constexpr map& operator=(const map& other)
  {
    data = other.data;
    capacity = other.capacity;
    cmp = other.cmp;

    return *this;
  }

  constexpr map& operator=(cxpair<Key, Value>(&&p)[Size])
  {
    capacity = 0;
    auto begin = std::begin<Key, Value>(p);
    const auto end = std::end(p);
    while(begin != end)
    {
      push(begin);
      ++begin;
    }

    return *this;
  }

  constexpr map& operator=(const cxpair<Key, Value>(&p)[Size])
  {
    capacity = 0;
    auto begin = std::begin<Key, Value>(p);
    const auto end = std::end(p);
    while(begin != end)
    {
      push(begin);
      ++begin;
    }

    return *this;
  }

  constexpr mapped_type& at(const key_type& key)
  {
    for(auto& v : data)
      if(!cmp(v.first, key) && !cmp(key, v.first))
        return v.second;
    throw std::out_of_range("Element not found");
  }

  constexpr const mapped_type& at(const key_type& key) const
  {
    for(auto& v : data)
      if(!cmp(v.first, key) && !cmp(key, v.first))
        return v.second;
    throw std::out_of_range("Element not found");
  }

  constexpr mapped_type& operator[](const key_type& key)
  {
    for(auto& v : data)
      if(!cmp(v.first, key) && !cmp(key, v.first))
        return v.second;
    assert(false && "Element not found");
  }

  constexpr const mapped_type& operator[](const key_type& key) const
  {
    for(auto& v : data)
      if(!cmp(v.first, key) && !cmp(key, v.first))
        return v.second;
    assert(false && "Element not found");
  }

  constexpr iterator begin() noexcept
  { return { data, capacity, 0 }; }

  constexpr const_iterator begin() const noexcept
  { return { data, capacity, 0 }; }

  constexpr const_iterator cbegin() const noexcept
  { return { data, capacity, 0 }; }

  constexpr iterator end() noexcept
  { return { data, capacity }; }

  constexpr const_iterator end() const noexcept
  { return { data, capacity }; }

  constexpr const_iterator cend() const noexcept
  { return { data, capacity }; }

  [[nodiscard]] constexpr bool empty() const noexcept
  { return capacity == 0; }

  constexpr size_type size() const noexcept
  { return capacity; }

  constexpr size_type max_size() const noexcept
  { return Size; }

  constexpr size_type count(const Key& x) const
  {
    std::size_t i = 0;
    for(auto& v : data)
      if(!cmp(v.first, x) && !cmp(x, v.first))
        ++i;
    assert(i == 0 || i == 1);
    return i;
  }

  constexpr iterator find(const Key& key)
  {
    for(auto it = begin(); it != end(); ++it)
      if(!cmp(it->first, key) && !cmp(key, it->first))
        return it;
    return end();
  }

  constexpr const_iterator find(const Key& key) const
  {
    for(auto it = begin(); it != end(); ++it)
      if(!cmp(it->first, key) && !cmp(key, it->first))
        return it;
    return end();
  }

  constexpr bool contains( const Key& key ) const
  { return count(key) == 1; }

  constexpr std::pair<iterator, iterator> equal_range(const Key& key)
  { return std::make_pair(lower_bound(key), upper_bound(key)); }

  constexpr std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
  { return std::make_pair(lower_bound(key), upper_bound(key)); }

  constexpr iterator lower_bound(const Key& key)
  {
    auto first = begin(), last = end(), it = begin();
    typename std::iterator_traits<decltype(first)>::difference_type count, step;
    count = std::distance(first, last);

    while(count > 0)
    {
      it = first;
      step = count / 2;
      std::advance(it, step);
      if(comp(it->first, key))
      {
        first = ++it;
        count -= step + 1;
      }
      else
        count = step;
    }
    return first;
  }

  constexpr const_iterator lower_bound(const Key& key) const
  {
    auto first = begin(), last = end(), it = begin();
    typename std::iterator_traits<decltype(first)>::difference_type count, step;
    count = std::distance(first, last);

    while(count > 0)
    {
      it = first;
      step = count / 2;
      std::advance(it, step);
      if(comp(it->first, key))
      {
        first = ++it;
        count -= step + 1;
      }
      else
        count = step;
    }
    return first;
  }

  constexpr iterator upper_bound(const Key& key)
  {
    auto first = begin(), last = end(), it = begin();
    typename std::iterator_traits<decltype(first)>::difference_type count, step;
    count = std::distance(first, last);

    while(count > 0)
    {
      it = first;
      step = count / 2;
      std::advance(it, step);
      if(!comp(key, it->first))
      {
        first = ++it;
        count -= step + 1;
      }
      else
        count = step;
    }
    return first;
  }

  constexpr const_iterator upper_bound(const Key& key) const
  {
    auto first = begin(), last = end(), it = begin();
    typename std::iterator_traits<decltype(first)>::difference_type count, step;
    count = std::distance(first, last);

    while(count > 0)
    {
      it = first;
      step = count / 2;
      std::advance(it, step);
      if(!comp(key, it->first))
      {
        first = ++it;
        count -= step + 1;
      }
      else
        count = step;
    }
    return first;
  }

  constexpr key_compare key_comp() const
  { return {}; }

  constexpr void clear() noexcept
  { capacity = 0; }

  constexpr cxpair<iterator, bool> insert(const value_type& value)
  {
    auto it = begin();
    while(it != end())
    {
      if(!cmp(it->first, value.first) && !cmp(value.first, it->first))
        return { it, false }; // no insertion, this element is the reason
    }

    if(!(capacity + 1 < Size))
      throw std::out_of_range("Not enough space to insert stuff");
    data[capacity++] = value;

    return {begin() + capacity - 1, true};
  }

  constexpr cxpair<iterator, bool> insert(value_type&& value)
  {
    auto it = begin();
    while(it != end())
    {
      if(!cmp(it->first, value.first) && !cmp(value.first, it->first))
        return { it, false }; // no insertion, this element is the reason
    }
    if(!(capacity + 1 < Size))
      throw std::out_of_range("Not enough space to insert stuff");
    data[capacity++] = value;

    return {begin() + capacity - 1, true};
  }

  template<class InputIt>
  constexpr void insert(InputIt first, InputIt last)
  {
    auto dist = std::distance(first, last);
    if(!(capacity + 1 < Size))
      throw std::out_of_range("Not enough space to insert stuff");

    while(first != last)
    {
      insert(*first);
      ++first;
    }
  }

  constexpr void insert(std::initializer_list<value_type> ilist)
  { insert(ilist.begin(), ilist.end()); }

  template <class M>
  constexpr cxpair<iterator, bool> insert_or_assign(const key_type& k, M&& obj)
  {
    auto it = begin();
    while(it != end())
    {
      if(!cmp(it->first, k) && !cmp(k, it->first))
      {
        *it->second = std::move(obj);
        return { it, false }; // no insertion, this element is the reason
      }
    }
    if(!(capacity + 1 < Size))
      throw std::out_of_range("Not enough space to insert stuff");
    data[capacity++] = {k, std::move(obj)};

    return {begin() + capacity - 1, true};
  }

  template <class M>
  constexpr cxpair<iterator, bool> insert_or_assign(key_type&& k, M&& obj)
  {
    auto it = begin();
    while(it != end())
    {
      if(!cmp(it->first, k) && !cmp(k, it->first))
      {
        *it->second = std::move(obj);
        return { it, false }; // no insertion, this element is the reason
      }
    }
    if(!(capacity + 1 < Size))
      throw std::out_of_range("Not enough space to insert stuff");
    data[capacity++] = {k, std::move(obj)};

    return {begin() + capacity - 1, true};
  }

  template <class... Args>
  constexpr cxpair<iterator, bool> try_emplace(const key_type& k, Args&&... args)
  {
    auto it = begin();
    while(it != end())
    {
      if(!cmp(it->first, k) && !cmp(k, it->first))
        return { it, false }; // no insertion, this element is the reason
    }
    if(!(capacity + 1 < Size))
      throw std::out_of_range("Not enough space to insert stuff");

        // std::launder?
    new (&data[capacity++]) value_type { args... };

    return {begin() + capacity - 1, true};
  }

  template <class... Args>
  constexpr cxpair<iterator, bool> try_emplace(key_type&& k, Args&&... args)
  {
    auto it = begin();
    while(it != end())
    {
      if(!cmp(it->first, k) && !cmp(k, it->first))
        return { it, false }; // no insertion, this element is the reason
    }
    if(!(capacity + 1 < Size))
      throw std::out_of_range("Not enough space to insert stuff");

    // std::launder?
    new (&data[capacity++]) value_type {args...};

    return {begin() + capacity - 1, true};
  }

  constexpr iterator erase(iterator pos)
  {
    if(capacity == 0)
        throw std::runtime_error("Can't erase anything from empty map");
    auto dist = std::distance(pos, end());
    if(dist > 0)
    {
        std::memmove(pos, std::next(pos), dist);
        --capacity;
    }
    return pos;
  }

  constexpr iterator erase(const_iterator first, const_iterator last)
  {
    if(capacity == 0)
      throw std::runtime_error("Can't erase anything from empty map");
    auto dist = std::distance(first, last);

    if(capacity <= dist) // <- shouldn't this be an assertion?
      throw std::runtime_error("Can't erase more elements than present");

    while(first != last)
    {
      erase(first);
      ++first;
    }
    return first;
  }

  constexpr size_type erase(const key_type& key)
  {
    for(auto it = begin(); it != end(); ++it)
    {
      if(!cmp(it->first, key) && !cmp(key, it->first))
      {
        erase(it);
        return 1;
      }
    }
    return 0;
  }

  constexpr void swap(map& other) noexcept(std::is_nothrow_swappable<Compare>::value)
  { std::swap(other.data, data); std::swap(other.capacity, capacity); std::swap(other.cmp, cmp); }

  template<class C2, std::size_t s>
  constexpr void merge(map<Key, Value, C2, s>& source)
  {
    if(capacity + source.capacity < Size)
      throw std::out_of_range("Capacity is too small to merge maps");
    for(auto it = source.begin(); it != source.end(); ++it)
      insert(it);
  }

  template<class C2, std::size_t s>
  constexpr void merge(map<Key, Value, C2, s>&& source)
  {
    if(capacity + source.capacity < Size)
      throw std::out_of_range("Capacity is too small to merge maps");
    for(auto it = source.begin(); it != source.end(); ++it)
      insert(it);
  }

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator==(const map<Key,Value,Compare,Size>& lhs, const map<Key,Value,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator!=(const map<Key,Value,Compare,Size>& lhs, const map<Key,Value,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator<(const map<Key,Value,Compare,Size>& lhs, const map<Key,Value,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator<=(const map<Key,Value,Compare,Size>& lhs, const map<Key,Value,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator>(const map<Key,Value,Compare,Size>& lhs, const map<Key,Value,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator>=(const map<Key,Value,Compare,Size>& lhs, const map<Key,Value,Compare1,S1>& rhs);

  template<class K, class V, class C, std::size_t S, class C1, std::size_t S1>
  friend void swap(map<Key,Value,C,S>& lhs, map<Key,Value,C1,S1>& rhs)
    noexcept(std::declval<map<Key,Value,C,S>>().swap(std::declval<Key, Value, C1, S1>()));

  template<class Pred>
  friend void erase_if(map<Key,Value,Compare,Size>& c, Pred pred);
private:
  template<typename Itr>
  constexpr void push(Itr it)
  {
    if(capacity >= Size)
      throw std::range_error("Index past end of map's capacity");
    else
    {
      auto& v = data[capacity++];
      v = std::move(*it); // <- due to this, we need cxpair instead of std::pair
    }
      // TODO: sort to do binary search
  }
private:
  std::array<value_type, Size> data;
  std::size_t capacity { 0 };

  Compare cmp { };
};

template<class Key, class T, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator==(const map<Key,T,Compare0,S0>& lhs, const map<Key,T,Compare1,S1>& rhs)
{ return S0 == S1 && lhs.data == rhs.data; }

template<class Key, class T, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator!=(const map<Key,T,Compare0,S0>& lhs, const map<Key,T,Compare1,S1>& rhs)
{ return S0 != S1 || lhs.data != rhs.data; }

template<class Key, class T, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator<(const map<Key,T,Compare0,S0>& lhs, const map<Key,T,Compare1,S1>& rhs)
{ return S0 < S1 && lhs.data < rhs.data; }

template<class Key, class T, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator<=(const map<Key,T,Compare0,S0>& lhs, const map<Key,T,Compare1,S1>& rhs)
{ return S0 <= S1 && lhs.data <= rhs.data; }

template<class Key, class T, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator>(const map<Key,T,Compare0,S0>& lhs, const map<Key,T,Compare1,S1>& rhs)
{ return S0 > S1 && lhs.data > rhs.data; }

template<class Key, class T, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator>=(const map<Key,T,Compare0,S0>& lhs, const map<Key,T,Compare1,S1>& rhs)
{ return S0 >= S1 && lhs.data >= rhs.data; }

template<class Key, class Value, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
void swap(map<Key,Value,Compare0,S0>& lhs, map<Key,Value,Compare1,S1>& rhs)
    noexcept(std::declval<map<Key,Value,Compare0,S0>>().swap(std::declval<Key, Value, Compare1, S1>()))
{ lhs.swap(rhs); }

template<class Key, class Value, class Compare, std::size_t Size, class Pred>
void erase_if(map<Key,Value,Compare,Size>& c, Pred pred)
{
  for(auto i = c.begin(), last = c.end(); i != last; )
  {
    if(pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
}

template<class InputIt,
        class Comp = std::less<std::remove_const_t<typename std::iterator_traits<InputIt>::value_type::first_type>>,
        std::size_t Size = 2>
map(InputIt, InputIt, Comp = Comp())
        -> map<std::remove_const_t<typename std::iterator_traits<InputIt>::value_type::first_type>,
                typename std::iterator_traits<InputIt>::value_type::second_type, Comp, Size>;

template<class Key, class Value, std::size_t Size>
map(cxpair<Key, Value>(&&)[Size])
        -> map<Key, Value, std::less<Key>, Size>;

template<class Key, class Value, class Comparator, std::size_t Size>
map(cxpair<Key, Value>(&&)[Size], const Comparator& cmp)
        -> map<Key, Value, Comparator, Size>;


template<typename Key, typename Value, std::size_t Size>
constexpr auto make_map(cxpair<Key, Value>(&&m)[Size])
        -> map<Key, Value, std::less<Key>, Size>
{ return map<Key, Value, std::less<Key>, Size>(std::begin(m), std::end(m)); }

template<typename Key, typename Value, typename Comparator, std::size_t Size>
constexpr auto make_map(cxpair<Key, Value>(&&m)[Size])
        -> map<Key, Value, Comparator, Size>
{ return map<Key, Value, Comparator, Size>(std::begin(m), std::end(m)); }

