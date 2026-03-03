/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_multinode_tests/DsfTopologyInfo.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss::utility {

void DsfTopologyInfo::populateDsfNodeInfo(
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap) {
  std::optional<int64_t> prevInterfaceNodeSwitchId;
  std::optional<int> minEdswSwitchId;

  for (const auto& [_, dsfNodes] : std::as_const(*dsfNodeMap)) {
    for (const auto& [key, node] : std::as_const(*dsfNodes)) {
      CHECK_EQ(SwitchID(key), node->getSwitchId())
          << "DsfNode map key must equal switchId";

      switchIdToSwitchName_[node->getSwitchId()] = node->getName();
      switchNameToSwitchIds_[node->getName()].insert(node->getSwitchId());
      switchNameToAsicType_[node->getName()] = node->getAsicType();
      switchNameToSystemPortRanges_[node->getName()] =
          node->getSystemPortRanges();

      if (node->getType() == cfg::DsfNodeType::INTERFACE_NODE) {
        auto& hwAsic = getHwAsicForAsicType(node->getAsicType());
        int numCores = hwAsic.getNumCores();

        if (prevInterfaceNodeSwitchId.has_value()) {
          int64_t step =
              node->getSwitchId() - prevInterfaceNodeSwitchId.value();
          CHECK_EQ(step, numCores)
              << "INTERFACE_NODE switchIds must be contiguous with step "
              << numCores << ", but got step " << step;
        }
        prevInterfaceNodeSwitchId = node->getSwitchId();

        CHECK(node->getClusterId().has_value());
        auto clusterId = node->getClusterId().value();
        if (!minEdswSwitchId.has_value()) {
          minEdswSwitchId = getMaxRdsw(node->getPlatformType()) * numCores;
        }
        if (node->getSwitchId() < minEdswSwitchId.value()) {
          clusterIdToRdsws_[clusterId].push_back(node->getName());
        } else {
          clusterIdToEdsws_[clusterId].push_back(node->getName());
        }
      } else if (node->getType() == cfg::DsfNodeType::FABRIC_NODE) {
        CHECK(node->getFabricLevel().has_value());
        if (node->getFabricLevel().value() == 1) {
          CHECK(node->getClusterId().has_value());
          auto clusterId = node->getClusterId().value();
          clusterIdToFdsws_[clusterId].push_back(node->getName());
        } else if (node->getFabricLevel().value() == 2) {
          sdsws_.insert(node->getName());
        } else {
          XLOG(FATAL) << "Invalid fabric level"
                      << node->getFabricLevel().value();
        }
      } else {
        XLOG(FATAL) << "Invalid DSF Node type"
                    << apache::thrift::util::enumNameSafe(node->getType());
      }
    }
  }
}

void DsfTopologyInfo::populateAllRdsws() {
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : rdsws) {
      allRdsws_.insert(rdsw);
    }
  }
}

void DsfTopologyInfo::populateAllEdsws() {
  for (const auto& [clusterId, edsws] : std::as_const(clusterIdToEdsws_)) {
    for (const auto& edsw : edsws) {
      allEdsws_.insert(edsw);
    }
  }
}

void DsfTopologyInfo::populateAllFdsws() {
  for (const auto& [clusterId, fdsws] : std::as_const(clusterIdToFdsws_)) {
    for (const auto& fdsw : fdsws) {
      allFdsws_.insert(fdsw);
    }
  }
}

void DsfTopologyInfo::populateAllSwitches() {
  allSwitches_.insert(allRdsws_.begin(), allRdsws_.end());
  allSwitches_.insert(allFdsws_.begin(), allFdsws_.end());
  allSwitches_.insert(allEdsws_.begin(), allEdsws_.end());
  allSwitches_.insert(sdsws_.begin(), sdsws_.end());
}

} // namespace facebook::fboss::utility
