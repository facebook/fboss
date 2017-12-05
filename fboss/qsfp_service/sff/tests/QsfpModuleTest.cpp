/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/sff/tests/MockQsfpModule.h"
#include "fboss/qsfp_service/sff/tests/MockTransceiverImpl.h"

#include <folly/Memory.h>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/sff/TransceiverImpl.h"
#include "fboss/qsfp_service/sff/QsfpModule.h"
#include "fboss/qsfp_service/sff/SffFieldInfo.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace facebook { namespace fboss {
using namespace ::testing;

class QsfpModuleTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto transceiverImpl = std::make_unique<NiceMock<MockTransceiverImpl> >();
    auto implPtr = transceiverImpl.get();
    // So we can check what happens during testing
    transImpl_ = transceiverImpl.get();
    qsfp_ = std::make_unique<MockQsfpModule>(std::move(transceiverImpl));

    // We're explicitly setting the value of the dirty bit, so fudge this
    EXPECT_CALL(*qsfp_, cacheIsValid()).WillRepeatedly(Return(true));
    EXPECT_CALL(*implPtr, detectTransceiver()).WillRepeatedly(Return(true));
  }

  PortStatus portStatus(bool enabled, bool up, int32_t speed = 25000) {
  // usese dummy tcvr mapping for now
    TransceiverIdxThrift tcvr(
      apache::thrift::FragileConstructor::FRAGILE, qsfp_->getID(), 0, {});
    return PortStatus(
      apache::thrift::FragileConstructor::FRAGILE,
      enabled, up, false, tcvr, speed);
  }

  std::unique_ptr<MockQsfpModule> qsfp_;
  NiceMock<MockTransceiverImpl>* transImpl_;
};

TEST_F(QsfpModuleTest, setRateSelect) {
  ON_CALL(*qsfp_, setRateSelectIfSupported(_, _, _))
    .WillByDefault(Invoke(qsfp_.get(),
          &MockQsfpModule::actualSetRateSelectIfSupported));
  EXPECT_CALL(*qsfp_, setPowerOverrideIfSupported(_)).Times(AtLeast(1));
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(*qsfp_, refreshCacheIfPossibleLocked()).Times(AtLeast(1));

  // every customize call does 1 write for enabling tx, plus any other
  // writes for settings changes.
  {
    InSequence a;
    // Unsupported
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(4);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    // Using V1
    qsfp_->setRateSelect(RateSelectState::EXTENDED_RATE_SELECT_V1,
        RateSelectSetting::FROM_6_6GB_AND_ABOVE);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);

    qsfp_->setRateSelect(RateSelectState::EXTENDED_RATE_SELECT_V2,
        RateSelectSetting::LESS_THAN_12GB);
    // 40G + LESS_THAN_12GB -> no change
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // 100G + LESS_THAN_12GB -> needs change
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(3);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    qsfp_->setRateSelect(RateSelectState::EXTENDED_RATE_SELECT_V2,
        RateSelectSetting::FROM_24GB_to_26GB);
    // 40G + FROM_24GB_to_26GB -> needs change
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(3);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // 100G + FROM_24GB_to_26GB -> no change
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);
  }
}

TEST_F(QsfpModuleTest, retrieveRateSelectSetting) {
  auto data = qsfp_->getRateSelectSettingValue(RateSelectState::UNSUPPORTED);
  EXPECT_EQ(data, RateSelectSetting::UNSUPPORTED);

  data = qsfp_->getRateSelectSettingValue(
      RateSelectState::APPLICATION_RATE_SELECT);
  EXPECT_EQ(data, RateSelectSetting::UNSUPPORTED);

  EXPECT_CALL(*qsfp_, getSettingsValue(_, _)).WillRepeatedly(
      Return(0b01010101));
  data = qsfp_->getRateSelectSettingValue(
      RateSelectState::EXTENDED_RATE_SELECT_V1);
  EXPECT_EQ(data, RateSelectSetting::FROM_2_2GB_TO_6_6GB);

  data = qsfp_->getRateSelectSettingValue(
      RateSelectState::EXTENDED_RATE_SELECT_V2);
  EXPECT_EQ(data, RateSelectSetting::FROM_12GB_TO_24GB);
}

