// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook::fboss {

class Wedge400CManager : public WedgeManager {
 public:
  explicit Wedge400CManager(
      const std::shared_ptr<const PlatformMapping> platformMapping,
      const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
          threads);
  ~Wedge400CManager() override {}
  int getNumQsfpModules() const override {
    return 48;
  }

 private:
  // Forbidden copy constructor and assignment operator
  Wedge400CManager(Wedge400CManager const&) = delete;
  Wedge400CManager& operator=(Wedge400CManager const&) = delete;

 protected:
  std::unique_ptr<TransceiverI2CApi> getI2CBus() override;
};

} // namespace facebook::fboss
