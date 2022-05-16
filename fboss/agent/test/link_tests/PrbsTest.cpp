// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

using namespace ::testing;
using namespace facebook::fboss;

template <
    prbs::PrbsPolynomial Polynomial,
    phy::PrbsComponent ComponentA,
    phy::PrbsComponent ComponentZ>
class PrbsTest : public LinkTest {
 public:
  bool checkPrbsSupported(
      std::string& interfaceName,
      phy::PrbsComponent component) {
    if (component == phy::PrbsComponent::ASIC) {
      // TODO: Not supported yet
      return false;
    } else if (
        component == phy::PrbsComponent::GB_LINE ||
        component == phy::PrbsComponent::GB_SYSTEM) {
      // TODO: Not supported yet
      return false;
    } else {
      auto qsfpServiceClient = utils::createQsfpServiceClient();
      checkWithRetry([this,
                      &interfaceName,
                      component,
                      qsfpServiceClient = std::move(qsfpServiceClient)] {
        return checkPrbsSupportedOnInterface<
            facebook::fboss::QsfpServiceAsyncClient>(
            qsfpServiceClient.get(), interfaceName, component);
      });
    }
    return true;
  }

 protected:
  void SetUp() override {
    LinkTest::SetUp();
    checkWithRetry([this] { return lldpNeighborsOnAllCabledPorts(); });

    // Get the list of ports and their components to enable the test on
    portsAndComponentsToTest_ = getPortsToTest();
    CHECK(!portsAndComponentsToTest_.empty());

    for (auto [port, component] : portsAndComponentsToTest_) {
      XLOG(INFO) << "Will run the PRBS test on "
                 << apache::thrift::util::enumNameSafe(component) << " on "
                 << port;
    }
  }

  void runTest() {
    prbs::InterfacePrbsState enabledState;
    enabledState.generatorEnabled() = true;
    enabledState.checkerEnabled() = true;
    enabledState.polynomial() = prbs::PrbsPolynomial(Polynomial);

    prbs::InterfacePrbsState disabledState;
    disabledState.generatorEnabled() = false;
    disabledState.checkerEnabled() = false;
    disabledState.polynomial() = prbs::PrbsPolynomial(Polynomial);

    auto timestampBeforeClear = std::time(nullptr);
    /* sleep override */ std::this_thread::sleep_for(1s);
    XLOG(INFO) << "Clearing PRBS stats before starting the test";
    clearPrbsStatsOnAllInterfaces();

    // 1. Verify the last clear timestamp advanced and num loss of lock was
    // reset to 0
    XLOG(INFO) << "Verifying PRBS stats are cleared before starting the test";
    checkPrbsStatsAfterClearOnAllInterfaces(
        timestampBeforeClear, false /* prbsEnabled */);

    // 2. Enable PRBS on all Ports
    XLOG(INFO) << "Enabling PRBS Generator on all ports";
    enabledState.generatorEnabled() = true;
    enabledState.checkerEnabled().reset();
    checkWithRetry(
        [this, &enabledState] { return setPrbsOnAllInterfaces(enabledState); });

    XLOG(INFO) << "Enabling PRBS Checker on all ports";
    enabledState.checkerEnabled() = true;
    enabledState.generatorEnabled().reset();
    checkWithRetry(
        [this, &enabledState] { return setPrbsOnAllInterfaces(enabledState); });

    // 3. Check Prbs State on all ports, they all should be enabled
    XLOG(INFO) << "Checking PRBS state after enabling PRBS";
    checkWithRetry([this, &enabledState] {
      return checkPrbsStateOnAllInterfaces(enabledState);
    });

    // 4. Let PRBS run for 30 seconds so that we can check the BER later
    /* sleep override */ std::this_thread::sleep_for(30s);

    // 5. Check PRBS stats, expect no loss of lock
    XLOG(INFO) << "Verifying PRBS stats";
    checkPrbsStatsOnAllInterfaces();

    // 6. Clear PRBS stats
    timestampBeforeClear = std::time(nullptr);
    /* sleep override */ std::this_thread::sleep_for(1s);
    XLOG(INFO) << "Clearing PRBS stats";
    clearPrbsStatsOnAllInterfaces();

    // 7. Verify the last clear timestamp advanced and that there was no
    // impact on some of the other fields
    /* sleep override */ std::this_thread::sleep_for(20s);
    XLOG(INFO) << "Verifying PRBS stats after clear";
    checkPrbsStatsAfterClearOnAllInterfaces(
        timestampBeforeClear, true /* prbsEnabled */);

    // 8. Disable PRBS on all Ports
    XLOG(INFO) << "Disabling PRBS";
    checkWithRetry([this, &disabledState] {
      return setPrbsOnAllInterfaces(disabledState);
    });

    // 9. Check Prbs State on all ports, they all should be disabled
    XLOG(INFO) << "Checking PRBS state after disabling PRBS";
    checkWithRetry([this, &disabledState] {
      return checkPrbsStateOnAllInterfaces(disabledState);
    });

    // 10. Link and traffic should come back up now
    XLOG(INFO) << "Waiting for links and traffic to come back up";
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    checkWithRetry([this] { return lldpNeighborsOnAllCabledPorts(); });
  }

