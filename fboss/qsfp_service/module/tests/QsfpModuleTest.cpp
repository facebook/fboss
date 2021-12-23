/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/module/tests/MockSffModule.h"
#include "fboss/qsfp_service/module/tests/MockTransceiverImpl.h"
#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"

#include <folly/Memory.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"
#include "fboss/qsfp_service/module/sff/SffFieldInfo.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace facebook {
namespace fboss {
using namespace ::testing;

class QsfpModuleTest : public ::testing::Test {
 public:
  void SetUp() override {
    wedgeManager_ = std::make_unique<MockWedgeManager>();
    // save some typing in most tests by creating the test qsfp
    // expecting 4 ports. Tests that need a different number of ports
    // can call setupQsfp() themselves.
    setupQsfp(4);
  }

  void setupQsfp(unsigned int portsPerTransceiver) {
    auto transceiverImpl = std::make_unique<NiceMock<MockTransceiverImpl>>();
    auto implPtr = transceiverImpl.get();
    // So we can check what happens during testing
    transImpl_ = transceiverImpl.get();
    qsfp_ = std::make_unique<MockSffModule>(
        wedgeManager_.get(), std::move(transceiverImpl), portsPerTransceiver);
    qsfp_->setVendorPN();

    gflags::SetCommandLineOptionWithMode(
        "tx_enable_interval", "0", gflags::SET_FLAGS_DEFAULT);

    // We're explicitly setting the value of the dirty bit, so fudge this
    EXPECT_CALL(*qsfp_, cacheIsValid()).WillRepeatedly(Return(true));
    EXPECT_CALL(*implPtr, detectTransceiver()).WillRepeatedly(Return(true));
    qsfp_->detectPresence();
  }

  PortStatus portStatus(bool enabled, bool up, int32_t speed = 25000) {
    // usese dummy tcvr mapping for now
    TransceiverIdxThrift tcvr(
        apache::thrift::FragileConstructor::FRAGILE, qsfp_->getID(), 0, {});
    return PortStatus(
        apache::thrift::FragileConstructor::FRAGILE,
        enabled,
        up,
        false,
        tcvr,
        speed,
        "");
  }

  NiceMock<MockTransceiverImpl>* transImpl_;
  std::unique_ptr<MockWedgeManager> wedgeManager_;
  std::unique_ptr<MockSffModule> qsfp_;
};

TEST_F(QsfpModuleTest, setRateSelect) {
  ON_CALL(*qsfp_, setRateSelectIfSupported(_, _, _))
      .WillByDefault(
          Invoke(qsfp_.get(), &MockSffModule::actualSetRateSelectIfSupported));
  EXPECT_CALL(*qsfp_, setPowerOverrideIfSupported(_)).Times(AtLeast(1));
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(AtLeast(1));

  // every customize call does 1 write for enabling tx, plus any other
  // writes for settings changes.
  {
    InSequence a;
    // Unsupported
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    // Using V1
    qsfp_->setRateSelect(
        RateSelectState::EXTENDED_RATE_SELECT_V1,
        RateSelectSetting::FROM_6_6GB_AND_ABOVE);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);

    qsfp_->setRateSelect(
        RateSelectState::EXTENDED_RATE_SELECT_V2,
        RateSelectSetting::LESS_THAN_12GB);
    // 40G + LESS_THAN_12GB -> no change
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // 100G + LESS_THAN_12GB -> needs change
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(2);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    qsfp_->setRateSelect(
        RateSelectState::EXTENDED_RATE_SELECT_V2,
        RateSelectSetting::FROM_24GB_to_26GB);
    // 40G + FROM_24GB_to_26GB -> needs change
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(2);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // 100G + FROM_24GB_to_26GB -> no change
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);
  }
}

