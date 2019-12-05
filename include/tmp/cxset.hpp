#pragma once

#include <tmp/bounded_array_iterator.hpp>
#include <tmp/cxpair.hpp>

#include <cassert>
#include <cstring>

// TODO: make this to a HAT-trie
//       i.e.:
//
//       [ 0 ][ 1 ]...[ 'h' ]...[ 254 ][ 255 ]
//                      / \
//                    /     \
//                  "e        [ 0 ][ 1 ]...[ 'o' ]...[254][255]
//                   l                       / \
//                   l                     "p   "l
//                   o"                     e"   e"

template<typename Key, class Compare = std::less<Key>, std::size_t Size = 2>
struct set
{
  using key_type = Key;
  using value_type = Key;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using key_compare = Compare;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = bounded_array_iterator<value_type, Size, false>;
  using const_iterator = bounded_array_iterator<value_type, Size, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr set(value_type(&&p)[Size])
    : set(std::begin(p), std::end(p))
  { }

  template<typename Itr>
  constexpr set(Itr begin, const Itr &end, Compare comp = Compare {})
  {
    while(begin != end)
    {
      push(begin);
      ++begin;
    }
  }

  constexpr set(set&& other)
    : data(other.data), capacity(other.capacity), cmp(other.cmp)
  {  }

  constexpr set(const set& other)
    : data(other.data), capacity(other.capacity), cmp(other.cmp)
  {  }

  constexpr set& operator=(set&& other)
  {
    data = other.data;
    capacity = other.capacity;
    cmp = other.cmp;

    return *this;
  }

  constexpr set& operator=(const set& other)
  {
    data = other.data;
    capacity = other.capacity;
    cmp = other.cmp;

    return *this;
  }

  constexpr set& operator=(Key(&&p)[Size])
  {
    capacity = 0;
    auto begin = std::begin<Key>(p);
    const auto end = std::end(p);
    while(begin != end)
    {
      push(begin);
      ++begin;
    }

    return *this;
  }

  constexpr set& operator=(const Key(&p)[Size])
  {
    capacity = 0;
    auto begin = std::begin<Key>(p);
    const auto end = std::end(p);
    while(begin != end)
    {
      push(begin);
      ++begin;
    }

    return *this;
  }

  constexpr value_type& at(const key_type& key)
  {
    for(auto& v : data)
      if(!cmp(v, key) && !cmp(key, v))
        return v;
    throw std::out_of_range("Element not found");
  }

  constexpr const value_type& at(const key_type& key) const
  {
    for(auto& v : data)
      if(!cmp(v, key) && !cmp(key, v))
        return v;
    throw std::out_of_range("Element not found");
  }

  constexpr value_type& operator[](const key_type& key)
  {
    for(auto& v : data)
      if(!cmp(v, key) && !cmp(key, v))
        return v;
    assert(false && "Element not found");
  }

  constexpr const value_type& operator[](const key_type& key) const
  {
    for(auto& v : data)
      if(!cmp(v, key) && !cmp(key, v))
        return v;
    assert(false && "Element not found");
  }

  constexpr iterator begin() noexcept
  { return { data }; }

  constexpr const_iterator begin() const noexcept
  { return { data }; }

  constexpr const_iterator cbegin() const noexcept
  { return { data }; }

  constexpr iterator end() noexcept
  { return { data }; }

  constexpr const_iterator end() const noexcept
  { return { data }; }

  constexpr const_iterator cend() const noexcept
  { return { data }; }

  [[nodiscard]] constexpr bool empty() const noexcept
  { return capacity; }

  constexpr size_type size() const noexcept
  { return capacity; }

  constexpr size_type max_size() const noexcept
  { return Size; }

  constexpr size_type count(const Key& x) const
  {
    std::size_t i = 0;
    for(auto& v : data)
      if(!cmp(v, x) && !cmp(x, v))
        ++i;
    assert(i == 0 || i == 1);
    return i;
  }

  constexpr iterator find(const Key& key)
  {
    for(auto it = begin(); it != end(); ++it)
      if(!cmp(*it, key) && !cmp(key, *it))
        return it;
    return end();
  }