  virtual std::vector<std::pair<std::string, phy::PrbsComponent>>
  getPortsToTest() = 0;

 private:
  std::vector<std::pair<std::string, phy::PrbsComponent>>
      portsAndComponentsToTest_;

  template <class Client>
  bool setPrbsOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PrbsComponent component,
      prbs::InterfacePrbsState& state) {
    try {
      client->sync_setInterfacePrbs(interfaceName, component, state);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Setting PRBS on " << interfaceName << " failed with "
                << ex.what();
      return false;
    }
    return true;
  }

  bool setPrbsOnAllInterfaces(prbs::InterfacePrbsState& state) {
    for (const auto& portAndComponentPair : portsAndComponentsToTest_) {
      auto interfaceName = portAndComponentPair.first;
      auto component = portAndComponentPair.second;
      if (component == phy::PrbsComponent::ASIC) {
        // TODO: Not supported yet
        return false;
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return false;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        checkWithRetry([this,
                        &interfaceName,
                        component,
                        &state,
                        qsfpServiceClient = std::move(qsfpServiceClient)] {
          return setPrbsOnInterface<facebook::fboss::QsfpServiceAsyncClient>(
              qsfpServiceClient.get(), interfaceName, component, state);
        });
      }
    }
    return true;
  }

  bool checkPrbsStateOnAllInterfaces(prbs::InterfacePrbsState& state) {
    for (const auto& portAndComponentPair : portsAndComponentsToTest_) {
      auto interfaceName = portAndComponentPair.first;
      auto component = portAndComponentPair.second;
      if (component == phy::PrbsComponent::ASIC) {
        // TODO: Not supported yet
        return false;
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return false;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        checkWithRetry([this,
                        &interfaceName,
                        component,
                        &state,
                        qsfpServiceClient = std::move(qsfpServiceClient)] {
          return checkPrbsStateOnInterface<
              facebook::fboss::QsfpServiceAsyncClient>(
              qsfpServiceClient.get(), interfaceName, component, state);
        });
      }
    }
    return true;
  }

  template <class Client>
  bool checkPrbsStateOnInterface(
      Client* client,
      std::string interfaceName,
      phy::PrbsComponent component,
      prbs::InterfacePrbsState& expectedState) {
    try {
      prbs::InterfacePrbsState state;
      client->sync_getInterfacePrbsState(state, interfaceName, component);
      if (expectedState.generatorEnabled().has_value() &&
          expectedState.generatorEnabled().value()) {
        // Check both enabled state and polynomial when expected state is
        // enabled
        return state.generatorEnabled().has_value() &&
            state.generatorEnabled().value() &&
            (*state.polynomial() == *expectedState.polynomial());
      } else if (
          expectedState.checkerEnabled().has_value() &&
          expectedState.checkerEnabled().value()) {
        // Check the checker enabled state
        return state.checkerEnabled().has_value() &&
            state.checkerEnabled().value() &&
            (*state.polynomial() == *expectedState.polynomial());
      } else {
        // Generator and checker should both be disabled
        return !(
            (state.generatorEnabled().has_value() &&
             state.generatorEnabled().value()) ||
            (state.checkerEnabled().has_value() &&
             state.checkerEnabled().value()));
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Checking PRBS State on " << interfaceName << " failed with "
                << ex.what();
      return false;
    }
  }

  void checkPrbsStatsOnAllInterfaces() {
    for (const auto& portAndComponentPair : portsAndComponentsToTest_) {
      auto interfaceName = portAndComponentPair.first;
      auto component = portAndComponentPair.second;
      if (component == phy::PrbsComponent::ASIC) {
        // TODO: Not supported yet
        return;
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        checkWithRetry([this,
                        &interfaceName,
                        component,
                        qsfpServiceClient = std::move(qsfpServiceClient)] {
          return checkPrbsStatsOnInterface<
              facebook::fboss::QsfpServiceAsyncClient>(
              qsfpServiceClient.get(), interfaceName, component);
        });
      }
    }
  }

  template <class Client>
  bool checkPrbsStatsOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PrbsComponent component) {
    try {
      phy::PrbsStats stats;
      client->sync_getInterfacePrbsStats(stats, interfaceName, component);
      EXPECT_FALSE(stats.get_laneStats().empty());
      for (const auto& laneStat : stats.get_laneStats()) {
        EXPECT_TRUE(laneStat.get_locked());
        EXPECT_FALSE(laneStat.get_numLossOfLock());
        EXPECT_TRUE(laneStat.get_ber() >= 0 && laneStat.get_ber() < 1);
        EXPECT_TRUE(laneStat.get_maxBer() >= 0 && laneStat.get_maxBer() < 1);
        EXPECT_TRUE(laneStat.get_ber() <= laneStat.get_maxBer());
        XLOG(DBG2) << folly::sformat(
            "Interface {:s}, lane: {:d}, locked: {:d}, numLossOfLock: {:d}, ber: {:e}, maxBer: {:e}",
            interfaceName,
            laneStat.get_laneId(),
            laneStat.get_locked(),
            laneStat.get_numLossOfLock(),
            laneStat.get_ber(),
            laneStat.get_maxBer());
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Setting PRBS on " << interfaceName << " failed with "
                << ex.what();
      return false;
    }
    return true;
  }

  void checkPrbsStatsAfterClearOnAllInterfaces(
      std::time_t timestampBeforeClear,
      bool prbsEnabled) {
    for (const auto& portAndComponentPair : portsAndComponentsToTest_) {
      auto interfaceName = portAndComponentPair.first;
      auto component = portAndComponentPair.second;
      if (component == phy::PrbsComponent::ASIC) {
        // TODO: Not supported yet
        return;
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        checkWithRetry([this,
                        timestampBeforeClear,
                        &interfaceName,
                        component,
                        qsfpServiceClient = std::move(qsfpServiceClient),
                        prbsEnabled] {
          return checkPrbsStatsAfterClearOnInterface<
              facebook::fboss::QsfpServiceAsyncClient>(
              qsfpServiceClient.get(),
              timestampBeforeClear,
              interfaceName,
              component,
              prbsEnabled);
        });
      }
    }
  }

  template <class Client>
  bool checkPrbsStatsAfterClearOnInterface(
      Client* client,
      std::time_t timestampBeforeClear,
      std::string& interfaceName,
      phy::PrbsComponent component,
      bool prbsEnabled) {
    try {
      phy::PrbsStats stats;
      client->sync_getInterfacePrbsStats(stats, interfaceName, component);
      EXPECT_FALSE(stats.get_laneStats().empty());
      for (const auto& laneStat : stats.get_laneStats()) {
        EXPECT_EQ(laneStat.get_locked(), prbsEnabled);
        EXPECT_EQ(laneStat.get_numLossOfLock(), 0);
        EXPECT_GT(laneStat.get_timeSinceLastClear(), timestampBeforeClear);
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Setting PRBS on " << interfaceName << " failed with "
                << ex.what();
      return false;
    }
    return true;
  }

  void clearPrbsStatsOnAllInterfaces() {
    for (const auto& portAndComponentPair : portsAndComponentsToTest_) {
      auto interfaceName = portAndComponentPair.first;
      auto component = portAndComponentPair.second;
      if (component == phy::PrbsComponent::ASIC) {
        // TODO: Not supported yet
        return;
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        checkWithRetry([this,
                        &interfaceName,
                        component,
                        qsfpServiceClient = std::move(qsfpServiceClient)] {
          return clearPrbsStatsOnInterface<
              facebook::fboss::QsfpServiceAsyncClient>(
              qsfpServiceClient.get(), interfaceName, component);
        });
      }
    }
  }

  template <class Client>
  bool clearPrbsStatsOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PrbsComponent component) {
    try {
      client->sync_clearInterfacePrbsStats(interfaceName, component);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Setting PRBS on " << interfaceName << " failed with "
                << ex.what();
      return false;
    }
    return true;
  }

  template <class Client>
  bool checkPrbsSupportedOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PrbsComponent component) {
    try {
      std::vector<prbs::PrbsPolynomial> prbsCaps;
      client->sync_getSupportedPrbsPolynomials(
          prbsCaps, interfaceName, component);
      return std::find(prbsCaps.begin(), prbsCaps.end(), Polynomial) !=
          prbsCaps.end();
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Setting PRBS on " << interfaceName << " failed with "
                << ex.what();
      return false;
    }
  }
};

