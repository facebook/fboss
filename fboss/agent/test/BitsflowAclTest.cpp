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

namespace {
enum class BitsflowLockdownLevels {
  FORWARDING_ONLY = 1,
  FORWARDING_WITH_LOCAL_CLIENTS = 2,
  FORWARDING_WITH_NON_WRITE_CLIENTS = 3,
  FORWARDING_WITH_SPECIFIC_WRITE_CLIENTS = 4,
  DISABLED = 5,
};

/*
 * These ACLs are copied from configerator, where the bitsflow ACLs
 * are generated, desired to keep it in sync once in a while though
 * its not a requirement.
 */
std::map<BitsflowLockdownLevels, std::string> lockdownLevelAcl = {
    {BitsflowLockdownLevels::FORWARDING_ONLY,
     "bitsflow_fboss_wedge_agent_forwarding_only.materialized_JSON"},
    {BitsflowLockdownLevels::FORWARDING_WITH_LOCAL_CLIENTS,
     "bitsflow_fboss_wedge_agent_forwarding_with_local_clients.materialized_JSON"},
    {BitsflowLockdownLevels::FORWARDING_WITH_NON_WRITE_CLIENTS,
     "bitsflow_fboss_wedge_agent_forwarding_with_non_write_clients.materialized_JSON"},
    {BitsflowLockdownLevels::FORWARDING_WITH_SPECIFIC_WRITE_CLIENTS,
     "bitsflow_fboss_wedge_agent_forwarding_with_specific_write_clients.materialized_JSON"},
    {BitsflowLockdownLevels::DISABLED, ""}};

const auto kBitsflowAclsPath{"fboss/agent/test/files/bitsflow_acls/"};
std::string getStaticFileAclPath(BitsflowLockdownLevels level) {
  std::string aclPath = "";
  auto iter = lockdownLevelAcl.find(level);
  if (iter != lockdownLevelAcl.end()) {
    aclPath = kBitsflowAclsPath + iter->second;
  }
  return aclPath;
}
} // namespace

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
