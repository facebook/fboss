// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook::fboss {

class MockTransceiverI2CApi : public TransceiverI2CApi {
 public:
  MockTransceiverI2CApi() {}
  ~MockTransceiverI2CApi() override {}
  MOCK_METHOD0(open, void());
  MOCK_METHOD0(close, void());
  MOCK_METHOD1(verifyBus, void(bool));
  bool isPresent(unsigned int id) override;
  void moduleRead(
      unsigned int module,
      uint8_t i2cAddress,
      int offset,
      int len,
      uint8_t* buf) override;

  MOCK_METHOD5(
      moduleWrite,
      void(unsigned int, uint8_t, int, int, const uint8_t*));

  void scanPresence(std::map<int32_t, ModulePresence>&) override;

  void overridePresence(unsigned int id, bool presence);
  void overrideMgmtInterface(unsigned int id, uint8_t mgmt);
  bool throwReadExceptionForMgmtInterface_{false};
  bool throwReadExceptionForDomQuery_{false};

 private:
  std::map<unsigned int, bool> overridenPresence_;
  std::map<unsigned int, uint8_t> overridenMgmtInterface_;
};

} // namespace facebook::fboss
