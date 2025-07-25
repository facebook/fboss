// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <boost/container/flat_map.hpp>
#include "fboss/lib/bsp/BspSystemContainer.h"
#include "fboss/lib/bsp/BspTransceiverApi.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook {
namespace fboss {

class TransceiverI2CApi;

class BspWedgeManager : public WedgeManager {
 public:
  BspWedgeManager(
      const BspSystemContainer* systemContainer,
      std::unique_ptr<BspTransceiverApi> api,
      const std::shared_ptr<const PlatformMapping> platformMapping,
      PlatformType type,
      const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
          threads);
  ~BspWedgeManager() override {}

  int getNumQsfpModules() const override;

  std::unique_ptr<TransceiverI2CApi> getI2CBus() override;

  // Forbidden copy constructor and assignment operator
  BspWedgeManager(BspWedgeManager const&) = delete;
  BspWedgeManager& operator=(BspWedgeManager const&) = delete;

 private:
  const BspSystemContainer* systemContainer_;
};

} // namespace fboss
} // namespace facebook
