#pragma once

#include <nlohmann/json.hpp>

#include <source_range.hpp>
#include <map>

struct fixit_info
{
  std::map<untied_source_pos, std::string> changes; 
};

void to_json(nlohmann::json& j, const fixit_info& info);
void from_json(const nlohmann::json& j, fixit_info& info);

