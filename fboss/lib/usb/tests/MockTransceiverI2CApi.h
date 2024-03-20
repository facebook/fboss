// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"

#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook::fboss {

class TransceiverManager;

class MockTransceiverI2CApi : public TransceiverI2CApi {
 public:
  MockTransceiverI2CApi() = default;
  ~MockTransceiverI2CApi() override = default;
  MOCK_METHOD0(open, void());
  MOCK_METHOD0(close, void());
  MOCK_METHOD1(verifyBus, void(bool));
  bool isPresent(unsigned int id) override;
  void moduleRead(
      unsigned int module,
      const TransceiverAccessParameter& param,
      uint8_t* buf) override;

  MOCK_METHOD3(
      moduleWrite,
      void(unsigned int, const TransceiverAccessParameter&, const uint8_t*));

  void scanPresence(std::map<int32_t, ModulePresence>&) override;

  void overridePresence(unsigned int id, bool presence);
  void
  overrideMgmtInterface(unsigned int id, uint8_t mgmt, TransceiverManager* mgr);
  bool throwReadExceptionForMgmtInterface_{false};
  bool throwReadExceptionForDomQuery_{false};

 private:
  std::map<unsigned int, bool> overridenPresence_;
  std::map<unsigned int, uint8_t> overridenMgmtInterface_;
  std::map<unsigned int, std::unique_ptr<FakeTransceiverImpl>>
      overridenTransceivers_;
};

} // namespace facebook::fboss
