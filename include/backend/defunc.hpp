#pragma once

namespace ir
{
struct Node;
struct builder;

// / / TODO: What to do with returned higher order functions?
//             -> Still just pass them to a big switch....??

// This pass converts anything reachable from the given node to a first-order IR.
// It assumes that all call-sites are known.
// Because, the way this converts higher-order functions to first-order is as follows:
//   1) Look at all call sites of a higher-order function
//      and collect higher-order parameters
//   2) Replace the higher-order parameter with a simple integer
//      id that shall represent the passed function.
//   3) Repeat until we've seen all call sites.
//   4) Finally, replace the higher-order function with a version where
//      all it's function-parameters are just integers.
//      Convert its body to one big switch where we call the corresponding
//      function for the given integer.

const Node* defunctionalize(builder& b, const Node* n);

}

