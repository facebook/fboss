/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_multinode_tests/TopologyInfo.h"

#include "common/network/NetworkUtil.h"
#include "fboss/agent/test/agent_multinode_tests/DsfTopologyInfo.h"

namespace {

using facebook::fboss::FbossError;
using facebook::fboss::SwitchState;
using facebook::fboss::utility::TopologyInfo;

TopologyInfo::TopologyType getDerivedTopologyType(
    const std::shared_ptr<SwitchState>& switchState) {
  if (switchState->getDsfNodes()->size() > 0) {
    return TopologyInfo::TopologyType::DSF;
  }

  throw FbossError("Unsupported topology type");
}

} // namespace

namespace facebook::fboss::utility {

std::unique_ptr<TopologyInfo> TopologyInfo::makeTopologyInfo(
    const std::shared_ptr<SwitchState>& switchState) {
  auto topologyType = getDerivedTopologyType(switchState);

  switch (topologyType) {
    case TopologyType::DSF:
      return std::make_unique<DsfTopologyInfo>(switchState);
  }

  throw FbossError("Unexcepted topologyInfo: ", topologyType);
}

TopologyInfo::TopologyInfo(const std::shared_ptr<SwitchState>& switchState)
    : topologyType_(getDerivedTopologyType(switchState)) {
  myHostname_ = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
}

TopologyInfo::~TopologyInfo() {}

bool TopologyInfo::isTestDriver(const SwSwitch& sw) const {
  // Multi Node Tests follow this model:
  //  o One node (SwitchID 0 by convention) is the Test Driver.
  //  o Every other node runs a Test wedge_agent.
  //
  // The Test Driver executes this binary and may make Thrift calls to other
  // nodes for specific programming.
  // All the other nodes run wedge_agent_test / fboss_sw_agent_test. These
  // agents are same as production wedge_agent / fboss_sw_agent but with one
  // difference:
  //  o Production binaries link with if/ctrl.thrift.
  //  o Test binaries link with if/test_ctrl.thrift.
  //  o test_ctrl.thrift inherits from ctrl.thrift.
  //  o test_ctrl.thrift provides additional functionality including ability
  //    to perform disruptive operations that we cannot support in the
  //    roduction e.g. cold booting binaries.
  auto constexpr kTestDriverSwitchId = 0;

  bool ret = sw.getSwitchInfoTable().getSwitchIDs().contains(
      SwitchID(kTestDriverSwitchId));

  XLOG(DBG2) << "Multi Node Test Driver node: SwitchID " << kTestDriverSwitchId
             << " is " << (ret ? " part of " : " not part of")
             << " local switchIDs : "
             << folly::join(",", sw.getSwitchInfoTable().getSwitchIDs());
  return ret;
}

} // namespace facebook::fboss::utility
