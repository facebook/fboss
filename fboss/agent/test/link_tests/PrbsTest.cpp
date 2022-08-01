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
  bool checkValidMedia(PortID port, MediaInterfaceCode media) {
    auto tcvr =
        this->platform()->getPlatformPort(port)->getTransceiverID().value();
    if (auto tcvrInfo = this->platform()->getQsfpCache()->getIf(tcvr)) {
      if (auto mediaInterface = (*tcvrInfo).moduleMediaInterface()) {
        return *mediaInterface == media;
      }
    }
    return false;
  }

  bool checkPrbsSupported(
      std::string& interfaceName,
      phy::PrbsComponent component) {
    if (component == phy::PrbsComponent::ASIC) {
      auto agentClient = utils::createWedgeAgentClient();
      WITH_RETRIES_N_TIMED(
          {
            EXPECT_EVENTUALLY_TRUE(checkPrbsSupportedOnInterface<
                                   apache::thrift::Client<FbossCtrl>>(
                agentClient.get(), interfaceName, component));
          },
          12,
          std::chrono::milliseconds(5000));
    } else if (
        component == phy::PrbsComponent::GB_LINE ||
        component == phy::PrbsComponent::GB_SYSTEM) {
      // TODO: Not supported yet
      return false;
    } else {
      auto qsfpServiceClient = utils::createQsfpServiceClient();
      // Retry for a minute to give the qsfp_service enough chance to
      // successfully refresh a transceiver
      WITH_RETRIES_N_TIMED(
          {
            EXPECT_EVENTUALLY_TRUE(checkPrbsSupportedOnInterface<
                                   facebook::fboss::QsfpServiceAsyncClient>(
                qsfpServiceClient.get(), interfaceName, component));
          },
          12,
          std::chrono::milliseconds(5000));
    }
    return true;
  }

 protected:
  void SetUp() override {
    LinkTest::SetUp();
    waitForLldpOnCabledPorts();

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
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(
        { EXPECT_EVENTUALLY_TRUE(setPrbsOnAllInterfaces(enabledState)); },
        12,
        std::chrono::milliseconds(5000));

    // Certain CMIS optics work more reliably when there is a 10s time between
    // enabling the generator and the checker
    /* sleep override */ std::this_thread::sleep_for(10s);

    XLOG(INFO) << "Enabling PRBS Checker on all ports";
    enabledState.checkerEnabled() = true;
    enabledState.generatorEnabled().reset();
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(
        { EXPECT_EVENTUALLY_TRUE(setPrbsOnAllInterfaces(enabledState)); },
        12,
        std::chrono::milliseconds(5000));

    // 3. Check Prbs State on all ports, they all should be enabled
    XLOG(INFO) << "Checking PRBS state after enabling PRBS";
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(
        {
          EXPECT_EVENTUALLY_TRUE(checkPrbsStateOnAllInterfaces(enabledState));
        },
        12,
        std::chrono::milliseconds(5000));

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
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(
        { EXPECT_EVENTUALLY_TRUE(setPrbsOnAllInterfaces(disabledState)); },
        12,
        std::chrono::milliseconds(5000));

    // 9. Check Prbs State on all ports, they all should be disabled
    XLOG(INFO) << "Checking PRBS state after disabling PRBS";
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(
        {
          EXPECT_EVENTUALLY_TRUE(checkPrbsStateOnAllInterfaces(disabledState));
        },
        12,
        std::chrono::milliseconds(5000));

    // 10. Link and traffic should come back up now
    XLOG(INFO) << "Waiting for links and traffic to come back up";
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    waitForLldpOnCabledPorts();
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
        auto agentClient = utils::createWedgeAgentClient();
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(
                  setPrbsOnInterface<apache::thrift::Client<FbossCtrl>>(
                      agentClient.get(), interfaceName, component, state));
            },
            12,
            std::chrono::milliseconds(5000));
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return false;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(
                  setPrbsOnInterface<facebook::fboss::QsfpServiceAsyncClient>(
                      qsfpServiceClient.get(),
                      interfaceName,
                      component,
                      state));
            },
            12,
            std::chrono::milliseconds(5000));
      }
    }
    return true;
  }

  bool checkPrbsStateOnAllInterfaces(prbs::InterfacePrbsState& state) {
    for (const auto& portAndComponentPair : portsAndComponentsToTest_) {
      auto interfaceName = portAndComponentPair.first;
      auto component = portAndComponentPair.second;
      if (component == phy::PrbsComponent::ASIC) {
        auto agentClient = utils::createWedgeAgentClient();
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(
                  checkPrbsStateOnInterface<apache::thrift::Client<FbossCtrl>>(
                      agentClient.get(), interfaceName, component, state));
            },
            12,
            std::chrono::milliseconds(5000));
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return false;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(checkPrbsStateOnInterface<
                                     facebook::fboss::QsfpServiceAsyncClient>(
                  qsfpServiceClient.get(), interfaceName, component, state));
            },
            12,
            std::chrono::milliseconds(5000));
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
        auto agentClient = utils::createWedgeAgentClient();
        checkPrbsStatsOnInterface<apache::thrift::Client<FbossCtrl>>(
            agentClient.get(), interfaceName, component);
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        checkPrbsStatsOnInterface<facebook::fboss::QsfpServiceAsyncClient>(
            qsfpServiceClient.get(), interfaceName, component);
      }
    }
  }

  template <class Client>
  void checkPrbsStatsOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PrbsComponent component) {
    WITH_RETRIES_N_TIMED(
        {
          phy::PrbsStats stats;
          client->sync_getInterfacePrbsStats(stats, interfaceName, component);
          ASSERT_EVENTUALLY_FALSE(stats.get_laneStats().empty());
          for (const auto& laneStat : stats.get_laneStats()) {
            EXPECT_EVENTUALLY_TRUE(laneStat.get_locked());
            EXPECT_EVENTUALLY_FALSE(laneStat.get_numLossOfLock());
            EXPECT_EVENTUALLY_TRUE(
                laneStat.get_ber() >= 0 && laneStat.get_ber() < 1);
            EXPECT_EVENTUALLY_TRUE(
                laneStat.get_maxBer() >= 0 && laneStat.get_maxBer() < 1);
            EXPECT_EVENTUALLY_TRUE(laneStat.get_ber() <= laneStat.get_maxBer());
            EXPECT_EVENTUALLY_TRUE(laneStat.get_timeSinceLastLocked());
            XLOG(DBG2) << folly::sformat(
                "Interface {:s}, lane: {:d}, locked: {:d}, numLossOfLock: {:d}, ber: {:e}, maxBer: {:e}, timeSinceLastLock: {:d}",
                interfaceName,
                laneStat.get_laneId(),
                laneStat.get_locked(),
                laneStat.get_numLossOfLock(),
                laneStat.get_ber(),
                laneStat.get_maxBer(),
                laneStat.get_timeSinceLastLocked());
          }
        },
        12,
        std::chrono::milliseconds(5000));
  }

  void checkPrbsStatsAfterClearOnAllInterfaces(
      std::time_t timestampBeforeClear,
      bool prbsEnabled) {
    for (const auto& portAndComponentPair : portsAndComponentsToTest_) {
      auto interfaceName = portAndComponentPair.first;
      auto component = portAndComponentPair.second;
      if (component == phy::PrbsComponent::ASIC) {
        auto agentClient = utils::createWedgeAgentClient();
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(checkPrbsStatsAfterClearOnInterface<
                                     apache::thrift::Client<FbossCtrl>>(
                  agentClient.get(),
                  timestampBeforeClear,
                  interfaceName,
                  component,
                  prbsEnabled));
            },
            12,
            std::chrono::milliseconds(5000));
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(checkPrbsStatsAfterClearOnInterface<
                                     facebook::fboss::QsfpServiceAsyncClient>(
                  qsfpServiceClient.get(),
                  timestampBeforeClear,
                  interfaceName,
                  component,
                  prbsEnabled));
            },
            12,
            std::chrono::milliseconds(5000));
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
        EXPECT_LT(laneStat.get_timeSinceLastClear(), timestampBeforeClear);
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
        auto agentClient = utils::createWedgeAgentClient();
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(
                  clearPrbsStatsOnInterface<apache::thrift::Client<FbossCtrl>>(
                      agentClient.get(), interfaceName, component));
            },
            12,
            std::chrono::milliseconds(5000));
      } else if (
          component == phy::PrbsComponent::GB_LINE ||
          component == phy::PrbsComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(
            {
              EXPECT_EVENTUALLY_TRUE(clearPrbsStatsOnInterface<
                                     facebook::fboss::QsfpServiceAsyncClient>(
                  qsfpServiceClient.get(), interfaceName, component));
            },
            12,
            std::chrono::milliseconds(5000));
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

template <MediaInterfaceCode Media, prbs::PrbsPolynomial Polynomial>
class TransceiverLineToTransceiverLinePrbsTest
    : public PrbsTest<
          Polynomial,
          phy::PrbsComponent::TRANSCEIVER_LINE,
          phy::PrbsComponent::TRANSCEIVER_LINE> {
 protected:
  std::vector<std::pair<std::string, phy::PrbsComponent>> getPortsToTest()
      override {
    std::vector<std::pair<std::string, phy::PrbsComponent>> portsToTest;
    auto connectedPairs = this->getConnectedPairs();
    for (const auto [port1, port2] : connectedPairs) {
      auto portName1 = this->getPortName(port1);
      auto portName2 = this->getPortName(port2);

      if (!this->checkValidMedia(port1, Media) ||
          !this->checkValidMedia(port2, Media) ||
          !this->checkPrbsSupported(
              portName1, phy::PrbsComponent::TRANSCEIVER_LINE) ||
          !this->checkPrbsSupported(
              portName2, phy::PrbsComponent::TRANSCEIVER_LINE)) {
        continue;
      }
      portsToTest.push_back({portName1, phy::PrbsComponent::TRANSCEIVER_LINE});
      portsToTest.push_back({portName2, phy::PrbsComponent::TRANSCEIVER_LINE});
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

#define PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(MEDIA, POLYNOMIAL) \
  struct PRBS_TRANSCEIVER_TEST_NAME(                                   \
      MEDIA, TRANSCEIVER_LINE, TRANSCEIVER_LINE, POLYNOMIAL)           \
      : public TransceiverLineToTransceiverLinePrbsTest<               \
            MediaInterfaceCode::MEDIA,                                 \
            prbs::PrbsPolynomial::POLYNOMIAL> {};                      \
  TEST_F(                                                              \
      PRBS_TRANSCEIVER_TEST_NAME(                                      \
          MEDIA, TRANSCEIVER_LINE, TRANSCEIVER_LINE, POLYNOMIAL),      \
      prbsSanity) {                                                    \
    runTest();                                                         \
  }

PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(FR1_100G, PRBS31);

PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(FR4_200G, PRBS31Q);

PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(FR4_400G, PRBS31Q);
