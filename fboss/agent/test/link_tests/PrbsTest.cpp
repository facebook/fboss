// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/phy/PhyInterfaceHandler.h"
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
    // 1. Enable PRBS on all Ports
    // 2. Check Prbs State on all ports, they all should be enabled
    // 3. Let PRBS run for 30 seconds so that we can check the BER later
    // 4. Check PRBS stats, expect no loss of lock
    // 5. Clear PRBS stats
    // 6. Verify the last clear timestamp advanced and that there was no
    // impact on some of the other fields
    // 7. Disable PRBS on all Ports
    // 8. Check Prbs State on all ports, they all should be disabled
    // 9. Link and traffic should come back up now
  }

  virtual std::vector<std::pair<std::string, phy::PrbsComponent>>
  getPortsToTest() = 0;

 private:
  std::vector<std::pair<std::string, phy::PrbsComponent>>
      portsAndComponentsToTest_;
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
