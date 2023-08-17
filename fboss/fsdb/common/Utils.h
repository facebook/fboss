// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
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

OperDelta createDelta(std::vector<OperDeltaUnit>&& deltaUnits);

template <typename Node>
fsdb::OperDelta computeOperDelta(
    const std::shared_ptr<Node>& oldNode,
    const std::shared_ptr<Node>& newNode,
    const std::vector<std::string>& basePath,
    bool outputIdPaths = false) {
  std::vector<fsdb::OperDeltaUnit> operDeltaUnits{};

  auto processDelta = [basePath, &operDeltaUnits](
                          const std::vector<std::string>& path,
                          auto oldNode,
                          auto newNode,
                          thrift_cow::DeltaElemTag /* visitTag */) {
    std::vector<std::string> fullPath;
    fullPath.reserve(basePath.size() + path.size());
    fullPath.insert(fullPath.end(), basePath.begin(), basePath.end());
    fullPath.insert(fullPath.end(), path.begin(), path.end());
    // 1. TODO: metadata
    // 2. For each oper delta, hw agent only updates current state with newNode.
    //    Passing in empty oldNode to reduce memory in operDelta.
    decltype(oldNode) emptyOldNode;
    operDeltaUnits.push_back(buildOperDeltaUnit(
        fullPath, emptyOldNode, newNode, fsdb::OperProtocol::BINARY));
  };

  thrift_cow::RootDeltaVisitor::visit(
      oldNode,
      newNode,
      thrift_cow::DeltaVisitOptions(
          thrift_cow::DeltaVisitMode::MINIMAL,
          thrift_cow::DeltaVisitOrder::PARENTS_FIRST,
          outputIdPaths),
      std::move(processDelta));
  return createDelta(std::move(operDeltaUnits));
}

} // namespace facebook::fboss::fsdb