TEST_F(QsfpModuleTest, retrieveRateSelectSetting) {
  auto data = qsfp_->getRateSelectSettingValue(RateSelectState::UNSUPPORTED);
  EXPECT_EQ(data, RateSelectSetting::UNSUPPORTED);

  data = qsfp_->getRateSelectSettingValue(
      RateSelectState::APPLICATION_RATE_SELECT);
  EXPECT_EQ(data, RateSelectSetting::UNSUPPORTED);

  EXPECT_CALL(*qsfp_, getSettingsValue(_, _))
      .WillRepeatedly(Return(0b01010101));
  data = qsfp_->getRateSelectSettingValue(
      RateSelectState::EXTENDED_RATE_SELECT_V1);
  EXPECT_EQ(data, RateSelectSetting::FROM_2_2GB_TO_6_6GB);

  data = qsfp_->getRateSelectSettingValue(
      RateSelectState::EXTENDED_RATE_SELECT_V2);
  EXPECT_EQ(data, RateSelectSetting::FROM_12GB_TO_24GB);
}

TEST_F(QsfpModuleTest, setCdr) {
  ON_CALL(*qsfp_, setCdrIfSupported(_, _, _))
      .WillByDefault(
          Invoke(qsfp_.get(), &MockSffModule::actualSetCdrIfSupported));

  EXPECT_CALL(*qsfp_, setPowerOverrideIfSupported(_)).Times(AtLeast(1));
  EXPECT_CALL(*qsfp_, setRateSelectIfSupported(_, _, _)).Times(AtLeast(1));

  // every customize call does 1 write for enabling tx, plus any other
  // writes for settings changes.
  {
    InSequence a;
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
    // Unsupported
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    qsfp_->setCdrState(FeatureState::DISABLED, FeatureState::DISABLED);
    // Disabled + 40G
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG);
    // Disabled + 100G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG); // CHECK

    qsfp_->setCdrState(FeatureState::ENABLED, FeatureState::ENABLED);
    // Enabled + 40G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG); // CHECK
    // Enabled + 100G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG); // CHECK

    // One of rx an tx enabled with the other disabled
    qsfp_->setCdrState(FeatureState::DISABLED, FeatureState::ENABLED);
    // 40G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
    qsfp_->customizeTransceiver(cfg::PortSpeed::FORTYG); // CHECK
    // 100G
    EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
    qsfp_->customizeTransceiver(cfg::PortSpeed::HUNDREDG);
  }
}

TEST_F(QsfpModuleTest, portsChangedAllDown25G) {
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));
  // should customize w/ 25G
  EXPECT_CALL(*qsfp_, setCdrIfSupported(cfg::PortSpeed::TWENTYFIVEG, _, _))
      .Times(1);

  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });
}

TEST_F(QsfpModuleTest, portsChangedSpeedMismatch) {
  // Speeds don't match across ports, should throw
  EXPECT_ANY_THROW(qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false, 50000)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  }));
}

TEST_F(QsfpModuleTest, portsChangedSpeedMismatchButDisabled) {
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));
  // should customize w/ 25G. Mismatched port is disabled
  EXPECT_CALL(*qsfp_, setCdrIfSupported(cfg::PortSpeed::TWENTYFIVEG, _, _))
      .Times(1);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(false, false, 50000)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });
}

TEST_F(QsfpModuleTest, portsChanged50G) {
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));
  // should customize w/ 50G
  EXPECT_CALL(*qsfp_, setCdrIfSupported(cfg::PortSpeed::FIFTYG, _, _)).Times(1);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false, 50000)},
      {2, portStatus(false, false)},
      {3, portStatus(true, false, 50000)},
      {4, portStatus(false, false)},
  });
}

TEST_F(QsfpModuleTest, portsChangedAllUp) {
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, true)},
      {2, portStatus(true, true)},
      {3, portStatus(true, true)},
      {4, portStatus(true, true)},
  });
}

TEST_F(QsfpModuleTest, portsChangedOneUp) {
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, true)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });
}

TEST_F(QsfpModuleTest, portsChangedAllDown) {
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(1);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });
}

TEST_F(QsfpModuleTest, portsChangedMissingPort) {
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
  });
}

TEST_F(QsfpModuleTest, portsChangedExtraPort) {
  EXPECT_ANY_THROW(qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
      {5, portStatus(true, false)},
  }));
}

