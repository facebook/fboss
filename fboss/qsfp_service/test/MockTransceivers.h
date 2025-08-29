// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/sff/Sff8472Module.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/qsfp_service/module/tests/MockSffModule.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

namespace facebook::fboss {

class MockSff8472TransceiverImpl : public Sfp10GTransceiver {
 public:
  explicit MockSff8472TransceiverImpl(int module, TransceiverManager* mgr)
      : Sfp10GTransceiver(module, mgr) {}

  MOCK_METHOD0(detectTransceiver, bool());
  MOCK_METHOD0(triggerQsfpHardReset, void());
};

class MockSff8472Module : public Sff8472Module {
 public:
  explicit MockSff8472Module(
      std::set<std::string> portNames,
      MockSff8472TransceiverImpl* qsfpImpl,
      std::string tcvrName)
      : Sff8472Module(std::move(portNames), qsfpImpl, std::move(tcvrName)) {
    ON_CALL(*this, ensureTransceiverReadyLocked())
        .WillByDefault(testing::Return(true));
    ON_CALL(*this, numHostLanes()).WillByDefault(testing::Return(1));
  }

  MockSff8472TransceiverImpl* getTransceiverImpl() {
    return static_cast<MockSff8472TransceiverImpl*>(qsfpImpl_);
  }

  MOCK_METHOD1(configureModule, void(uint8_t));
  MOCK_METHOD1(customizeTransceiverLocked, void(TransceiverPortState&));
  MOCK_CONST_METHOD0(numHostLanes, unsigned int());

  MOCK_METHOD1(
      ensureRxOutputSquelchEnabled,
      void(const std::vector<HostLaneSettings>&));
  MOCK_METHOD0(resetDataPath, void());
  MOCK_METHOD1(updateCachedTransceiverInfoLocked, void(ModuleStatus));
  MOCK_METHOD0(ensureTransceiverReadyLocked, bool());
};

/*
 * Because this is a TransceiverStateMachine test, we only care the functions
 * we need to call in the process of New Port Programming using the new state
 * machine logic. We don't really care about whether the programming
 * iphy/xphy/xcvr actually changes the hardware in a unit test.
 * Therefore, we just need to mock the CmisModule and then check whether these
 * functions have been called correctly.
 */
class MockCmisTransceiverImpl : public Cmis200GTransceiver {
 public:
  explicit MockCmisTransceiverImpl(int module, TransceiverManager* mgr)
      : Cmis200GTransceiver(module, mgr) {}

  MOCK_METHOD0(detectTransceiver, bool());
  MOCK_METHOD0(triggerQsfpHardReset, void());
};

class MockCmisModule : public CmisModule {
 public:
  explicit MockCmisModule(
      std::set<std::string> portNames,
      MockCmisTransceiverImpl* qsfpImpl,
      std::shared_ptr<const TransceiverConfig> cfgPtr,
      std::string tcvrName)
      : CmisModule(
            std::move(portNames),
            qsfpImpl,
            cfgPtr,
            true /*supportRemediate*/,
            std::move(tcvrName)) {
    ON_CALL(*this, updateQsfpData(testing::_))
        .WillByDefault(testing::Assign(&dirty_, false));
    ON_CALL(*this, ensureTransceiverReadyLocked())
        .WillByDefault(testing::Return(true));
    ON_CALL(*this, getPortPrbsStateLocked(testing::_, testing::_))
        .WillByDefault(testing::Return(prbs::InterfacePrbsState()));
    ON_CALL(*this, numHostLanes()).WillByDefault(testing::Return(8));
  }

  MockCmisTransceiverImpl* getTransceiverImpl() {
    return static_cast<MockCmisTransceiverImpl*>(qsfpImpl_);
  }

  MOCK_METHOD1(configureModule, void(uint8_t));
  MOCK_METHOD1(customizeTransceiverLocked, void(TransceiverPortState&));
  MOCK_CONST_METHOD0(numHostLanes, unsigned int());

  MOCK_METHOD1(
      ensureRxOutputSquelchEnabled,
      void(const std::vector<HostLaneSettings>&));
  MOCK_METHOD0(resetDataPath, void());
  MOCK_METHOD1(updateQsfpData, void(bool));
  MOCK_METHOD1(updateCachedTransceiverInfoLocked, void(ModuleStatus));
  MOCK_CONST_METHOD0(ensureOutOfReset, void());
  MOCK_METHOD0(ensureTransceiverReadyLocked, bool());
  MOCK_METHOD2(
      getPortPrbsStateLocked,
      prbs::InterfacePrbsState(std::optional<const std::string>, phy::Side));
};

} // namespace facebook::fboss
