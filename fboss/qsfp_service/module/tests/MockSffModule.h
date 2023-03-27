/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/Memory.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"
#include "fboss/qsfp_service/module/sff/SffFieldInfo.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

constexpr auto kIntelModulePN = "SPTSBP2CLCKS";

}

namespace facebook {
namespace fboss {

class MockSffModule : public SffModule {
 public:
  explicit MockSffModule(
      TransceiverManager* transceiverManager,
      std::unique_ptr<TransceiverImpl> qsfpImpl)
      : SffModule(transceiverManager, std::move(qsfpImpl)) {
    ON_CALL(*this, updateQsfpData(testing::_))
        .WillByDefault(testing::Assign(&dirty_, false));
    ON_CALL(*this, ensureTransceiverReadyLocked())
        .WillByDefault(testing::Return(true));
  }
  MOCK_METHOD1(setPowerOverrideIfSupportedLocked, void(PowerControlState));
  MOCK_METHOD1(updateQsfpData, void(bool));
  MOCK_CONST_METHOD2(getSettingsValue, uint8_t(SffField, uint8_t));
  MOCK_METHOD0(getTransceiverInfo, TransceiverInfo());

  MOCK_METHOD3(
      setCdrIfSupported,
      void(cfg::PortSpeed, FeatureState, FeatureState));
  MOCK_METHOD3(
      setRateSelectIfSupported,
      void(cfg::PortSpeed, RateSelectState, RateSelectSetting));

  MOCK_CONST_METHOD0(getQsfpTransmitterTechnology, TransmitterTechnology());

  MOCK_METHOD0(ensureTxEnabled, void());
  MOCK_METHOD0(resetLowPowerMode, void());
  MOCK_METHOD0(ensureTransceiverReadyLocked, bool());
  MOCK_CONST_METHOD0(customizationSupported, bool());

  // Provide way to call parent
  void actualSetCdrIfSupported(
      cfg::PortSpeed speed,
      FeatureState tx,
      FeatureState rx) {
    SffModule::setCdrIfSupported(speed, tx, rx);
  }
  void actualSetRateSelectIfSupported(
      cfg::PortSpeed speed,
      RateSelectState currentState,
      RateSelectSetting currentSetting) {
    SffModule::setRateSelectIfSupported(speed, currentState, currentSetting);
  }
  void actualUpdateQsfpData(bool full) {
    present_ = true;
    SffModule::updateQsfpData(full);
  }

  void useActualGetTransceiverInfo() {
    ON_CALL(*this, getTransceiverInfo())
        .WillByDefault(testing::Return(SffModule::getTransceiverInfo()));
  }

  void setFlatMem() {
    flatMem_ = false;
  }

  void customizeTransceiver(TransceiverPortState& portState) override {
    dirty_ = false;
    present_ = true;
    SffModule::customizeTransceiver(portState);
  }

  TransceiverSettings getTransceiverSettingsInfo() override {
    TransceiverSettings settings = TransceiverSettings();
    settings.cdrTx() = cdrTx_;
    settings.cdrRx() = cdrRx_;
    settings.rateSelect() = state_;
    settings.rateSelectSetting() = setting_;
    return settings;
  }

  void setVendorPN(std::string vendorPN = kIntelModulePN) {
    TransceiverInfo info;
    Vendor vendor = Vendor();
    // shouldRemediate will check the vendor PN to skip doing it on Miniphoton
    // modules. Here we take a PN other than Miniphoton.
    vendor.partNumber() = vendorPN;
    info.vendor() = vendor;
    fakeInfo_ = info;
  }

  void setRateSelect(RateSelectState state, RateSelectSetting setting) {
    state_ = state;
    setting_ = setting;
  }

  void setCdrState(FeatureState tx, FeatureState rx) {
    cdrTx_ = tx;
    cdrRx_ = rx;
  }
  RateSelectSetting getRateSelectSettingValue(RateSelectState state) override {
    return SffModule::getRateSelectSettingValue(state);
  }

  std::unique_ptr<IOBuf> readTransceiver(
      TransceiverIOParameters param) override {
    return SffModule::readTransceiver(param);
  }

  bool writeTransceiver(TransceiverIOParameters param, uint8_t data) override {
    return SffModule::writeTransceiver(param, data);
  }

  TransceiverInfo fakeInfo_;

 private:
  FeatureState cdrTx_ = FeatureState::UNSUPPORTED;
  FeatureState cdrRx_ = FeatureState::UNSUPPORTED;
  RateSelectState state_ = RateSelectState::UNSUPPORTED;
  RateSelectSetting setting_ = RateSelectSetting::UNSUPPORTED;
};
} // namespace fboss
} // namespace facebook
