// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include "fboss/fsdb/common/Utils.h"
#include "fboss/thrift_cow/visitors/DeltaVisitor.h"

namespace facebook::fboss {

template <typename Node>
fsdb::OperDelta computeOperDelta(
    const std::shared_ptr<Node>& oldNode,
    const std::shared_ptr<Node>& newNode,
    const std::vector<std::string>& basePath) {
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
    // TODO: metadata
    operDeltaUnits.push_back(buildOperDeltaUnit(
        fullPath, oldNode, newNode, fsdb::OperProtocol::BINARY));
  };

  thrift_cow::RootDeltaVisitor::visit(
      oldNode,
      newNode,
      thrift_cow::DeltaVisitMode::MINIMAL,
      std::move(processDelta));

  fsdb::OperDelta operDelta{};
  operDelta.changes() = operDeltaUnits;
  operDelta.protocol() = fsdb::OperProtocol::BINARY;
  return operDelta;
}

std::vector<std::string> fsdbAgentDataSwitchStateRootPath();
std::vector<std::string> switchStateRootPath();
} // namespace facebook::fboss
