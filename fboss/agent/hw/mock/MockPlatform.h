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

#include <folly/experimental/TestUtil.h>
#include "fboss/agent/Platform.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/MockAsic.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <gmock/gmock.h>

namespace facebook::fboss {

class MockHwSwitch;
class HwTestHandle;
class MockPlatformPort;
class HwSwitchWarmBootHelper;

/*
 * MockPlatform is a mockable interface to a Platform. Non-critical
 * functions have stub implementations and functions that we need to
 * control for tests are mocked with gmock.
 */
class MockPlatform : public Platform {
 public:
  MockPlatform();
  MockPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<MockHwSwitch> hw);
  ~MockPlatform() override;

  HwSwitch* getHwSwitch() const override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  HwAsic* getAsic() const override;
  static const folly::MacAddress& getMockLocalMac();
  static const folly::IPAddressV6& getMockLinkLocalIp6();
  PlatformPort* getPlatformPort(PortID id) const override;
  HwSwitchWarmBootHelper* getWarmBootHelper() override;

  MOCK_METHOD0(
      createHandler,
      std::shared_ptr<apache::thrift::AsyncProcessorFactory>());
  MOCK_METHOD1(getProductInfo, void(ProductInfo& productInfo));
  MOCK_CONST_METHOD2(
      getPortMapping,
      TransceiverIdxThrift(PortID port, cfg::PortSpeed speed));
  MOCK_METHOD0(stop, void());
  MOCK_METHOD1(onHwInitialized, void(HwSwitchCallback* sw));
  MOCK_METHOD1(onInitialConfigApplied, void(HwSwitchCallback* sw));
  MOCK_METHOD0(initPorts, void());
  MOCK_CONST_METHOD0(supportsAddRemovePort, bool());

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac) override;
  void createTmpDir();
  void cleanupTmpDir();

  void initImpl(uint32_t hwFeaturesDesired) override {}

  /*
   * A temporary directory that contains the volatile and persistent state
   * directories.
   *
   * Each MockPlatform uses a separate temporary directory.  This allows
   * multiple unit tests to run in parallel without conflicting.
   */
  folly::test::TemporaryDirectory tmpDir_;
  std::unique_ptr<MockHwSwitch> hw_;
  std::unique_ptr<MockAsic> asic_;
  std::unordered_map<PortID, std::unique_ptr<MockPlatformPort>> portMapping_;
};

} // namespace facebook::fboss
