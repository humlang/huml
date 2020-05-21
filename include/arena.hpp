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

