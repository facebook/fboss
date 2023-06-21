// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/agent/test/link_tests/LinkTestUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

using namespace ::testing;
using namespace facebook::fboss;

struct TestPort {
  std::string portName;
  phy::PortComponent component;
  prbs::PrbsPolynomial polynomial;
};

class PrbsTest : public LinkTest {
 public:
  bool checkValidMedia(PortID port, MediaInterfaceCode media) {
    auto tcvrSpec = utility::getTransceiverSpec(sw(), port);
    this->platform()->getPlatformPort(port)->getTransceiverSpec();
    if (tcvrSpec) {
      if (auto mediaInterface = tcvrSpec->getMediaInterface()) {
        return *mediaInterface == media;
      }
    }
    return false;
  }

  bool checkPrbsSupported(
      std::string& interfaceName,
      phy::PortComponent component,
      prbs::PrbsPolynomial polynomial) {
    if (component == phy::PortComponent::ASIC) {
      auto agentClient = utils::createWedgeAgentClient();
      return checkPrbsSupportedOnInterface<apache::thrift::Client<FbossCtrl>>(

          agentClient.get(), interfaceName, component, polynomial);
    } else if (
        component == phy::PortComponent::GB_LINE ||
        component == phy::PortComponent::GB_SYSTEM) {
      // TODO: Not supported yet
      return false;
    } else {
      auto qsfpServiceClient = utils::createQsfpServiceClient();
      return checkPrbsSupportedOnInterface<apache::thrift::Client<QsfpService>>(
          qsfpServiceClient.get(), interfaceName, component, polynomial);
    }
    return true;
  }

 protected:
  void SetUp() override {
    LinkTest::SetUp();
    waitForLldpOnCabledPorts();

    // Get the list of ports and their components to enable the test on
    portsToTest_ = getPortsToTest();
    CHECK(!portsToTest_.empty());

    for (auto testPort : portsToTest_) {
      XLOG(DBG2) << "Will run the PRBS "
                 << apache::thrift::util::enumNameSafe(testPort.polynomial)
                 << " test on "
                 << apache::thrift::util::enumNameSafe(testPort.component)
                 << " on " << testPort.portName;
    }
  }

