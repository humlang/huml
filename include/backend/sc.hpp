#pragma once

namespace ir
{
struct Node;
struct builder;

bool supercompile(builder& b, const Node* ref);
}