TEST_F(QsfpModuleTest, setCdr) {
  ON_CALL(*qsfp_, setCdrIfSupported(_, _, _))
    .WillByDefault(Invoke(qsfp_.get(),
          &MockQsfpModule::actualSetCdrIfSupported));

  EXPECT_CALL(*qsfp_, setPowerOverrideIfSupported(_)).Times(AtLeast(1));
  EXPECT_CALL(*qsfp_, setRateSelectIfSupported(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(*qsfp_, refreshCacheIfPossibleLocked()).Times(AtLeast(1));

  // every customize call does 1 write for enabling tx, plus any other
  // writes for settings changes.
  {
    InSequence a;
    // Unsupported
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(3);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    qsfp_->setCdrState(FeatureState::DISABLED, FeatureState::DISABLED);
    // Disabled + 40G
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // Disabled + 100G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(2);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    qsfp_->setCdrState(FeatureState::ENABLED, FeatureState::ENABLED);
    // Enabled + 40G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(2);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // Enabled + 100G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    // One of rx an tx enabled with the other disabled
    qsfp_->setCdrState(FeatureState::DISABLED, FeatureState::ENABLED);
    // 40G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(2);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // 100G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(2);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);
  }
}

TEST_F(QsfpModuleTest, customizeIfDown25G) {
  qsfp_->portChanged(1, portStatus(true, false));
  qsfp_->portChanged(2, portStatus(true, false));
  qsfp_->portChanged(3, portStatus(true, false));
  qsfp_->portChanged(4, portStatus(true, false));

  // should customize w/ 25G
  EXPECT_CALL(
    *qsfp_, setCdrIfSupported(cfg::PortSpeed::TWENTYFIVEG, _, _)).Times(1);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDownSpeedMismatch) {
  qsfp_->portChanged(1, portStatus(true, false, 50000));
  qsfp_->portChanged(2, portStatus(true, false));
  qsfp_->portChanged(3, portStatus(true, false));
  qsfp_->portChanged(4, portStatus(true, false));

  // Speeds don't match across ports, should throw
  EXPECT_ANY_THROW(qsfp_->customizeTransceiverIfDown());
}

TEST_F(QsfpModuleTest, customizeIfDownSpeedMismatchButDisabled) {
  qsfp_->portChanged(1, portStatus(false, false, 50000));
  qsfp_->portChanged(2, portStatus(true, false));
  qsfp_->portChanged(3, portStatus(true, false));
  qsfp_->portChanged(4, portStatus(true, false));

  // should customize w/ 25G. Mismatched port is disabled
  EXPECT_CALL(
    *qsfp_, setCdrIfSupported(cfg::PortSpeed::TWENTYFIVEG, _, _)).Times(1);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDown50G) {
  qsfp_->portChanged(1, portStatus(true, false, 50000));
  qsfp_->portChanged(2, portStatus(false, false));
  qsfp_->portChanged(3, portStatus(true, false, 50000));
  qsfp_->portChanged(4, portStatus(false, false));

  // should customize w/ 50G
  EXPECT_CALL(
    *qsfp_, setCdrIfSupported(cfg::PortSpeed::FIFTYG, _, _)).Times(1);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDownAllUp) {
  qsfp_->portChanged(1, portStatus(true, true));
  qsfp_->portChanged(2, portStatus(true, true));
  qsfp_->portChanged(3, portStatus(true, true));
  qsfp_->portChanged(4, portStatus(true, true));

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDownOneUp) {
  qsfp_->portChanged(1, portStatus(true, true));
  qsfp_->portChanged(2, portStatus(true, false));
  qsfp_->portChanged(3, portStatus(true, false));
  qsfp_->portChanged(4, portStatus(true, false));

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDownAllDown) {
  qsfp_->portChanged(1, portStatus(true, false));
  qsfp_->portChanged(2, portStatus(true, false));
  qsfp_->portChanged(3, portStatus(true, false));
  qsfp_->portChanged(4, portStatus(true, false));

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(1);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDownMissingPort) {
  qsfp_->portChanged(1, portStatus(true, false));
  qsfp_->portChanged(2, portStatus(true, false));
  qsfp_->portChanged(3, portStatus(true, false));

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDownExtraPort) {
  qsfp_->portChanged(1, portStatus(true, false));
  qsfp_->portChanged(2, portStatus(true, false));
  qsfp_->portChanged(3, portStatus(true, false));
  qsfp_->portChanged(4, portStatus(true, false));
  qsfp_->portChanged(5, portStatus(true, false));

  EXPECT_ANY_THROW(qsfp_->customizeTransceiverIfDown());
}

TEST_F(QsfpModuleTest, customizeIfDownNonsensicalDisabledButUp) {
  qsfp_->portChanged(1, portStatus(false, true));
  qsfp_->portChanged(2, portStatus(false, true));
  qsfp_->portChanged(3, portStatus(false, true));
  qsfp_->portChanged(4, portStatus(false, true));

  // should not customize if any port is up or no ports are enabled
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->customizeTransceiverIfDown();
}

TEST_F(QsfpModuleTest, customizeIfDownNonsensicalDisabledButUpOneEnabled) {
  qsfp_->portChanged(1, portStatus(false, true));
  qsfp_->portChanged(2, portStatus(false, true));
  qsfp_->portChanged(3, portStatus(false, true));
  qsfp_->portChanged(4, portStatus(true, false));

  // This will never happen, but even if the only up ports are
  // disabled we shouldn't customize
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->customizeTransceiverIfDown();
}

}} // namespace facebook::fboss