  void runTest() {
    prbs::InterfacePrbsState enabledState;
    enabledState.generatorEnabled() = true;
    enabledState.checkerEnabled() = true;
    // Polynomial is set later in individual functions depending on the port and
    // component being tested

    prbs::InterfacePrbsState disabledState;
    disabledState.generatorEnabled() = false;
    disabledState.checkerEnabled() = false;

    auto timestampBeforeClear = std::time(nullptr);
    /* sleep override */ std::this_thread::sleep_for(1s);
    XLOG(DBG2) << "Clearing PRBS stats before starting the test";
    clearPrbsStatsOnAllInterfaces();

    // 1. Verify the last clear timestamp advanced and num loss of lock was
    // reset to 0
    XLOG(DBG2) << "Verifying PRBS stats are cleared before starting the test";
    checkPrbsStatsAfterClearOnAllInterfaces(
        timestampBeforeClear, false /* prbsEnabled */);

    // 2. Enable PRBS on all Ports
    XLOG(DBG2) << "Enabling PRBS Generator on all ports";
    enabledState.generatorEnabled() = true;
    enabledState.checkerEnabled().reset();
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
      EXPECT_EVENTUALLY_TRUE(setPrbsOnAllInterfaces(enabledState));
    });

    // Certain CMIS optics work more reliably when there is a 10s time between
    // enabling the generator and the checker
    /* sleep override */ std::this_thread::sleep_for(10s);

    XLOG(DBG2) << "Enabling PRBS Checker on all ports";
    enabledState.checkerEnabled() = true;
    enabledState.generatorEnabled().reset();
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
      EXPECT_EVENTUALLY_TRUE(setPrbsOnAllInterfaces(enabledState));
    });

    // 3. Check Prbs State on all ports, they all should be enabled
    XLOG(DBG2) << "Checking PRBS state after enabling PRBS";
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
      EXPECT_EVENTUALLY_TRUE(checkPrbsStateOnAllInterfaces(enabledState));
    });

    // 4. Let PRBS warm up for 30 seconds
    /* sleep override */ std::this_thread::sleep_for(30s);

    // 5. Clear the PRBS stats to clear the instability at PRBS startup
    XLOG(DBG2) << "Clearing PRBS stats before monitoring BER";
    clearPrbsStatsOnAllInterfaces();

    // 6. Let PRBS run for 30 seconds so that we can check the BER later
    /* sleep override */ std::this_thread::sleep_for(30s);

    // 7. Check PRBS stats, expect no loss of lock
    XLOG(DBG2) << "Verifying PRBS stats";
    checkPrbsStatsOnAllInterfaces();

    // 8. Clear PRBS stats
    timestampBeforeClear = std::time(nullptr);
    /* sleep override */ std::this_thread::sleep_for(1s);
    XLOG(DBG2) << "Clearing PRBS stats";
    clearPrbsStatsOnAllInterfaces();

    // 9. Verify the last clear timestamp advanced and that there was no
    // impact on some of the other fields
    /* sleep override */ std::this_thread::sleep_for(20s);
    XLOG(DBG2) << "Verifying PRBS stats after clear";
    checkPrbsStatsAfterClearOnAllInterfaces(
        timestampBeforeClear, true /* prbsEnabled */);

    // 10. Disable PRBS on all Ports
    XLOG(DBG2) << "Disabling PRBS";
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
      EXPECT_EVENTUALLY_TRUE(setPrbsOnAllInterfaces(disabledState));
    });

    // 11. Check Prbs State on all ports, they all should be disabled
    XLOG(DBG2) << "Checking PRBS state after disabling PRBS";
    // Retry for a minute to give the qsfp_service enough chance to
    // successfully refresh a transceiver
    WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
      EXPECT_EVENTUALLY_TRUE(checkPrbsStateOnAllInterfaces(disabledState));
    });

    // 12. Link and traffic should come back up now
    XLOG(DBG2) << "Waiting for links and traffic to come back up";
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    waitForLldpOnCabledPorts();
  }

  virtual std::vector<TestPort> getPortsToTest() = 0;

 private:
  std::vector<TestPort> portsToTest_;

  template <class Client>
  bool setPrbsOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PortComponent component,
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
    for (const auto& testPort : portsToTest_) {
      auto interfaceName = testPort.portName;
      auto component = testPort.component;
      state.polynomial_ref() = testPort.polynomial;
      if (component == phy::PortComponent::ASIC) {
        auto agentClient = utils::createWedgeAgentClient();
        WITH_RETRIES_N_TIMED(6, std::chrono::milliseconds(5000), {
          EXPECT_EVENTUALLY_TRUE(
              setPrbsOnInterface<apache::thrift::Client<FbossCtrl>>(
                  agentClient.get(), interfaceName, component, state));
        });
      } else if (
          component == phy::PortComponent::GB_LINE ||
          component == phy::PortComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return false;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
          EXPECT_EVENTUALLY_TRUE(
              setPrbsOnInterface<apache::thrift::Client<QsfpService>>(
                  qsfpServiceClient.get(), interfaceName, component, state));
        });
      }
    }
    return true;
  }

  bool checkPrbsStateOnAllInterfaces(prbs::InterfacePrbsState& state) {
    for (const auto& testPort : portsToTest_) {
      auto interfaceName = testPort.portName;
      auto component = testPort.component;
      state.polynomial_ref() = testPort.polynomial;
      if (component == phy::PortComponent::ASIC) {
        auto agentClient = utils::createWedgeAgentClient();
        WITH_RETRIES_N_TIMED(6, std::chrono::milliseconds(5000), {
          EXPECT_EVENTUALLY_TRUE(
              checkPrbsStateOnInterface<apache::thrift::Client<FbossCtrl>>(
                  agentClient.get(), interfaceName, component, state));
        });
      } else if (
          component == phy::PortComponent::GB_LINE ||
          component == phy::PortComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return false;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
          EXPECT_EVENTUALLY_TRUE(
              checkPrbsStateOnInterface<apache::thrift::Client<QsfpService>>(
                  qsfpServiceClient.get(), interfaceName, component, state));
        });
      }
    }
    return true;
  }

  template <class Client>
  bool checkPrbsStateOnInterface(
      Client* client,
      std::string interfaceName,
      phy::PortComponent component,
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
    for (const auto& testPort : portsToTest_) {
      auto interfaceName = testPort.portName;
      auto component = testPort.component;
      if (component == phy::PortComponent::ASIC) {
        auto agentClient = utils::createWedgeAgentClient();
        checkPrbsStatsOnInterface<apache::thrift::Client<FbossCtrl>>(
            agentClient.get(), interfaceName, component);
      } else if (
          component == phy::PortComponent::GB_LINE ||
          component == phy::PortComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        checkPrbsStatsOnInterface<apache::thrift::Client<QsfpService>>(
            qsfpServiceClient.get(), interfaceName, component);
      }
    }
  }

  template <class Client>
  void checkPrbsStatsOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PortComponent component) {
    WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
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
    });
  }

  void checkPrbsStatsAfterClearOnAllInterfaces(
      std::time_t timestampBeforeClear,
      bool prbsEnabled) {
    for (const auto& testPort : portsToTest_) {
      auto interfaceName = testPort.portName;
      auto component = testPort.component;
      if (component == phy::PortComponent::ASIC) {
        // Only check agent's prbs stats if prbs is enabled. Agent doesn't
        // return stats when prbs is disabled
        if (prbsEnabled) {
          auto agentClient = utils::createWedgeAgentClient();
          WITH_RETRIES_N_TIMED(6, std::chrono::milliseconds(5000), {
            EXPECT_EVENTUALLY_TRUE(checkPrbsStatsAfterClearOnInterface<
                                   apache::thrift::Client<FbossCtrl>>(
                agentClient.get(),
                timestampBeforeClear,
                interfaceName,
                component,
                prbsEnabled));
          });
        }
      } else if (
          component == phy::PortComponent::GB_LINE ||
          component == phy::PortComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
          EXPECT_EVENTUALLY_TRUE(checkPrbsStatsAfterClearOnInterface<
                                 apache::thrift::Client<QsfpService>>(
              qsfpServiceClient.get(),
              timestampBeforeClear,
              interfaceName,
              component,
              prbsEnabled));
        });
      }
    }
  }

  template <class Client>
  bool checkPrbsStatsAfterClearOnInterface(
      Client* client,
      std::time_t timestampBeforeClear,
      std::string& interfaceName,
      phy::PortComponent component,
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
    for (const auto& testPort : portsToTest_) {
      auto interfaceName = testPort.portName;
      auto component = testPort.component;
      if (component == phy::PortComponent::ASIC) {
        auto agentClient = utils::createWedgeAgentClient();
        WITH_RETRIES_N_TIMED(6, std::chrono::milliseconds(5000), {
          EXPECT_EVENTUALLY_TRUE(
              clearPrbsStatsOnInterface<apache::thrift::Client<FbossCtrl>>(
                  agentClient.get(), interfaceName, component));
        });
      } else if (
          component == phy::PortComponent::GB_LINE ||
          component == phy::PortComponent::GB_SYSTEM) {
        // TODO: Not supported yet
        return;
      } else {
        auto qsfpServiceClient = utils::createQsfpServiceClient();
        // Retry for a minute to give the qsfp_service enough chance to
        // successfully refresh a transceiver
        WITH_RETRIES_N_TIMED(12, std::chrono::milliseconds(5000), {
          EXPECT_EVENTUALLY_TRUE(
              clearPrbsStatsOnInterface<apache::thrift::Client<QsfpService>>(
                  qsfpServiceClient.get(), interfaceName, component));
        });
      }
    }
  }

  template <class Client>
  bool clearPrbsStatsOnInterface(
      Client* client,
      std::string& interfaceName,
      phy::PortComponent component) {
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
      phy::PortComponent component,
      prbs::PrbsPolynomial polynomial) {
    try {
      std::vector<prbs::PrbsPolynomial> prbsCaps;
      client->sync_getSupportedPrbsPolynomials(
          prbsCaps, interfaceName, component);
      return std::find(prbsCaps.begin(), prbsCaps.end(), polynomial) !=
          prbsCaps.end();
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Checking PRBS capabilities on " << interfaceName
                << " failed with " << ex.what();
      return false;
    }
  }
};