template <
    MediaInterfaceCode Media,
    prbs::PrbsPolynomial Polynomial,
    phy::PrbsComponent ComponentA,
    phy::PrbsComponent ComponentZ>
class TransceiverPrbsTest
    : public PrbsTest<Polynomial, ComponentA, ComponentZ> {
 protected:
  std::vector<std::pair<std::string, phy::PrbsComponent>> getPortsToTest()
      override {
    std::vector<std::pair<std::string, phy::PrbsComponent>> portsToTest;
    if (ComponentA == ComponentZ) {
      // Only possible when the component is the line side of transceiver.
      // For system side, the other component should either be a GB or ASIC
      CHECK(ComponentA == phy::PrbsComponent::TRANSCEIVER_LINE);
      auto connectedPairs = this->getConnectedPairs();
      for (const auto [port1, port2] : connectedPairs) {
        auto portName1 = this->getPortName(port1);
        auto portName2 = this->getPortName(port2);
        auto tcvr1 = this->platform()
                         ->getPlatformPort(port1)
                         ->getTransceiverID()
                         .value();
        auto tcvr2 = this->platform()
                         ->getPlatformPort(port2)
                         ->getTransceiverID()
                         .value();

        auto checkValidMedia = [this](facebook::fboss::TransceiverID tcvrID) {
          if (auto tcvrInfo = this->platform()->getQsfpCache()->getIf(tcvrID)) {
            if (auto mediaInterface = (*tcvrInfo).moduleMediaInterface()) {
              return *mediaInterface == Media;
            }
          }
          return false;
        };
        if (!checkValidMedia(tcvr1) || !checkValidMedia(tcvr2) ||
            !this->checkPrbsSupported(portName1, ComponentA) ||
            !this->checkPrbsSupported(portName2, ComponentZ)) {
          continue;
        }
        portsToTest.push_back({portName1, ComponentA});
        portsToTest.push_back({portName2, ComponentZ});
      }
    }
    return portsToTest;
  }
};

