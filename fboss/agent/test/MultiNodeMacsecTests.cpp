/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fboss/mka_service/if/facebook/gen-cpp2/MKAService.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/link_tests/facebook/MacsecTestUtils.h"
#include "fboss/facebook/mka_service/mka_module/MKAStructs.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/mka_service/if/facebook/gen-cpp2/mka_config_constants.h"
#include "fboss/mka_service/if/facebook/gen-cpp2/mka_config_types.h"

using namespace facebook::fboss;

using mka::Cak;
using mka::MACSecCapability;
using mka::mka_config_constants;
using mka::MKAConfig;
using mka::MKAServiceConfig;
using mka::MKASessionInfo;
using mka::MKATimers;
using mka::MKATransport;
using mka::OperStatus;
using mka::ProfileKeyStatus;

DECLARE_bool(enable_macsec);

class MultiNodeMacsecTest : public MultiNodeTest {
 public:
  void SetUp() override {
    MultiNodeTest::SetUp();
    setupMkaClient();
    // We need to wait for QSFP and Agent to program
    // XPHY and IPHY respectively before configuring MKA sessions ??
    // Creating MKA session happens in setup() method of test case.
    // sleep(60)
  }

  void TearDown() override {
    mkaClient_.reset();
    MultiNodeTest::TearDown();
  }

 protected:
  std::unique_ptr<facebook::fboss::mka::MKAServiceAsyncClient> mkaClient_;

  cfg::SwitchConfig initialConfig() const override {
    auto config = utility::onePortPerInterfaceConfig(
        platform()->getHwSwitch(),
        testPorts(),
        {{cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::NONE}},
        true /*interfaceHasSubnet*/,
        false /*setInterfaceMac*/);
    config.loadBalancers()->push_back(
        facebook::fboss::utility::getEcmpFullHashConfig(sw()->getPlatform()));
    return config;
  }

  void setCmdLineFlagOverrides() const override {
    FLAGS_enable_macsec = true;
    MultiNodeTest::setCmdLineFlagOverrides();
  }

  folly::MacAddress getLocalMac() {
    return sw()->getPlatform()->getLocalMac();
  }

  void setupMkaClient() {
    checkWithRetry(
        [this] {
          try {
            folly::SocketAddress addr("::1", 5920);
            auto socket = folly::AsyncSocket::newSocket(
                folly::EventBaseManager::get()->getEventBase(), addr, 5000);
            auto channel = apache::thrift::RocketClientChannel::newChannel(
                std::move(socket));
            mkaClient_ =
                std::make_unique<facebook::fboss::mka::MKAServiceAsyncClient>(
                    std::move(channel));
            mkaClient_->sync_getStatus();
          } catch (const std::exception& e) {
            XLOG(INFO) << " Failed to get mka_service status: " << e.what();
            return false;
          }
          return true;
        },
        120 /*retries*/,
        1s,
        "Never got a working connection to MKA with valid status");
  }

  void checkMacsecWithRetry(
      const std::vector<std::string>& macsecPorts,
      const std::string& ckn,
      bool reconnectMkaClientOnFailure,
      int retries) {
    checkWithRetry(
        [this, &macsecPorts, ckn, reconnectMkaClientOnFailure] {
          try {
            return utils::checkMacsecProgrammed(macsecPorts, ckn, mkaClient_);
          } catch (const std::exception& e) {
            if (reconnectMkaClientOnFailure) {
              XLOG(ERR) << " Error checking macsec sessions: " << e.what()
                        << " reinitinging macsec client";
              setupMkaClient();
            }
            return false;
          }
        },
        retries);
  }
};

TEST_F(MultiNodeMacsecTest, verifyMkaSession) {
  auto setup = [=]() {
    // Get ports
    auto ports = testPortNames();
    XLOG(INFO) << " No of test ports: " << ports.size();

    // Create MKA config for each port
    auto primaryCak = utils::makeCak(utils::getCak(), utils::getCkn());
    auto secondaryCak = utils::makeCak(utils::getCak2(), utils::getCkn2());
    std::optional<MKATimers> timers = std::nullopt;
    bool dropUnencrypted = true;
    for (auto p : ports) {
      auto config = utils::makeMKAConfig(
          p,
          primaryCak,
          secondaryCak,
          timers,
          dropUnencrypted,
          isDUT(),
          getLocalMac());
      mkaClient_->sync_updateKey(config);
    }
  };

  auto verify = [this]() {
    auto ports = testPortNames();
    checkMacsecWithRetry(ports, utils::getCkn(), false, 60);
  };

  verifyAcrossWarmBoots(setup, verify);
}