  constexpr const_iterator find(const Key& key) const
  {
    for(auto it = begin(); it != end(); ++it)
      if(!cmp(*it, key) && !cmp(key, *it))
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
      if(comp(*it, key))
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
      if(comp(*it, key))
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
      if(!comp(key, *it))
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
      if(!comp(key, *it))
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
        throw std::runtime_error("Can't erase anything from empty set");
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
      throw std::runtime_error("Can't erase anything from empty set");
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

  constexpr void swap(set& other) noexcept(std::is_nothrow_swappable<Compare>::value)
  { std::swap(other.data, data); std::swap(other.capacity, capacity); std::swap(other.cmp, cmp); }

  template<class C2, std::size_t s>
  constexpr void merge(set<Key, C2, s>& source)
  {
    if(capacity + source.capacity < Size)
      throw std::out_of_range("Capacity is too small to merge sets");
    for(auto it = source.begin(); it != source.end(); ++it)
      insert(it);
  }

  template<class C2, std::size_t s>
  constexpr void merge(set<Key, C2, s>&& source)
  {
    if(capacity + source.capacity < Size)
      throw std::out_of_range("Capacity is too small to merge sets");
    for(auto it = source.begin(); it != source.end(); ++it)
      insert(it);
  }

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator==(const set<Key,Compare,Size>& lhs, const set<Key,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator!=(const set<Key,Compare,Size>& lhs, const set<Key,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator<(const set<Key,Compare,Size>& lhs, const set<Key,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator<=(const set<Key,Compare,Size>& lhs, const set<Key,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator>(const set<Key,Compare,Size>& lhs, const set<Key,Compare1,S1>& rhs);

  template<class Compare1, std::size_t S1>
  friend constexpr bool operator>=(const set<Key,Compare,Size>& lhs, const set<Key,Compare1,S1>& rhs);

  template<class K, class V, class C, std::size_t S, class C1, std::size_t S1>
  friend void swap(set<Key,C,S>& lhs, set<Key,C1,S1>& rhs)
    noexcept(std::declval<set<Key,C,S>>().swap(std::declval<Key, C1, S1>()));

  template<class Pred>
  friend void erase_if(set<Key,Compare,Size>& c, Pred pred);
private:
  template<typename Itr>
  constexpr void push(Itr it)
  {
    if(capacity >= Size)
      throw std::range_error("Index past end of set's capacity");
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

template<class Key, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator==(const set<Key,Compare0,S0>& lhs, const set<Key,Compare1,S1>& rhs)
{ return S0 == S1 && lhs.data == rhs.data; }

template<class Key, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator!=(const set<Key,Compare0,S0>& lhs, const set<Key,Compare1,S1>& rhs)
{ return S0 != S1 || lhs.data != rhs.data; }

template<class Key, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator<(const set<Key,Compare0,S0>& lhs, const set<Key,Compare1,S1>& rhs)
{ return S0 < S1 && lhs.data < rhs.data; }

template<class Key, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator<=(const set<Key,Compare0,S0>& lhs, const set<Key,Compare1,S1>& rhs)
{ return S0 <= S1 && lhs.data <= rhs.data; }

template<class Key, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator>(const set<Key,Compare0,S0>& lhs, const set<Key,Compare1,S1>& rhs)
{ return S0 > S1 && lhs.data > rhs.data; }

template<class Key, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
constexpr bool operator>=(const set<Key,Compare0,S0>& lhs, const set<Key,Compare1,S1>& rhs)
{ return S0 >= S1 && lhs.data >= rhs.data; }

template<class Key, class Compare0, class Compare1, std::size_t S0, std::size_t S1>
void swap(set<Key,Compare0,S0>& lhs, set<Key,Compare1,S1>& rhs)
    noexcept(std::declval<set<Key,Compare0,S0>>().swap(std::declval<Key, Compare1, S1>()))
{ lhs.swap(rhs); }

template<class Key, class Compare, std::size_t Size, class Pred>
void erase_if(set<Key,Compare,Size>& c, Pred pred)
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
set(InputIt, InputIt, Comp = Comp())
        -> set<std::remove_const_t<typename std::iterator_traits<InputIt>::value_type::first_type>, Comp, Size>;

template<class Key, std::size_t Size>
set(Key(&&)[Size])
        -> set<Key, std::less<Key>, Size>;

template<class Key, class Comparator, std::size_t Size>
set(Key(&&)[Size], const Comparator& cmp)
        -> set<Key, Comparator, Size>;


template<class Key, std::size_t Size>
constexpr auto make_set(Key(&&m)[Size])
        -> set<Key, std::less<Key>, Size>
{ return set<Key, std::less<Key>, Size>(std::begin(m), std::end(m)); }

template<class Key, typename Comparator, std::size_t Size>
constexpr auto make_set(Key(&&m)[Size])
        -> set<Key, Comparator, Size>
{ return set<Key, Comparator, Size>(std::begin(m), std::end(m)); }

