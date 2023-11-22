// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_visitation.h"

namespace facebook::fboss::fsdb {

void initThread(folly::StringPiece name);

// TODO: add tests to check whether it works as expected
template <class T>
bool ThriftVisitationEquals(const T& obj1, const T& obj2) {
  if constexpr (apache::thrift::is_thrift_class_v<T>) {
    auto res = true;
    apache::thrift::for_each_field(
        obj1, obj2, [&](const auto& /* meta */, auto&& ref1, auto&& ref2) {
          if (!res || ref1.has_value() != ref2.has_value() ||
              !ThriftVisitationEquals(ref1, ref2)) {
            res = false;
          }
        });
    return res;
  } else {
    // TODO: when does this occur ? For a map, as in the comment in D26488063
    // ?
  }
  return true;
}

} // namespace facebook::fboss::fsdb
