// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <vector>
#include "fboss/agent/platforms/common/fake_test/FakeTestPlatformMapping.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/SlotThreadHelper.h"

namespace facebook::fboss {
const inline std::shared_ptr<
    std::unordered_map<TransceiverID, SlotThreadHelper>>
makeSlotThreadHelper(
    const std::shared_ptr<const FakeTestPlatformMapping> platformMapping) {
  std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
      slotThreadHelper = std::make_shared<
          std::unordered_map<TransceiverID, SlotThreadHelper>>();

  for (const auto& tcvrID :
       utility::getTransceiverIds(platformMapping->getChips())) {
    slotThreadHelper->emplace(tcvrID, SlotThreadHelper(tcvrID));
  }

  return slotThreadHelper;
}

const inline std::shared_ptr<const FakeTestPlatformMapping>
makeFakePlatformMapping(
    int numModules,
    int numPortsPerModule,
    bool multipleTransceiversPerPort = false) {
  std::vector<int> controllingPortIDs(numModules);
  std::generate(
      begin(controllingPortIDs),
      end(controllingPortIDs),
      [n = 1, numPortsPerModule]() mutable {
        int port = n;
        n += numPortsPerModule;
        return port;
      });
  return std::make_shared<FakeTestPlatformMapping>(
      controllingPortIDs, numPortsPerModule, multipleTransceiversPerPort);
}
} // namespace facebook::fboss
