#include <symbol.hpp>
#include <tmp.hpp>

thread_local std::unordered_map<std::uint_fast32_t, std::string> symbol::symbols = {};

symbol::symbol(const std::string& str)
  : hash(hash_string(str))
{ lookup_or_emplace(hash, str.c_str()); }

symbol::symbol(const char* str)
  : hash(hash_string(str))
{ lookup_or_emplace(hash, str); }

symbol::symbol(const symbol& s)
  : hash(s.hash)
{  }

symbol::symbol(symbol&& s)
  : hash(s.hash)
{  }

symbol::~symbol() noexcept
{  }

symbol& symbol::operator=(const std::string& str)
{
  this->hash = hash_string(str);
  lookup_or_emplace(this->hash, str.c_str());

  return *this;
}

symbol& symbol::operator=(const char* str)
{
  this->hash = hash_string(str);
  lookup_or_emplace(this->hash, str);

  return *this;
}

symbol& symbol::operator=(const symbol& s)
{
  this->hash = s.hash;

  return *this;
}

symbol& symbol::operator=(symbol&& s)
{
  this->hash = s.hash;

  return *this;
}

std::ostream& operator<<(std::ostream& os, const symbol& symb)
{
  os << symbol::symbols[symb.hash];
  return os;
}

std::string& symbol::lookup_or_emplace(std::uint_fast32_t hash, const char* str)
{
  auto it = symbols.find(hash);
  if(it != symbols.end())
    return symbols[hash];

  symbols[hash] = str;
  return symbols[hash];
}

std::ostream& operator<<(std::ostream& os, const std::vector<symbol>& symbs)
{
  for(auto& s : symbs)
    os << s;
  return os;
}

std::uint_fast32_t symbol::get_hash() const
{ return hash; }

const std::string& symbol::get_string() const
{ return symbols[hash]; }

bool operator==(const symbol& a, const symbol& b)
{
  return a.get_hash() == b.get_hash();
}

bool operator!=(const symbol& a, const symbol& b)
{
  return a.get_hash() != b.get_hash();
}


