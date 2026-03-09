// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"

namespace facebook::fboss::fsdb::test {

struct SwitchStateScale {
  int fibV4Size{0};
  int fibV6Size{0};
  int v4Nexthops{0};
  int v6Nexthops{0};
  int remoteSystemPortMapSize{0};
  int remoteInterfaceMapSize{0};

  // NEW fields from sample data analysis
  int portCount{0};
  int vlanCount{0};
  int transceiverCount{0};
  int interfaceCount{0};
  int systemPortCount{0};
  int dsfNodeCount{0};
  int aclCount{0};
  int bufferPoolCfgCount{0};
  int mirrorCount{0};
  int qosPolicyCount{0};
  int loadBalancerCount{0};
  int ipTunnelCount{0};
  int aggregatePortCount{0};
  int portFlowletCfgCount{0};
  int mirrorOnDropReportCount{0};
  bool hasControlPlane{true};
  bool hasSwitchSettings{true};
  bool hasAclTableGroup{true};
};

// Builder methods — populate SwitchState fields based on scale
void populatePorts(state::SwitchState& state, const SwitchStateScale& scale);
void populateVlans(state::SwitchState& state, const SwitchStateScale& scale);
void populateInterfaces(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateTransceivers(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateSystemPorts(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateDsfNodes(state::SwitchState& state, const SwitchStateScale& scale);
void populateControlPlane(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateSwitchSettings(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateBufferPoolCfg(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateMirrors(state::SwitchState& state, const SwitchStateScale& scale);
void populateQosPolicies(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateLoadBalancers(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateAclTableGroups(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateAcls(state::SwitchState& state, const SwitchStateScale& scale);
void populateIpTunnels(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateAggregatePorts(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populatePortFlowletCfg(
    state::SwitchState& state,
    const SwitchStateScale& scale);
void populateMirrorOnDropReports(
    state::SwitchState& state,
    const SwitchStateScale& scale);

} // namespace facebook::fboss::fsdb::test
