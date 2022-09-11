// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_storage/CowStorage.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

template <typename NodeT>
OperDeltaUnit buildOperDeltaUnit(
    const std::vector<std::string>& path,
    const NodeT& oldNode,
    const NodeT& newNode,
    OperProtocol protocol) {
  OperDeltaUnit unit;
  unit.path()->raw() = path;
  if (oldNode) {
    unit.oldState() = oldNode->encode(protocol);
  }
  if (newNode) {
    unit.newState() = newNode->encode(protocol);
  }
  return unit;
}

} // namespace facebook::fboss::fsdb
