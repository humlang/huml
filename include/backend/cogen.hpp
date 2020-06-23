#pragma once

#include <string>

namespace ir
{
struct Node;
struct builder;

bool cogen(std::string name, const Node* ref);
}


