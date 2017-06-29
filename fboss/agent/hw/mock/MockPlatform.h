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
#include "fboss/agent/types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <gmock/gmock.h>

namespace facebook { namespace fboss {

class MockHwSwitch;

/*
 * MockPlatform is a mockable interface to a Platform. Non-critical
 * functions have stub implementations and functions that we need to
 * control for tests are mocked with gmock.
 */
class MockPlatform : public Platform {
 public:
  MockPlatform();
  explicit MockPlatform(std::unique_ptr<MockHwSwitch> hw);
  ~MockPlatform() override;

  HwSwitch* getHwSwitch() const override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;

  MOCK_METHOD1(createHandler, std::unique_ptr<ThriftHandler>(SwSwitch* sw));
  MOCK_METHOD1(getProductInfo, void(ProductInfo& productInfo));
  MOCK_CONST_METHOD1(getPortMapping, TransceiverIdxThrift(PortID port));
  MOCK_CONST_METHOD1(getPlatformPort, PlatformPort*(PortID port));
  MOCK_CONST_METHOD0(getLocalMac, folly::MacAddress());
  MOCK_METHOD1(onHwInitialized, void(SwSwitch* sw));

 private:
  void createTmpDir();
  void cleanupTmpDir();

  /*
   * A temporary directory that contains the volatile and persistent state
   * directories.
   *
   * Each MockPlatform uses a separate temporary directory.  This allows
   * multiple unit tests to run in parallel without conflicting.
   */
  folly::test::TemporaryDirectory tmpDir_;
  std::unique_ptr<MockHwSwitch> hw_;
};

}} // facebook::fboss