template <MediaInterfaceCode Media, prbs::PrbsPolynomial Polynomial>
class TransceiverLineToTransceiverLinePrbsTest : public PrbsTest {
 protected:
  std::vector<TestPort> getPortsToTest() override {
    std::vector<TestPort> portsToTest;
    auto connectedPairs = this->getConnectedPairs();
    for (const auto [port1, port2] : connectedPairs) {
      auto portName1 = this->getPortName(port1);
      auto portName2 = this->getPortName(port2);

      if (!this->checkValidMedia(port1, Media) ||
          !this->checkValidMedia(port2, Media) ||
          !this->checkPrbsSupported(
              portName1, phy::PortComponent::TRANSCEIVER_LINE, Polynomial) ||
          !this->checkPrbsSupported(
              portName2, phy::PortComponent::TRANSCEIVER_LINE, Polynomial)) {
        continue;
      }
      portsToTest.push_back(
          {portName1, phy::PortComponent::TRANSCEIVER_LINE, Polynomial});
      portsToTest.push_back(
          {portName2, phy::PortComponent::TRANSCEIVER_LINE, Polynomial});
    }
    return portsToTest;
  }
};

template <
    MediaInterfaceCode Media,
    prbs::PrbsPolynomial PolynomialA,
    phy::PortComponent ComponentA,
    prbs::PrbsPolynomial PolynomialZ>