#define PRBS_TEST_NAME(COMPONENT_A, COMPONENT_B, POLYNOMIAL)          \
  BOOST_PP_CAT(                                                       \
      Prbs_,                                                          \
      BOOST_PP_CAT(                                                   \
          BOOST_PP_CAT(COMPONENT_A, BOOST_PP_CAT(_TO_, COMPONENT_B)), \
          BOOST_PP_CAT(_, POLYNOMIAL)))

#define PRBS_TRANSCEIVER_TEST_NAME(                         \
    MEDIA, COMPONENT_A, COMPONENT_B, POLYNOMIAL)            \
  BOOST_PP_CAT(                                             \
      PRBS_TEST_NAME(COMPONENT_A, COMPONENT_B, POLYNOMIAL), \
      BOOST_PP_CAT(_, MEDIA))

#define PRBS_TRANSCEIVER_TEST(MEDIA, COMPONENT_A, COMPONENT_B, POLYNOMIAL)     \
  struct PRBS_TRANSCEIVER_TEST_NAME(                                           \
      MEDIA, COMPONENT_A, COMPONENT_B, POLYNOMIAL)                             \
      : public TransceiverPrbsTest<                                            \
            MediaInterfaceCode::MEDIA,                                         \
            prbs::PrbsPolynomial::POLYNOMIAL,                                  \
            phy::PrbsComponent::COMPONENT_A,                                   \
            phy::PrbsComponent::COMPONENT_B> {};                               \
  TEST_F(                                                                      \
      PRBS_TRANSCEIVER_TEST_NAME(MEDIA, COMPONENT_A, COMPONENT_B, POLYNOMIAL), \
      prbsSanity) {                                                            \
    runTest();                                                                 \
  }

PRBS_TRANSCEIVER_TEST(FR1_100G, TRANSCEIVER_LINE, TRANSCEIVER_LINE, PRBS31);

PRBS_TRANSCEIVER_TEST(FR4_200G, TRANSCEIVER_LINE, TRANSCEIVER_LINE, PRBS31Q);
