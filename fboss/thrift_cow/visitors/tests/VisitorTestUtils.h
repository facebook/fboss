// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <thrift/lib/cpp2/op/Get.h>
#include "fboss/thrift_cow/nodes/tests/gen-cpp2/test_types.h"
#include "fboss/thrift_cow/visitors/PathVisitor.h"

namespace facebook::fboss::thrift_cow {

namespace k = apache::thrift::ident;

// Helper to get field ID as int16_t for a given thrift type and ident
template <typename TType, typename Id>
constexpr int16_t fieldId() {
  return folly::to_underlying(apache::thrift::op::get_field_id_v<TType, Id>);
}

TestStruct createSimpleTestStruct();

template <typename Node, typename Func>
inline ThriftTraverseResult visitPath(
    Node& node,
    pv_detail::PathIter begin,
    pv_detail::PathIter end,
    Func&& f) {
  auto op = pvlambda(std::forward<Func>(f));
  return PathVisitor<typename Node::TC>::visit(
      node, begin, end, PathVisitOptions::visitLeaf(), op);
}

} // namespace facebook::fboss::thrift_cow
