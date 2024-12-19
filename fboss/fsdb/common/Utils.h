// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <string>
#include "fboss/fsdb/common/Types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

class Utils {
 public:
  template <typename... Args>
  static FsdbException createFsdbException(
      FsdbErrorCode errorCode,
      Args... args) {
    const auto message = folly::to<std::string>(args...);
    XLOG(ERR) << "Creating FsdbException with code "
              << apache::thrift::util::enumNameSafe(errorCode) << ": "
              << message;
    FsdbException e;
    e.message() = std::move(message);
    e.errorCode() = errorCode;
    return e;
  }

  template <typename T>
  static bool isPrefixOf(const std::vector<T>& v1, const std::vector<T>& v2) {
    if (v1.size() > v2.size()) {
      return false;
    }
    return v1.end() == std::mismatch(v1.begin(), v1.end(), v2.begin()).first;
  }
};

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
                          thrift_cow::SimpleTraverseHelper& traverser,
                          auto oldNode,
                          auto newNode,
                          thrift_cow::DeltaElemTag /* visitTag */) {
    std::vector<std::string> fullPath;
    fullPath.reserve(basePath.size() + traverser.path().size());
    fullPath.insert(fullPath.end(), basePath.begin(), basePath.end());
    fullPath.insert(
        fullPath.end(), traverser.path().begin(), traverser.path().end());
    // 1. TODO: metadata
    // 2. For each oper delta, hw agent only updates current state with newNode.
    //    Passing in empty oldNode to reduce memory in operDelta.
    decltype(oldNode) emptyOldNode;
    operDeltaUnits.push_back(buildOperDeltaUnit(
        fullPath, emptyOldNode, newNode, fsdb::OperProtocol::BINARY));
  };

  thrift_cow::SimpleTraverseHelper traverser;
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
