#pragma once

namespace ir
{
struct Node;
struct builder;

bool partially_evaluate(builder& b, const Node* ref);
}


