#pragma once

#include <cassert>
#include <memory>
#include <vector>
#include <variant>

template<typename T>
struct monotonic_buffer
{
public:
  union monotonic_item
  {
  public:
    monotonic_item* get_next_item() const { return next; }
    void set_next_item(monotonic_item* n) { next = n; }
 
    T* get_storage() { return reinterpret_cast<T*>(object); }
 
    static monotonic_item* storage_to_item(T* t)
    { return reinterpret_cast<monotonic_item*>(t); }
  private:
    monotonic_item* next;
    alignas(alignof(T)) char object[sizeof(T)];
  };

  struct monotonic_arena
  {
  private:
    std::unique_ptr<monotonic_item[]> storage;
    std::unique_ptr<monotonic_arena> next;

    monotonic_arena(std::size_t arena_size)
      : storage(new monotonic_item[arena_size])
    {
      for(std::size_t i = 1; i < arena_size; i++)
        storage[i - 1].set_next_item(&storage[i]);
      storage[arena_size - 1].set_next_item(nullptr);
    }

    monotonic_item* get_storage() const { return storage.get(); }

    void set_next_arena(std::unique_ptr<monotonic_arena>&& n)
    {
      assert(!next);
 
      next.reset(n.release());
    }
  };

public:
  monotonic_buffer(std::size_t arena_size)
      : arena_size(arena_size), arena(new monotonic_arena(arena_size)),
        free_list(arena->get_storage())
  {  }

  template<typename... Args>
  T* alloc(Args&&... args)
  {
    if(free_list == nullptr)
    {
      std::unique_ptr<monotonic_arena> new_arena(new monotonic_arena(arena_size));
      new_arena->set_next_arena(std::move(arena));
      arena.reset(new_arena.release());
      free_list = arena->get_storage();
    }
 
    monotonic_item* current_item = free_list;
    free_list = current_item->get_next_item();
 
    T* result = current_item->get_storage();
    new (result) T(std::forward<Args>(args)...);
 
    return result;
  }

  void free(T* t)
  {
    t->T::~T();
 
    monotonic_item* current_item = monotonic_item::storage_to_item(t);
 
    current_item->set_next_item(free_list);
    free_list = current_item;
  }

private:
  std::size_t arena_size;
  std::unique_ptr<monotonic_arena> arena;
  monotonic_item *free_list;
}; 

template<std::size_t Len, class... Types>
struct aligned_variant
{
  static constexpr std::size_t alignment_value = std::max({alignof(Types)...});
 
  struct type
  {
    alignas(alignment_value) unsigned char _s[std::max({Len, sizeof(Types)...})];
  };
};

template<typename T>
void destroy(T& t)
{ t.~T(); }

template<class... Types>
struct aligned_variant_vec
{
  using aligned_t = typename aligned_variant<1U, Types...>::type;
  using container_t = std::vector<aligned_t>;

  static constexpr std::size_t alignment_value = aligned_variant<1U, Types...>::alignment_value;


  template<std::size_t n, typename T, typename H, typename... Tl>
  struct find_id
  {
    static constexpr std::size_t value = (std::is_same_v<T, H> ? n : find_id<n + 1, T, Tl...>::value);
  };
  template<std::size_t n, typename T, typename H>
  struct find_id<n, T, H>
  {
    static constexpr std::size_t value = (std::is_same_v<T, H> ? n : -1);
  };

  template<std::size_t n, typename H, typename... Args>
  struct get_t
  {
    using type = std::conditional_t<n == 0, H, typename get_t<n - 1, Args...>::type>;
  };
  template<std::size_t n, typename H>
  struct get_t<n, H>
  {
    using type = std::conditional_t<n == 0, H, unsigned char>;
  };


  template<typename T, typename... Args>
  void emplace_back(Args&&... args)
  {
    static_assert((std::is_same_v<T, Types> || ...), "Type must occur in type list.");

    if(object_count + 1 >= data.capacity())
    {
      data.resize(data.capacity() == 0 ? 1 : data.capacity() << 1);
    }

    auto* obj = &data[object_count];

    new (obj) T(args...);

    types.push_back(find_id<0, T, Types...>::value);
    object_count++;
  }

  bool empty() { return object_count == 0; }

  template<typename T>
  T& back()
  { return *reinterpret_cast<T*>(&data[(object_count - 1)]); }

  std::variant<std::monostate, std::add_pointer_t<Types>...> operator[](std::size_t i)
  {
    // We need to explicitly hardcode any possible condition.
    switch(types[i])
    {
#define rcase(num) \
      case num: if constexpr(num < sizeof...(Types)) return reinterpret_cast<typename get_t<num, Types...>::type*>(&data[i]); break

      rcase(0); rcase(1); rcase(2); rcase(3); rcase(4); rcase(5); rcase(6); rcase(7); rcase(8); rcase(9);
      rcase(10); rcase(11); rcase(12); rcase(13); rcase(14); rcase(15); rcase(16); rcase(17); rcase(18); rcase(19);
      rcase(20); rcase(21); rcase(22); rcase(23); rcase(24); rcase(25); rcase(26); rcase(27); rcase(28); rcase(29);
      rcase(30); rcase(31); rcase(32); rcase(33); rcase(34); rcase(35); rcase(36); rcase(37); rcase(38); rcase(39);
      rcase(40); rcase(41); rcase(42); rcase(43); rcase(44); rcase(45); rcase(46); rcase(47); rcase(48); rcase(49);

#undef rcase
      default: assert(false && "Error: Hardcode more rcases to allow larger lists.");
    }
    return std::monostate {};
  }

  template<typename F>
  void use_in(std::size_t i, F&& ctx)
  {
    static_assert(!std::is_same_v<unsigned char, typename get_t<8, Types...>::type>, "");
    // We need to explicitly hardcode any possible condition.
    switch(types[i])
    {
#define rcase(num) \
      case num: if constexpr(num < sizeof...(Types)) ctx(*reinterpret_cast<typename get_t<num, Types...>::type*>(&data[i])); break

      rcase(0); rcase(1); rcase(2); rcase(3); rcase(4); rcase(5); rcase(6); rcase(7); rcase(8); rcase(9);
      rcase(10); rcase(11); rcase(12); rcase(13); rcase(14); rcase(15); rcase(16); rcase(17); rcase(18); rcase(19);
      rcase(20); rcase(21); rcase(22); rcase(23); rcase(24); rcase(25); rcase(26); rcase(27); rcase(28); rcase(29);
      rcase(30); rcase(31); rcase(32); rcase(33); rcase(34); rcase(35); rcase(36); rcase(37); rcase(38); rcase(39);
      rcase(40); rcase(41); rcase(42); rcase(43); rcase(44); rcase(45); rcase(46); rcase(47); rcase(48); rcase(49);

#undef rcase
      default: assert(false && "Error: Hardcode more rcases to allow larger lists.");
    }
  }
private:
   container_t data;
   std::size_t object_count { 0 };

   std::vector<std::size_t> types;
};

