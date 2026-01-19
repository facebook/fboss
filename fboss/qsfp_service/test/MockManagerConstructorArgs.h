// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <vector>
#include "fboss/agent/platforms/common/fake_test/FakeTestPlatformMapping.h"
#include "fboss/qsfp_service/QsfpServiceThreads.h"

namespace facebook::fboss {
const inline std::shared_ptr<QsfpServiceThreads> makeQsfpServiceThreads(
    const std::shared_ptr<const FakeTestPlatformMapping> platformMapping) {
  return createQsfpServiceThreads(platformMapping);
}

const inline std::shared_ptr<const FakeTestPlatformMapping>
makeFakePlatformMapping(
    int numModules,
    int numPortsPerModule,
    FakeTestPlatformMappingType mappingType =
        FakeTestPlatformMappingType::STANDARD) {
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
      controllingPortIDs, numPortsPerModule, mappingType);
}
} // namespace facebook::fboss
