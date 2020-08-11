#pragma once

#include <nlohmann/json.hpp>

#include <source_range.hpp>

struct fixit_info
{
  std::vector<std::pair<untied_source_pos, nlohmann::json>> changes; 
};

void to_json(nlohmann::json& j, const fixit_info& info);
void from_json(const nlohmann::json& j, fixit_info& info);

