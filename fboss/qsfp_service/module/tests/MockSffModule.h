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
      std::set<std::string> portNames,
      TransceiverImpl* qsfpImpl,
      std::shared_ptr<const TransceiverConfig> cfgOverridePtr,
      std::string tcvrName)
      : SffModule(
            std::move(portNames),
            qsfpImpl,
            cfgOverridePtr,
            std::move(tcvrName)) {
    ON_CALL(*this, updateQsfpData(testing::_))
        .WillByDefault(testing::Assign(&dirty_, false));
    ON_CALL(*this, ensureTransceiverReadyLocked())
        .WillByDefault(testing::Return(true));
    ON_CALL(*this, numHostLanes()).WillByDefault(testing::Return(4));
    ON_CALL(*this, getTransceiverInfo())
        .WillByDefault(testing::Return(TransceiverInfo{}));
  }
  MOCK_METHOD1(setPowerOverrideIfSupportedLocked, void(PowerControlState));
  MOCK_METHOD1(updateQsfpData, void(bool));
  MOCK_CONST_METHOD0(numHostLanes, unsigned int());
  MOCK_CONST_METHOD2(getSettingsValue, uint8_t(SffField, uint8_t));
  MOCK_CONST_METHOD0(getTransceiverInfo, TransceiverInfo());

  MOCK_METHOD3(
      setCdrIfSupported,
      void(cfg::PortSpeed, FeatureState, FeatureState));
  MOCK_METHOD3(
      setRateSelectIfSupported,
      void(cfg::PortSpeed, RateSelectState, RateSelectSetting));

  MOCK_CONST_METHOD0(getQsfpTransmitterTechnology, TransmitterTechnology());

  MOCK_CONST_METHOD1(configuredHostLanes, std::vector<uint8_t>(uint8_t));
  MOCK_CONST_METHOD1(configuredMediaLanes, std::vector<uint8_t>(uint8_t));

  MOCK_METHOD0(ensureTxEnabled, void());
  MOCK_METHOD0(resetLowPowerMode, void());
  MOCK_METHOD0(ensureTransceiverReadyLocked, bool());
  MOCK_CONST_METHOD0(customizationSupported, bool());
  MOCK_METHOD0(getModuleStatus, ModuleStatus());
  MOCK_METHOD0(getVendorInfo, Vendor());
  MOCK_METHOD0(verifyEepromChecksums, bool());
  MOCK_CONST_METHOD0(getModuleMediaInterface, MediaInterfaceCode());

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
    info.tcvrState()->vendor() = vendor;
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

  bool writeTransceiver(TransceiverIOParameters param, const uint8_t* data)
      override {
    return SffModule::writeTransceiver(param, data);
  }

  void setFwVersion(std::string appFwVersion, std::string dspFwVersion) {
    ModuleStatus moduleStatus;
    FirmwareStatus fw;
    fw.version() = appFwVersion;
    fw.dspFwVer() = dspFwVersion;
    moduleStatus.fwStatus() = fw;
    ON_CALL(*this, getModuleStatus())
        .WillByDefault(testing::Return(moduleStatus));
  }

  void setAppFwVersion(std::string fwVersion) {
    ModuleStatus moduleStatus;
    FirmwareStatus fw;
    fw.version() = fwVersion;
    moduleStatus.fwStatus() = fw;
    ON_CALL(*this, getModuleStatus())
        .WillByDefault(testing::Return(moduleStatus));
  }

  void setDspFwVersion(std::string fwVersion) {
    ModuleStatus moduleStatus;
    FirmwareStatus fw;
    fw.dspFwVer() = fwVersion;
    moduleStatus.fwStatus() = fw;
    ON_CALL(*this, getModuleStatus())
        .WillByDefault(testing::Return(moduleStatus));
  }

  void overrideVendorPN(std::string partNumber) {
    Vendor vendor;
    vendor.partNumber() = partNumber;
    ON_CALL(*this, getVendorInfo()).WillByDefault(testing::Return(vendor));
  }

  void overrideVendorInfo(
      std::string name,
      std::string partNumber,
      std::string serialNumber) {
    Vendor vendor;
    vendor.name() = name;
    vendor.partNumber() = partNumber;
    vendor.serialNumber() = serialNumber;
    ON_CALL(*this, getVendorInfo()).WillByDefault(testing::Return(vendor));
  }

  void overrideValidChecksums(bool isValid) {
    ON_CALL(*this, verifyEepromChecksums())
        .WillByDefault(testing::Return(isValid));
  }
  void overrideMediaInterfaceCode(MediaInterfaceCode mediaInterfaceCode) {
    ON_CALL(*this, getModuleMediaInterface())
        .WillByDefault(testing::Return(mediaInterfaceCode));
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
