/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "openr/common/NetworkUtil.h"

#include <openr/if/gen-cpp2/OpenrCtrlCpp.h>
#include <memory>
#include "servicerouter/client/cpp2/ClientFactory.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

#include "common/process/Process.h"
#include "fboss/agent/RouteUpdateWrapper.h"

using namespace facebook::fboss;

static constexpr int kOpenrThriftPort{2018};

static auto constexpr kConnTimeout = 1000;
static auto constexpr kRecvTimeout = 45000;
static auto constexpr kSendTimeout = 5000;

static const std::string& kTestPrefix = "123::/96";

template <typename Client>
std::unique_ptr<Client> createPlaintextClient(const int port) {
  auto eb = folly::EventBaseManager::get()->getEventBase();
  auto addr = folly::SocketAddress("::1", port);
  auto sock = folly::AsyncSocket::newSocket(eb, addr, kConnTimeout);
  sock->setSendTimeout(kSendTimeout);
  auto channel =
      apache::thrift::HeaderClientChannel::newChannel(std::move(sock));
  channel->setTimeout(kRecvTimeout);
  return std::make_unique<Client>(std::move(channel));
}

std::unique_ptr<apache::thrift::Client<openr::thrift::OpenrCtrlCpp>>
getOpenrClient() {
  return createPlaintextClient<
      apache::thrift::Client<openr::thrift::OpenrCtrlCpp>>(kOpenrThriftPort);
}

void checkOpenrInitialization() {
  bool initializationConverged = false;
  checkWithRetry(
      [&initializationConverged]() {
        try {
          auto client = getOpenrClient();
          initializationConverged = client->sync_initializationConverged();
          return true;
        } catch (const std::exception& e) {
          return false;
        }
      },
      300, /* retries */
      1s,
      "Cannot connect to OpenR service");
  EXPECT_TRUE(initializationConverged);
}

int checkOpenrNeighbors() {
  int nbrCount = 0;
  checkWithRetry(
      [&nbrCount]() {
        try {
          std::vector<::openr::thrift::SparkNeighbor> nbrs;
          auto client = getOpenrClient();
          client->sync_getNeighbors(nbrs);
          nbrCount = nbrs.size();
          if (nbrCount >= 1) {
            return true;
          }
        } catch (const std::exception& e) {
          return false;
        }
        return false;
      },
      600, /* retries */
      1s,
      "Cannot connect to OpenR service");

  return nbrCount;
}

void advertiseOpenrRoutes(std::vector<std::string> toAdd) {
  std::vector<openr::thrift::PrefixEntry> prefixes;
  for (const auto& entry : toAdd) {
    openr::thrift::PrefixEntry prefixEntry;
    *prefixEntry.prefix() = openr::toIpPrefix(entry);
    prefixEntry.type() = openr::thrift::PrefixType::LOOPBACK;
    prefixes.push_back(prefixEntry);
  }

  checkWithRetry(
      [&]() {
        try {
          auto client = getOpenrClient();
          client->sync_advertisePrefixes(std::move(prefixes));
        } catch (const std::exception& e) {
          return false;
        }
        return true;
      },
      600, /* retries */
      1s,
      "Cannot connect to OpenR service");
}

class MultiNodeOpenrTest : public MultiNodeTest {
 public:
  void SetUp() override {
    MultiNodeTest::SetUp();
    if (isDUT()) {
      verifyReachability();
    } else {
      advertiseOpenrRoutes({kTestPrefix});
    }
    auto addRoute = [this](auto ip, const auto& nhops) {
      auto updater = sw()->getRouteUpdater();
      updater.addRoute(
          RouterID(0),
          ip,
          0,
          ClientID::BGPD,
          RouteNextHopEntry(nhops, AdminDistance::EBGP));
      updater.program();
    };

    addRoute(folly::IPAddress("0.0.0.0"), makeNextHops(getV4Neighbors()));
    addRoute(folly::IPAddress("::"), makeNextHops(getV6Neighbors()));
  }