class PhyToTransceiverSystemPrbsTest : public PrbsTest {
 protected:
  std::vector<TestPort> getPortsToTest() override {
    CHECK(
        ComponentA == phy::PortComponent::ASIC ||
        ComponentA == phy::PortComponent::GB_LINE);
    std::vector<TestPort> portsToTest;
    auto connectedPairs = this->getConnectedPairs();
    for (const auto [port1, port2] : connectedPairs) {
      for (const auto port : {port1, port2}) {
        auto portName = this->getPortName(port);
        if (!this->checkValidMedia(port, Media) ||
            !this->checkPrbsSupported(portName, ComponentA, PolynomialA) ||
            !this->checkPrbsSupported(
                portName,
                phy::PortComponent::TRANSCEIVER_SYSTEM,
                PolynomialZ)) {
          continue;
        }
        portsToTest.push_back(
            {portName, phy::PortComponent::TRANSCEIVER_SYSTEM, PolynomialZ});
        portsToTest.push_back({portName, ComponentA, PolynomialA});
      }
    }
    return portsToTest;
  }
};

#define PRBS_TEST_NAME(COMPONENT_A, COMPONENT_Z, POLYNOMIAL_A, POLYNOMIAL_Z) \
  BOOST_PP_CAT(                                                              \
      Prbs_,                                                                 \
      BOOST_PP_CAT(                                                          \
          BOOST_PP_CAT(                                                      \
              COMPONENT_A,                                                   \
              BOOST_PP_CAT(                                                  \
                  _,                                                         \
                  BOOST_PP_CAT(                                              \
                      POLYNOMIAL_A, BOOST_PP_CAT(_TO_, COMPONENT_Z)))),      \
          BOOST_PP_CAT(_, POLYNOMIAL_Z)))

#define PRBS_TRANSCEIVER_TEST_NAME(                                         \
    MEDIA, COMPONENT_A, COMPONENT_Z, POLYNOMIAL_A, POLYNOMIAL_Z)            \
  BOOST_PP_CAT(                                                             \
      PRBS_TEST_NAME(COMPONENT_A, COMPONENT_Z, POLYNOMIAL_A, POLYNOMIAL_Z), \
      BOOST_PP_CAT(_, MEDIA))

#define PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(MEDIA, POLYNOMIAL)        \
  struct PRBS_TRANSCEIVER_TEST_NAME(                                          \
      MEDIA, TRANSCEIVER_LINE, TRANSCEIVER_LINE, POLYNOMIAL, POLYNOMIAL)      \
      : public TransceiverLineToTransceiverLinePrbsTest<                      \
            MediaInterfaceCode::MEDIA,                                        \
            prbs::PrbsPolynomial::POLYNOMIAL> {};                             \
  TEST_F(                                                                     \
      PRBS_TRANSCEIVER_TEST_NAME(                                             \
          MEDIA, TRANSCEIVER_LINE, TRANSCEIVER_LINE, POLYNOMIAL, POLYNOMIAL), \
      prbsSanity) {                                                           \
    runTest();                                                                \
  }

#define PRBS_PHY_TRANSCEIVER_SYSTEM_TEST(                                   \
    MEDIA, POLYNOMIALA, COMPONENTA, POLYNOMIALB)                            \
  struct PRBS_TRANSCEIVER_TEST_NAME(                                        \
      MEDIA, COMPONENTA, TRANSCEIVER_SYSTEM, POLYNOMIALA, POLYNOMIALB)      \
      : public PhyToTransceiverSystemPrbsTest<                              \
            MediaInterfaceCode::MEDIA,                                      \
            prbs::PrbsPolynomial::POLYNOMIALA,                              \
            phy::PortComponent::COMPONENTA,                                 \
            prbs::PrbsPolynomial::POLYNOMIALB> {};                          \
  TEST_F(                                                                   \
      PRBS_TRANSCEIVER_TEST_NAME(                                           \
          MEDIA, COMPONENTA, TRANSCEIVER_SYSTEM, POLYNOMIALA, POLYNOMIALB), \
      prbsSanity) {                                                         \
    runTest();                                                              \
  }

PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(FR1_100G, PRBS31);

PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(FR4_200G, PRBS31Q);

PRBS_TRANSCEIVER_LINE_TRANSCEIVER_LINE_TEST(FR4_400G, PRBS31Q);

PRBS_PHY_TRANSCEIVER_SYSTEM_TEST(FR4_200G, PRBS31, ASIC, PRBS31Q);

PRBS_PHY_TRANSCEIVER_SYSTEM_TEST(FR4_400G, PRBS31, ASIC, PRBS31Q);
