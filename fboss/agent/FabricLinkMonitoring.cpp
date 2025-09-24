// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricLinkMonitoring.h"

#include "fboss/agent/Utils.h"
#include "fboss/agent/VoqUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

// Constructor initializes the fabric link monitoring system by processing
// DSF nodes and link information from the switch configuration
FabricLinkMonitoring::FabricLinkMonitoring(const cfg::SwitchConfig* config)
    : isVoqSwitch_(
          *config->switchSettings()->switchType() == cfg::SwitchType::VOQ) {
  processDsfNodes(config);
  processLinkInfo(config);
}

// Returns mapping from port IDs to their corresponding switch IDs
// TODO: Implement the actual switch ID mapping logic based on processed data
std::map<PortID, SwitchID> FabricLinkMonitoring::getPort2SwitchIdMapping() {
  // TODO: Implement the actual switch ID mapping logic
  return std::map<PortID, SwitchID>();
}

void FabricLinkMonitoring::processDsfNodes(const cfg::SwitchConfig* config) {
  switchName2SwitchId_.clear();
  for (const auto& [_, dsfNode] : *config->dsfNodes()) {
    auto fabricLevel = dsfNode.fabricLevel().value_or(0);
    auto nodeSwitchId = SwitchID(*dsfNode.switchId());

    updateSwitchNameMapping(dsfNode, nodeSwitchId);
    updateLowestSwitchIds(dsfNode, nodeSwitchId, fabricLevel);
  }
}

// Populate the switch name to switch ID mapping. If multiple names map to the
// same switchID, cache just the lowest switch ID available.
void FabricLinkMonitoring::updateSwitchNameMapping(
    const auto& dsfNode,
    const SwitchID& nodeSwitchId) {
  const auto& nodeName = *dsfNode.name();
  auto [it, inserted] =
      switchName2SwitchId_.try_emplace(nodeName, nodeSwitchId);
  if (!inserted && nodeSwitchId < it->second) {
    it->second = nodeSwitchId;
  }
}

// Keep track of the lowest switch IDs for leaf / L1 and L2, which is needed to
// find the offset of switch ID of a specific layer.
void FabricLinkMonitoring::updateLowestSwitchIds(
    const auto& dsfNode,
    const SwitchID& nodeSwitchId,
    const int fabricLevel) {
  if (*dsfNode.type() == cfg::DsfNodeType::INTERFACE_NODE) {
    lowestLeafSwitchId_ = std::min(nodeSwitchId, lowestLeafSwitchId_);
  } else {
    if (fabricLevel == 1) {
      lowestL1SwitchId_ = std::min(nodeSwitchId, lowestL1SwitchId_);
    } else if (fabricLevel == 2) {
      lowestL2SwitchId_ = std::min(nodeSwitchId, lowestL2SwitchId_);
    } else {
      throw FbossError(
          "DSF node should be one of interface node, l1 or l2 fabric switch!");
    }
  }
}

void FabricLinkMonitoring::processLinkInfo(
    const cfg::SwitchConfig* /*config*/) {
  // Stub - will be implemented in later diff, but add validation call
  validateLinkLimits();
}

// Keep track of the number of links between leaf and L1 and
// also between L1 and L2 in the network.
void FabricLinkMonitoring::updateLinkCounts(
    const cfg::SwitchConfig* config,
    const SwitchID& neighborSwitchId) {
  if (isVoqSwitch_ || isConnectedToVoqSwitch(config, neighborSwitchId)) {
    ++numLeafToL1Links_;
  } else {
    ++numL1ToL2Links_;
  }
}

// Switch IDs for links are limited and hence need to ensure that the
// num links between leaf-L1 and L1-L2 are within the expected bounds.
void FabricLinkMonitoring::validateLinkLimits() const {
  if (numLeafToL1Links_ > kFabricLinkMonitoringMaxLeafSwitchIds) {
    throw FbossError(
        "Too many leaf to L1 links, max expected ",
        kFabricLinkMonitoringMaxLeafSwitchIds);
  }
  if (numL1ToL2Links_ > kFabricLinkMonitoringMaxLevel2SwitchIds) {
    throw FbossError(
        "Too many L1 to L2 links, max expected ",
        kFabricLinkMonitoringMaxLevel2SwitchIds);
  }
}

} // namespace facebook::fboss
