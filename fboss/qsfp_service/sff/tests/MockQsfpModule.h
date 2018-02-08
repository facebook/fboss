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
#include "fboss/qsfp_service/sff/TransceiverImpl.h"
#include "fboss/qsfp_service/sff/QsfpModule.h"
#include "fboss/qsfp_service/sff/SffFieldInfo.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace facebook { namespace fboss {

class MockQsfpModule : public QsfpModule {
 public:
  explicit MockQsfpModule(std::unique_ptr<TransceiverImpl> qsfpImpl) :
      QsfpModule(std::move(qsfpImpl)) {
    ON_CALL(*this, updateQsfpData(testing::_))
      .WillByDefault(testing::Assign(&dirty_, false));
  }
  MOCK_METHOD1(setPowerOverrideIfSupported, void(PowerControlState));
  MOCK_CONST_METHOD0(cacheIsValid, bool());
  MOCK_METHOD1(updateQsfpData, void(bool));
  MOCK_METHOD2(getSettingsValue, uint8_t(SffField, uint8_t));
  MOCK_METHOD0(getTransceiverInfo, TransceiverInfo());

  MOCK_METHOD3(setCdrIfSupported, void(cfg::PortSpeed, FeatureState,
        FeatureState));
  MOCK_METHOD3(setRateSelectIfSupported, void(cfg::PortSpeed,
        RateSelectState, RateSelectSetting));
  // Provide way to call parent
  void actualSetCdrIfSupported(cfg::PortSpeed speed, FeatureState tx,
      FeatureState rx) {
    QsfpModule::setCdrIfSupported(speed, tx, rx);
  }
  void actualSetRateSelectIfSupported(cfg::PortSpeed speed,
      RateSelectState currentState, RateSelectSetting currentSetting) {
    QsfpModule::setRateSelectIfSupported(speed, currentState, currentSetting);
  }
  void actualUpdateQsfpData(bool full) {
    present_ = true;
    QsfpModule::updateQsfpData(full);
  }
  void setFlatMem() {
    flatMem_ = false;
  }


  void customizeTransceiver(cfg::PortSpeed speed) override {
    dirty_ = false;
    present_ = true;
    QsfpModule::customizeTransceiver(speed);
  }

  bool getTransceiverSettingsInfo(TransceiverSettings& settings) override {
    settings.cdrTx = cdrTx_;
    settings.cdrRx = cdrRx_;
    settings.rateSelect = state_;
    settings.rateSelectSetting = setting_;
    return true;
  }

  void setRateSelect(RateSelectState state, RateSelectSetting setting) {
    state_ = state;
    setting_ = setting;
  }

  void setCdrState(FeatureState tx, FeatureState rx) {
    cdrTx_ = tx;
    cdrRx_ = rx;
  }
  RateSelectSetting getRateSelectSettingValue(RateSelectState state) {
    return QsfpModule::getRateSelectSettingValue(state);
  }

 private:
  FeatureState cdrTx_ = FeatureState::UNSUPPORTED;
  FeatureState cdrRx_ = FeatureState::UNSUPPORTED;
  RateSelectState state_ = RateSelectState::UNSUPPORTED;
  RateSelectSetting setting_ = RateSelectSetting::UNSUPPORTED;
};
}}
