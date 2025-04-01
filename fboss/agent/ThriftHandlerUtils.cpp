/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ThriftHandlerUtils.h"

namespace facebook::fboss::utility {

std::string getCurrentStateJSONForPathHelper(
    const std::string& path,
    const std::shared_ptr<SwitchState>& sw) {
  std::vector<std::string> thriftPath;
  folly::split('/', path, thriftPath);
  thrift_cow::GetEncodedPathVisitorOperator op(fsdb::OperProtocol::SIMPLE_JSON);
  auto traverseResult = thrift_cow::RootPathVisitor::visit(
      *std::const_pointer_cast<const SwitchState>(sw),
      thriftPath.begin(),
      thriftPath.end(),
      thrift_cow::PathVisitMode::LEAF,
      op);
  switch (traverseResult) {
    case thrift_cow::ThriftTraverseResult::OK:
      return op.val->toStdString();
    case thrift_cow::ThriftTraverseResult::VISITOR_EXCEPTION:
      throw FbossError("Visitor exception when traversing thrift path.");
    default:
      throw FbossError("Invalid thrift path provided.");
  }
}
void clearSwPortStats(
    std::vector<int32_t>& ports,
    std::shared_ptr<SwitchState> state) {
  auto getPortLinkStateCounterKey = [&](std::vector<std::string>& portKeys,
                                        const std::shared_ptr<Port> port) {
    auto portId = port->getID();
    auto portName = port->getName().empty()
        ? folly::to<std::string>("port", portId)
        : port->getName();
    portKeys.emplace_back(
        folly::to<std::string>(portName, ".", "link_state.flap"));
  };
  auto getLinkStateCounterKey = [&](std::vector<std::string>& globalKeys) {
    globalKeys.emplace_back("link_state.flap");
  };

  auto statsMap = facebook::fb303::fbData->getStatMap();
  for (const auto& portId : ports) {
    const auto port = state->getPorts()->getNodeIf(PortID(portId));
    std::vector<std::string> portKeys;
    getPortLinkStateCounterKey(portKeys, port);
    for (const auto& key : portKeys) {
      // this API locks statistics for the key
      // ensuring no race condition with update/delete
      // in different thread
      statsMap->clearValue(key);
    }
  }
  std::vector<std::string> globalKeys;
  getLinkStateCounterKey(globalKeys);
  for (const auto& key : globalKeys) {
    // this API locks statistics for the key
    // ensuring no race condition with update/delete
    // in different thread
    statsMap->clearValue(key);
  }
}
} // namespace facebook::fboss::utility