TEST_F(QsfpModuleTest, portsChangedOnePortPerModule) {
  setupQsfp(1);
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(1);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
  });
}

TEST_F(QsfpModuleTest, portsChangedMissingPortOnePortPerModule) {
  setupQsfp(1);
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({});
}

TEST_F(QsfpModuleTest, portsChangedExtraPortOnePortPerModule) {
  setupQsfp(1);
  EXPECT_ANY_THROW(qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  }));
}

TEST_F(QsfpModuleTest, portsChangedNonsensicalDisabledButUp) {
  // should not customize if any port is up or no ports are enabled
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(false, true)},
      {2, portStatus(false, true)},
      {3, portStatus(false, true)},
      {4, portStatus(false, true)},
  });
}

TEST_F(QsfpModuleTest, portsChangedNonsensicalDisabledButUpOneEnabled) {
  // This will never happen, but even if the only up ports are
  // disabled we shouldn't customize
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(false, true)},
      {2, portStatus(false, true)},
      {3, portStatus(false, true)},
      {4, portStatus(true, false)},
  });
}

TEST_F(QsfpModuleTest, portsChangedNotDirtySafeToCustomize) {
  // refresh, which should set module dirty_ = false
  qsfp_->refresh();
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });

  // However, we should call customize when we next refresh (though
  // not on subsequent refresh calls)
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(1);
  qsfp_->refresh();
  qsfp_->refresh();
}

TEST_F(QsfpModuleTest, portsChangedNotDirtySafeToCustomizeStale) {
  // refresh, which should set module dirty_ = false
  qsfp_->refresh();
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });

  gflags::SetCommandLineOptionWithMode(
      "customize_interval", "0", gflags::SET_FLAGS_DEFAULT);

  // However, we should call customize when we next refresh
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(2);
  qsfp_->refresh();

  // With interval of 0, data should now be stale so we should
  // customize again.
  qsfp_->refresh();
}

TEST_F(QsfpModuleTest, portsChangedNotDirtyNotSafeToCustomize) {
  // refresh, which should set module dirty_ = false
  qsfp_->refresh();

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, true)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });

  // one up port should prevent customization on future refresh calls
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->refresh();
  qsfp_->refresh();
}

TEST_F(QsfpModuleTest, portsChangedWithNewStateMachineNotSafeToCustomize) {
  // refresh, which should set module dirty_ = false
  qsfp_->refresh();

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });

  gflags::SetCommandLineOptionWithMode(
      "customize_interval", "0", gflags::SET_FLAGS_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "use_new_state_machine", "1", gflags::SET_FLAGS_DEFAULT);

  // Even with interval of 0 and all ports down, because it's using new state
  // machine, we won't trigger customization at all.
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->refresh();
  qsfp_->refresh();
}

TEST_F(QsfpModuleTest, refreshDirtyNoPorts) {
  // should not customize if we don't have knowledge of all ports
  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->refresh();
}

TEST_F(QsfpModuleTest, updateQsfpDataPartial) {
  // Ensure that partial updates don't ever call writeTranscevier,
  // which needs to gain control of the bus and slows the call
  // down drastically.
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
  qsfp_->actualUpdateQsfpData(false);
}

TEST_F(QsfpModuleTest, updateQsfpDataFull) {
  // Bit of a hack to ensure we have flatMem_ == false.
  ON_CALL(*transImpl_, readTransceiver(_, _, _, _))
      .WillByDefault(DoAll(
          InvokeWithoutArgs(qsfp_.get(), &MockSffModule::setFlatMem),
          Return(0)));

  // Full updates do need to write to select higher pages
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(AtLeast(1));

  qsfp_->actualUpdateQsfpData(true);
}

TEST_F(QsfpModuleTest, skipCustomizingMissingPorts) {
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));
  // set present_ = false, dirty_ = true
  EXPECT_CALL(*transImpl_, detectTransceiver()).WillRepeatedly(Return(false));
  qsfp_->detectPresence();

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });

  // We should also not actually customize on subsequent refresh calls
  qsfp_->refresh();
  qsfp_->refresh();
}

