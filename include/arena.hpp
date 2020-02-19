#pragma once

#include <vector>

struct arena
{
  arena() : capacity(5000) // alloc 5kB
          , buffer(capacity)
  {  }

  template<typename T, typename... Args>
  T& alloc(Args&&... args);

  template<typename T>
  void destruct();

private:
  std::size_t capacity;
  std::vector<unsigned char> buffer;
};

