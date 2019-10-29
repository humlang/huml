#pragma once

#include <algorithm>
#include <sstream>
#include <memory>
#include <vector>
#include <map>

struct CmdOptions
{
  template<typename T>
  struct TaggedValue;

  class Value : public std::enable_shared_from_this<Value>
  {
  public:
    using Ptr = std::shared_ptr<Value>;

    virtual Ptr clone(std::string def_val) = 0;
    virtual void parse(int& i, int argc, const char** argv) = 0;
    virtual void parse_default(std::string default_val) = 0;

    template<typename T>
    T& get();

    std::string default_value;
  };
  template<typename T>
  struct TaggedValue : Value
  {
    using Ptr = std::shared_ptr<TaggedValue<T>>;

    static Value::Ptr create()
    { return std::make_shared<TaggedValue<T>>(); }

    Value::Ptr clone(std::string def_val) override
    { parse_default(def_val); return std::make_shared<TaggedValue<T>>(*this); }

    void parse(int& i, int argc, const char** argv) override;
    void parse_default(std::string default_val) override;

    T val;    
  };
  struct CmdOptionsAdder
  {
    CmdOptionsAdder(CmdOptions& ref);

    CmdOptionsAdder& operator()(std::string option, std::string description,
                                Value::Ptr value = std::make_shared<TaggedValue<bool>>(),
                                std::string default_value = "false");
  private:
    CmdOptions& ref;
  };
  struct Option
  {
    std::vector<std::string> long_names;
    std::vector<std::string> short_names;

    std::string description;
    
    Value::Ptr value;
  };
public:
  using ValueMapping = std::map<std::string, Value::Ptr>;

  CmdOptions(std::string name, std::string description);

  CmdOptionsAdder add_options();
  void add(std::string option, std::string description, Value::Ptr value = std::make_shared<TaggedValue<bool>>(),
                                                        std::string default_value = "false");
  ValueMapping parse(int argc, const char** argv);

  void print_help();
  void handle_unrecognized_option(std::string opt);
private:
  std::string name;
  std::string description;

  std::vector<Option> opts;
};

template<typename T>
void CmdOptions::TaggedValue<T>::parse(int& i, int argc, const char** argv)
{
  if constexpr(std::is_same<T, std::vector<std::string>>::value)
  {
    ++i;
    while(i < argc)
    {
      if(argv[i][0] == '-')
      {
        if(argv[i][1] == '-' && argv[i][2] == '\0')
        {
          val.push_back(argv[i++]);
          continue;
        }
        else
        {
          // another command line argument, exit earlier
          break;
        }
      }

      val.push_back(argv[i++]);
    }
  }
  else if constexpr(std::is_same<T, std::string>::value)
  {
    if(argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == '\0')
    {
      val = "--";
      i++;
    }
    else
    {
      val = argv[++i];
      ++i;
    } 
  }
  else if constexpr(std::is_same<T, bool>::value)
  {
    // TODO: check for overrides, e.g. "--transmogrify yes"

    constexpr const char* truth[]   = { "yes", "true", "t", "y" };
    constexpr const char* falsity[] = { "no", "false", "n", "f" };

    auto cpy = default_value;
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), [](unsigned char c) { return std::tolower(c); });

    if(std::find(std::begin(truth), std::end(truth), cpy) != std::end(truth))
    {
      //default is true, so we set val to false
      val = false;
    }
    else if(std::find(std::begin(falsity), std::end(falsity), cpy) != std::end(falsity))
    {
      //default is false, so we set val to true
      val = true;
    }
    ++i;
  }
  else
  {
    ++i;
    std::stringstream ss(argv[i++]);
    ss >> val;
  }
}

template<typename T>
void CmdOptions::TaggedValue<T>::parse_default(std::string def_val)
{
  if constexpr(std::is_same<T, std::vector<std::string>>::value)
  {
    // TODO
  }
  else if constexpr(std::is_same<T, bool>::value)
  {
    // TODO: check for overrides, e.g. "--transmogrify yes"

    constexpr const char* truth[]   = { "yes", "true", "t", "y" };
    constexpr const char* falsity[] = { "no", "false", "n", "f" };

    auto cpy = default_value;
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), [](unsigned char c) { return std::tolower(c); });

    if(std::find(std::begin(truth), std::end(truth), cpy) != std::end(truth))
      val = true;
    else if(std::find(std::begin(falsity), std::end(falsity), cpy) != std::end(falsity))
      val = false;
  }
  else
  {
    std::stringstream ss(def_val);
    ss >> val;
  }
}

template<typename T>
T& CmdOptions::Value::get()
{ return static_cast<TaggedValue<T>&>(*this).val; }


