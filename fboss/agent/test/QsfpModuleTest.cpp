/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/Memory.h>
#include "fboss/agent/TransceiverImpl.h"
#include "fboss/agent/QsfpModule.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/optic_types.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace facebook::fboss;
using namespace ::testing;
namespace {

class MockQsfpModule : public QsfpModule {
 public:
  explicit MockQsfpModule(std::unique_ptr<TransceiverImpl> qsfpImpl) :
    QsfpModule(std::move(qsfpImpl)) {}
  MOCK_METHOD1(setPowerOverrideIfSupported, void(PowerControlState));
  MOCK_CONST_METHOD0(cacheIsValid, bool());
  void customizeTransceiver(cfg::PortSpeed speed) {
    dirty_ = false;
    QsfpModule::customizeTransceiver(speed);
  }
  bool getTransceiverSettingsInfo(TransceiverSettings &settings) {
    settings.cdrTx = tx_;
    settings.cdrRx = rx_;
    return true;
  }
  void setFeatureState(FeatureState tx, FeatureState rx) {
    tx_ = tx;
    rx_ = rx;
  }

 private:
  FeatureState tx_ = FeatureState::UNSUPPORTED;
  FeatureState rx_ = FeatureState::UNSUPPORTED;
};

class MockTransceiverImpl : public TransceiverImpl {
 public:
  MOCK_METHOD4(readTransceiver, int(int,int,int,uint8_t*));
  MOCK_METHOD4(writeTransceiver, int(int,int,int,uint8_t*));
  MOCK_METHOD0(detectTransceiver, bool());
  MOCK_METHOD0(getName, folly::StringPiece());
  MOCK_METHOD0(getNum, int());
};

/*class QsfpModuleTest : public ::testing::Test {
  public:
    void SetUp() override {

    }

}*/
TEST(QsfpModuleTest, setCdr) {
  auto transceiverImpl = folly::make_unique<NiceMock<MockTransceiverImpl> >();
  // So we can check what happens during testing
  auto trans = transceiverImpl.get();
  MockQsfpModule qsfp(std::move(transceiverImpl));

  // We're explicitly setting the value of the dirty bit, so fudge this
  EXPECT_CALL(qsfp, cacheIsValid()).WillRepeatedly(Return(true));
  EXPECT_CALL(qsfp, setPowerOverrideIfSupported(_)).Times(AtLeast(1));

  {
    InSequence a;
    // Unsupported
    EXPECT_CALL(*trans, writeTransceiver(_, _, _, _)).Times(0);
    qsfp.customizeTransceiver(cfg::PortSpeed::FORTYG);
    qsfp.customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    qsfp.setFeatureState(FeatureState::DISABLED, FeatureState::DISABLED);
    // Disabled + 40G
    qsfp.customizeTransceiver(cfg::PortSpeed::FORTYG);
    // Disabled + 100G
    EXPECT_CALL(*trans, writeTransceiver(_, _, _, _)).Times(1);
    qsfp.customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    qsfp.setFeatureState(FeatureState::ENABLED, FeatureState::ENABLED);
    // Enabled + 40G
    EXPECT_CALL(*trans, writeTransceiver(_, _, _, _)).Times(1);
    qsfp.customizeTransceiver(cfg::PortSpeed::FORTYG);
    // Enabled + 100G
    EXPECT_CALL(*trans, writeTransceiver(_, _, _, _)).Times(0);
    qsfp.customizeTransceiver(cfg::PortSpeed::HUNDREDG);

    // One of rx an tx enabled with the other disabled
    qsfp.setFeatureState(FeatureState::DISABLED, FeatureState::ENABLED);
    // 40G
    EXPECT_CALL(*trans, writeTransceiver(_, _, _, _)).Times(1);
    qsfp.customizeTransceiver(cfg::PortSpeed::FORTYG);
    // 100G
    EXPECT_CALL(*trans, writeTransceiver(_, _, _, _)).Times(1);
    qsfp.customizeTransceiver(cfg::PortSpeed::HUNDREDG);
  }
}

} // namespace facebook::fboss
