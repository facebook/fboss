// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <unordered_map>
#include "fboss/lib/bsp/BspPimContainer.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspSystemContainer {
 public:
  explicit BspSystemContainer(BspPlatformMapping* bspMapping);

  const BspPimContainer* getPimContainerFromPimID(int pimID) const;
  const BspPimContainer* getPimContainerFromTcvrID(int tcvrID) const;
  int getNumTransceivers() const;
  int getPimIDFromTcvrID(int tcvrID) const;
  int getNumPims() const;

 private:
  std::unordered_map<int, std::unique_ptr<BspPimContainer>> pimContainers_;
  BspPlatformMapping* bspMapping_;
};

} // namespace fboss
} // namespace facebook
