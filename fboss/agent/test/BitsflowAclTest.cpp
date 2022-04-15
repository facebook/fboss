/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class BitsflowAclTest : public ::testing::Test {
 public:
  void initialSetup() {
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
    createThriftThread();
  }

  void TearDown() override {
    if (thriftServer_) {
      thriftServer_->stop();
    }
    if (thriftThread_.has_value()) {
      (*thriftThread_).join();
    }
    if (thriftServer_) {
      thriftServer_.reset();
    }
  }

 private:
  void createThriftThread() {
    uint16_t anyPort = 0;
    auto eventBase = new folly::EventBase();
    auto handler = std::make_shared<ThriftHandler>(sw_);
    folly::EventBaseManager::get()->clearEventBase();
    thriftServer_ = setupThriftServer(
        *eventBase, handler, anyPort, false /* isDuplex */, true /* setupSSL*/);
    thriftThread_ = std::thread([&]() noexcept {
      XLOG(DBG0) << "Starting thrift server thread ...";
      folly::setThreadName("agent-ThriftServer");
      thriftServer_->serve();
      XLOG(DBG0) << "Stopping thrift server thread ...";
    });

    // Wait until thrift server starts
    while (true) {
      auto evb = thriftServer_->getServeEventBase();
      if (evb != nullptr and evb->isRunning()) {
        break;
      }
      std::this_thread::yield();
    }
  }

  SwSwitch* sw_;
  std::optional<std::thread> thriftThread_;
  std::unique_ptr<HwTestHandle> handle_{nullptr};
  std::shared_ptr<apache::thrift::ThriftServer> thriftServer_{nullptr};
};
