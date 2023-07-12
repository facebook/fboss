// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/Wedge400I2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook {
namespace fboss {

class Wedge400CManager : public WedgeManager {
 public:
  Wedge400CManager();
  ~Wedge400CManager() override {}
  int getNumQsfpModules() override {
    return 48;
  }

 private:
  // Forbidden copy constructor and assignment operator
  Wedge400CManager(Wedge400CManager const&) = delete;
  Wedge400CManager& operator=(Wedge400CManager const&) = delete;

  std::unique_ptr<PlatformMapping> createWedge400CPlatformMapping();

 protected:
  std::unique_ptr<TransceiverI2CApi> getI2CBus() override;
};

} // namespace fboss
} // namespace facebook