 private:
  std::vector<folly::IPAddress> getNeighbors() const {
    std::vector<folly::IPAddress> nbrs;
    auto makeIps = [&nbrs](const std::vector<std::string>& nbrStrs) {
      for (auto& nbr : nbrStrs) {
        nbrs.push_back(folly::IPAddress(nbr));
      }
    };
    if (isDUT()) {
      makeIps(
          {"1::1",
           "2::1",
           "3::1",
           "4::1",
           "1.0.0.1",
           "2.0.0.1",
           "3.0.0.1",
           "4.0.0.1"});
    } else {
      makeIps(
          {"1::3",
           "2::3",
           "3::3",
           "4::3",
           "1.0.0.3",
           "2.0.0.3",
           "3.0.0.3",
           "4.0.0.3"});
    }
    return nbrs;
  }
  template <typename AddrT>
  std::vector<AddrT> getNeighbors() const {
    std::vector<AddrT> addrs;
    for (auto& nbr : getNeighbors()) {
      if (nbr.version() == AddrT().version()) {
        addrs.push_back(AddrT(nbr.str()));
      }
    }
    return addrs;
  }
  std::vector<folly::IPAddressV6> getV6Neighbors() const {
    return getNeighbors<folly::IPAddressV6>();
  }
  std::vector<folly::IPAddressV4> getV4Neighbors() const {
    return getNeighbors<folly::IPAddressV4>();
  }
  void verifyReachability() const {
    // In a cold boot ports can flap initially. Wait for ports to
    // stabilize state
    if (platform()->getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
      sleep(60);
    }
    for (auto dstIp : getNeighbors()) {
      std::string pingCmd = "ping -c 5 ";
      std::string resultStr;
      std::string errStr;
      EXPECT_TRUE(facebook::process::Process::execShellCmd(
          pingCmd + dstIp.str(), &resultStr, &errStr));
    }
  }
  cfg::SwitchConfig initialConfig() const override {
    auto config = utility::onePortPerInterfaceConfig(
        platform()->getHwSwitch(),
        testPorts(),
        utility::kDefaultLoopbackMap(),
        true /*interfaceHasSubnet*/,
        false /*setInterfaceMac*/);

    utility::setDefaultCpuTrafficPolicyConfig(config, platform()->getAsic());
    utility::addCpuQueueConfig(config, platform()->getAsic());

    return config;
  }

 protected:
  template <typename TIpAddress>
  void checkRoute(TIpAddress prefix, uint8_t length, bool exists) {
    WITH_RETRIES({
      const auto& fibContainer =
          sw()->getState()->getFibs()->getNode(RouterID(0));
      auto fib = fibContainer->template getFib<TIpAddress>();
      auto testRoute = fib->getRouteIf({prefix, length});
      if (exists) {
        EXPECT_EVENTUALLY_NE(testRoute, nullptr);
        if (testRoute) {
          EXPECT_TRUE(testRoute->getEntryForClient(ClientID::OPENR));
        }
      } else {
        EXPECT_EVENTUALLY_EQ(testRoute, nullptr);
      }
    });
  }
};

TEST_F(MultiNodeOpenrTest, verifyNeighborDiscovery) {
  auto verify = [&]() {
    checkOpenrInitialization();
    auto nbrCount = checkOpenrNeighbors();
    EXPECT_GE(nbrCount, 1);
  };
  verifyAcrossWarmBoots(verify);
}

TEST_F(MultiNodeOpenrTest, addOpenrRoute) {
  auto setup = [&]() {
    checkOpenrInitialization();
    auto nbrCount = checkOpenrNeighbors();
    EXPECT_GE(nbrCount, 1);
  };
  auto verify = [&]() {
    auto prefix = folly::IPAddress::createNetwork(kTestPrefix);
    checkRoute(prefix.first.asV6(), prefix.second, true);
  };

  verifyAcrossWarmBoots(setup, verify);
}
