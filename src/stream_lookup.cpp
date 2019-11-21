#include <stream_lookup.hpp>

#include <filesystem>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <chrono>

namespace fs = std::filesystem;

constexpr static auto cur_time = [](){
      auto now = std::chrono::system_clock::now();
      auto in_time_t = std::chrono::system_clock::to_time_t(now);

      std::stringstream ss;
      ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
      return ss.str();
    };

stream_lookup_t::stream_lookup_t()
  : files(), map(),
  stdin_module((fs::temp_directory_path() / ("STDIN_" + cur_time())).string())
#ifdef H_LANG_TESTING
  ,test_module((fs::temp_directory_path() / ("TEST_" + cur_time())).string())
#endif
{
}

stream_lookup_t::~stream_lookup_t()
{
  assert(map.empty() && "All streams must be dropped.");
}

void stream_lookup_t::process_stdin()
{
#ifndef H_LANG_TESTING
  std::ofstream output_writer(stdin_module);
  output_writer << std::cin.rdbuf();
  stdin_processed = true;
#endif
}

std::istream& stream_lookup_t::operator[](std::string_view str)
{
  if(str == "STDIN")
  {
    if(!stdin_processed)
      process_stdin();
    str = stdin_module;
  }
#ifdef H_LANG_TESTING
  else if(str == "TESTSTREAM")
  {
    str = test_module;
  }
#endif
  auto it = map.find(str);
  assert(it == map.end() && "Stream should have been dropped.");

  files.push_back(std::move(std::make_unique<std::ifstream>(str.data())));

  map[str] = files.end() - 1;

  return *files.back();
}

void stream_lookup_t::drop(std::string_view str)
{
  if(str == "STDIN")
  {
    str = stdin_module;
  }
#ifdef H_LANG_TESTING
  else if(str == "TESTSTREAM")
  {
    str = test_module;
  }
#endif
  auto it = map.find(str);

  assert(it != map.end() && "Stream should exist.");

  files.erase(it->second);
  map.erase(it);
}

#ifdef H_LANG_TESTING
void stream_lookup_t::write_test(std::string_view str)
{
  std::ofstream of(test_module);
  of << str;
}
#endif
