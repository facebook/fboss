// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

#include "fboss/qsfp_service/module/tests/MockSffModule.h"

namespace facebook::fboss {

class MockWedgeManager : public WedgeManager {
 public:
  MockWedgeManager() : WedgeManager(nullptr, nullptr, PlatformMode::WEDGE) {}
  void makeTransceiverMap() {
    for (int idx = 0; idx < getNumQsfpModules(); idx++) {
      std::unique_ptr<MockSffModule> qsfp = std::make_unique<MockSffModule>(
          this, nullptr, numPortsPerTransceiver());
      mockTransceivers_.emplace(TransceiverID(idx), qsfp.get());
      transceivers_.wlock()->emplace(TransceiverID(idx), move(qsfp));
    }
  }

  PlatformMode getPlatformMode() override {
    return PlatformMode::WEDGE;
  }

  std::map<TransceiverID, MockSffModule*> mockTransceivers_;
};

} // namespace facebook::fboss
