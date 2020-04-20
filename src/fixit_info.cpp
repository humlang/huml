#include <fixit_info.hpp>
#include <source_range.hpp>

void to_json(nlohmann::json& j, const fixit_info& info)
{
  j = info.changes;
}

void from_json(const nlohmann::json& j, fixit_info& info)
{
  info.changes = j.get<std::vector<std::pair<untied_source_pos, nlohmann::json>>>();
}

