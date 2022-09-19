// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <memory>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/BspTransceiverContainer.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspPimContainer {
 public:
  explicit BspPimContainer(BspPimMapping& bspPimMapping);
  const BspTransceiverContainer* getTransceiverContainer(int tcvrID) const;

  bool isTcvrPresent(int tcvrID) const;
  void initAllTransceivers() const;
  void clearAllTransceiverReset() const;
  void triggerTcvrHardReset(int tcvrID) const;

 private:
  std::unordered_map<int, std::unique_ptr<BspTransceiverContainer>>
      tcvrContainers_;
  BspPimMapping bspPimMapping_;
};

} // namespace fboss
} // namespace facebook
