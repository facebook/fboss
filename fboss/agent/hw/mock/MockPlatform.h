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
#include "fboss/agent/types.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"

#include <gmock/gmock.h>

namespace facebook { namespace fboss {

class MockHwSwitch;

class MockPlatform : public Platform {
 public:
  MockPlatform();
  ~MockPlatform() override;

  HwSwitch* getHwSwitch() const override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  cfg::PortSpeed getPortSpeed(PortID port) const;
  cfg::PortSpeed getMaxPortSpeed(PortID port) const;
  void getProductInfo(ProductInfo& info) override {
    // Nothing to do
  };

  MOCK_CONST_METHOD0(getLocalMac, folly::MacAddress());
  MOCK_METHOD1(onHwInitialized, void(SwSwitch*));

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