TEST_F(QsfpModuleTest, skipCustomizingCopperPorts) {
  ON_CALL(*qsfp_, getTransceiverInfo())
      .WillByDefault(Return(qsfp_.get()->fakeInfo_));
  // Should get detected as copper and skip all customization
  EXPECT_CALL(*qsfp_, getQsfpTransmitterTechnology())
      .WillRepeatedly(Return(TransmitterTechnology::COPPER));

  EXPECT_CALL(*qsfp_, setCdrIfSupported(_, _, _)).Times(0);
  qsfp_->transceiverPortsChanged({
      {1, portStatus(true, false)},
      {2, portStatus(true, false)},
      {3, portStatus(true, false)},
      {4, portStatus(true, false)},
  });

  // We should also not actually customize on subsequent refresh calls
  qsfp_->refresh();
  qsfp_->refresh();
}

TEST_F(QsfpModuleTest, readTransceiver) {
  // Skip the length field and confirm that the length of data in response is 1.
  // Page is also skipped so there should not be a write to byte 127.
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
  EXPECT_CALL(*transImpl_, readTransceiver(_, _, _, _)).Times(1);
  TransceiverIOParameters param;
  param.offset_ref() = 0;
  auto buf = qsfp_->readTransceiver(param);
  EXPECT_EQ(buf->length(), 1);

  // Test for a specific length
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
  EXPECT_CALL(*transImpl_, readTransceiver(_, _, _, _)).Times(1);
  param.length_ref() = 10;
  buf = qsfp_->readTransceiver(param);
  EXPECT_EQ(buf->length(), *param.length_ref());

  // Set the page
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
  EXPECT_CALL(*transImpl_, readTransceiver(_, _, _, _)).Times(1);
  param.page_ref() = 3;
  buf = qsfp_->readTransceiver(param);
  EXPECT_EQ(buf->length(), *param.length_ref());

  // Test on a transceiver that fails detection
  EXPECT_CALL(*transImpl_, detectTransceiver()).WillRepeatedly(Return(false));
  EXPECT_CALL(*transImpl_, readTransceiver(_, _, _, _)).Times(0);
  qsfp_->detectPresence();
  buf = qsfp_->readTransceiver(param);
  EXPECT_EQ(buf->length(), 0);
}

TEST_F(QsfpModuleTest, writeTransceiver) {
  // Expect a call to writeTransceiver and the result to be successful
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(1);
  TransceiverIOParameters param;
  param.offset_ref() = 0x23;
  EXPECT_EQ(qsfp_->writeTransceiver(param, 0xab), true);

  // Set the page
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(2);
  param.page_ref() = 3;
  EXPECT_EQ(qsfp_->writeTransceiver(param, 0xde), true);

  // Test on a transceiver that fails detection, the result should be false
  EXPECT_CALL(*transImpl_, detectTransceiver()).WillRepeatedly(Return(false));
  EXPECT_CALL(*transImpl_, writeTransceiver(_, _, _, _)).Times(0);
  qsfp_->detectPresence();
  EXPECT_EQ(qsfp_->writeTransceiver(param, 0xac), false);
}

TEST_F(QsfpModuleTest, populateSnapshots) {
  auto snapshots = qsfp_->getTransceiverSnapshots().getSnapshots();
  EXPECT_TRUE(snapshots.empty());
  qsfp_->refresh();
  snapshots = qsfp_->getTransceiverSnapshots().getSnapshots();
  EXPECT_FALSE(snapshots.empty());

  // fill the buffer
  for (auto i = 1; i < snapshots.maxSize(); i++) {
    qsfp_->refresh();
  }
  snapshots = qsfp_->getTransceiverSnapshots().getSnapshots();

  // Verify that we stay at the max size
  EXPECT_EQ(snapshots.size(), snapshots.maxSize());
  qsfp_->refresh();
  snapshots = qsfp_->getTransceiverSnapshots().getSnapshots();
  EXPECT_EQ(snapshots.size(), snapshots.maxSize());
}
} // namespace fboss
} // namespace facebook
