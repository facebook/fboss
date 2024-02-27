// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/agent/test/link_tests/LinkTestUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

using namespace ::testing;
using namespace facebook::fboss;

namespace {

struct OpticsThresholdRange {
  double minThreshold;
  double maxThreshold;
};

struct OpticsSidePerformanceMonitoringThresholds {
  OpticsThresholdRange pam4eSnr;
  OpticsThresholdRange pam4Ltp;
  OpticsThresholdRange preFecBer;
};

struct OpticsPerformanceMonitoringThresholds {
  OpticsSidePerformanceMonitoringThresholds mediaThresholds;
  OpticsSidePerformanceMonitoringThresholds hostThresholds;
};

// CMIS optics thresholds
struct OpticsPerformanceMonitoringThresholds kCmisOpticsThresholds = {
    .mediaThresholds =
        {
            .pam4eSnr = {19.0, 49.0},
            .pam4Ltp = {33.0, 99.0},
            .preFecBer = {0, 2.4e-5},
        },
    .hostThresholds =
        {
            .pam4eSnr = {19.0, 49.0},
            .pam4Ltp = {33.0, 99.0},
            .preFecBer = {0, 2.4e-5},
        },
};

} // namespace

class OpticsTest : public LinkTest {
 public:
  std::set<std::pair<PortID, PortID>> getConnectedOpticalPortPairs() const {
    // TransceiverFeature::NONE will get us all optical pairs.
    return getConnectedOpticalPortPairWithFeature(
        TransceiverFeature::NONE,
        phy::Side::LINE /* side doesn't matter when feature is None */);
  }
};

TEST_F(OpticsTest, verifyTxRxLatches) {
  /*
   * 1. Filter out ports with optics
   * 2. Set ASIC port status to false on A side
   * 3. Expect TX_LOS, TX_LOL on A side, RX_LOL on Z side
   * 4. Set ASIC port status to true on A side
   * 5. Expect TX_LOS, TX_LOL, RX_LOL to be cleared on A and Z sides
   * 6. Repeat steps 2-5 by flipping A and Z sides
   */
  auto opticalPortPairs = getConnectedOpticalPortPairs();
  EXPECT_FALSE(opticalPortPairs.empty())
      << "Did not detect any optical transceivers";

  // Pause remediation because we don't want transceivers to remediate while
  // they are down and then interfere with latches
  auto qsfpServiceClient = utils::createQsfpServiceClient();
  qsfpServiceClient->sync_pauseRemediation(24 * 60 * 60 /* 24hrs */, {});

  auto verifyLatches =
      [this](
          std::unordered_map<TransceiverID, std::string>& transceiverIds,
          bool txLatch,
          bool rxLatch) {
        std::vector<int32_t> onlyTcvrIds;
        for (const auto& tcvrId : transceiverIds) {
          onlyTcvrIds.push_back(int32_t(tcvrId.first));
        }
        WITH_RETRIES_N_TIMED(10, std::chrono::seconds(10), {
          auto transceiverInfos = waitForTransceiverInfo(onlyTcvrIds);
          for (const auto& tcvrId : onlyTcvrIds) {
            auto& portName = transceiverIds[TransceiverID(tcvrId)];
            auto tcvrInfoInfoItr = transceiverInfos.find(tcvrId);
            ASSERT_EVENTUALLY_TRUE(tcvrInfoInfoItr != transceiverInfos.end());

            auto& tcvrState = *tcvrInfoInfoItr->second.tcvrState();
            auto mediaInterface = tcvrState.moduleMediaInterface().value_or({});
            auto& hostLanes = tcvrState.portNameToHostLanes()->at(portName);
            auto& mediaLanes = tcvrState.portNameToMediaLanes()->at(portName);
            auto& hostLaneSignals = *tcvrState.hostLaneSignals();
            auto& mediaLaneSignals = *tcvrState.mediaLaneSignals();

            ASSERT_EVENTUALLY_GT(hostLanes.size(), 0);
            ASSERT_EVENTUALLY_GT(mediaLanes.size(), 0);
            ASSERT_EVENTUALLY_GT(hostLaneSignals.size(), 0);
            ASSERT_EVENTUALLY_GT(mediaLaneSignals.size(), 0);

            for (const auto& signal : hostLaneSignals) {
              if (std::find(
                      hostLanes.begin(), hostLanes.end(), signal.get_lane()) !=
                  hostLanes.end()) {
                ASSERT_EVENTUALLY_TRUE(signal.txLol().has_value());
                ASSERT_EVENTUALLY_TRUE(signal.txLos().has_value());
                // TX_LOL is not reliable right now on certain 100G
                // CWDM4 optics like AOI and Miniphoton. So skip checking it
                // on these optics for now
                if (mediaInterface != MediaInterfaceCode::CWDM4_100G) {
                  EXPECT_EVENTUALLY_EQ(signal.txLol().value(), txLatch)
                      << portName << ", lane: " << signal.get_lane();
                }
                // We see TX_LOS set only on the first lane of FR1_100G. This is
                // a bug but we can't get the vendor to fix it now. Therefore,
                // handle it separately in the test
                if (mediaInterface != MediaInterfaceCode::FR1_100G ||
                    signal.get_lane() == 0) {
                  EXPECT_EVENTUALLY_EQ(signal.txLos().value(), txLatch)
                      << portName << ", lane: " << signal.get_lane();
                }
              }
            }

            for (const auto& signal : mediaLaneSignals) {
              if (std::find(
                      mediaLanes.begin(),
                      mediaLanes.end(),
                      signal.get_lane()) != mediaLanes.end()) {
                // Unfortunately, can't rely on rxLos as it doesn't always get
                // set. Some optics don't squelch their line side when the
                // system side is down.
                ASSERT_EVENTUALLY_TRUE(signal.rxLol().has_value());

                // RX_LOL is not reliable right now on certain 100G
                // CWDM4 optics like AOI and Miniphoton. So skip checking it
                // on these optics for now
                if (mediaInterface != MediaInterfaceCode::CWDM4_100G) {
                  EXPECT_EVENTUALLY_EQ(signal.rxLol().value(), rxLatch)
                      << portName << ", lane: " << signal.get_lane();
                }
              }
            }
          }
        });
      };

  auto verify = [this, opticalPortPairs, verifyLatches](bool disableAPort) {
    std::unordered_map<TransceiverID, std::string> portsWithTxDown;
    std::unordered_map<TransceiverID, std::string> portsWithRxDown;
    for (const auto& port : opticalPortPairs) {
      auto tcvrIdA =
          platform()->getPlatformPort(port.first)->getTransceiverID().value();
      auto portNameA = getPortName(port.first);
      auto tcvrIdZ =
          platform()->getPlatformPort(port.second)->getTransceiverID().value();
      auto portNameZ = getPortName(port.second);
      if (disableAPort) {
        XLOG(INFO) << "Disabling port " << portNameA << ", tcvrId " << tcvrIdA;
        setPortStatus(port.first, false);
        portsWithTxDown.insert({tcvrIdA, portNameA});
        portsWithRxDown.insert({tcvrIdZ, portNameZ});
      } else {
        XLOG(INFO) << "Disabling port " << portNameZ << ", tcvrId " << tcvrIdZ;
        setPortStatus(port.second, false);
        portsWithTxDown.insert({tcvrIdZ, portNameZ});
        portsWithRxDown.insert({tcvrIdA, portNameA});
      }
    }
    EXPECT_NO_THROW(verifyLatches(
        portsWithTxDown, true /* txLatch */, false /* rxLatch */));
    EXPECT_NO_THROW(verifyLatches(
        portsWithRxDown, false /* txLatch */, true /* rxLatch */));

    // Set the port status on all cabled ports to true. The link should come
    // back up
    for (const auto& port : opticalPortPairs) {
      setPortStatus(port.first, true);
      setPortStatus(port.second, true);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    EXPECT_NO_THROW(verifyLatches(
        portsWithTxDown, false /* txLatch */, false /* rxLatch */));
    EXPECT_NO_THROW(verifyLatches(
        portsWithRxDown, false /* txLatch */, false /* rxLatch */));
  };

  XLOG(INFO) << "Testing TX_LOS, TX_LOL on A side, RX_LOS, RX_LOL on Z side";
  verify(true /* disableAPort */);

  XLOG(INFO) << "Testing TX_LOS, TX_LOL on Z side, RX_LOS, RX_LOL on A side";
  verify(false /* disableAPort */);
}

/*
 * opticsVdmPerformanceMonitoring
 *
 * Check VDM parameters are within the threshold for VDM supported optics
 * Steps:
 * 1. Find the list of optical ports with VDM supported optics
 * 2. Wait till we have data for 20 seconds
 * 3. Get the TransceiverInfo from qsfp_service
 * 4. validate the VDM Performance Monitoring parameters within the thresholds
 *    defined in spec
 */
TEST_F(LinkTest, opticsVdmPerformanceMonitoring) {
  // 1. Find the list of optical ports with VDM supported optics
  auto connectedPairPortIds = getConnectedOpticalPortPairWithFeature(
      TransceiverFeature::VDM, phy::Side::LINE);
  CHECK(!connectedPairPortIds.empty())
      << "opticsVdmPerformanceMonitoring: No optics capable of this test";

  std::vector<PortID> allTestPorts;
  for (auto portPair : connectedPairPortIds) {
    allTestPorts.push_back(portPair.first);
    allTestPorts.push_back(portPair.second);
  }

  // 2. Wait till we have data for 20 seconds
  // The qsfp_service cache refresh may not happen at exactly in 20 seconds so
  // we read the TransceiverInfo time and then wait till we get 20 second later
  // data
  std::vector<int32_t> transceiverIds;
  for (const auto& port : allTestPorts) {
    auto tcvrId = platform()->getPlatformPort(port)->getTransceiverID().value();
    transceiverIds.push_back(tcvrId);
  }
  auto transceiverInfos = waitForTransceiverInfo(transceiverIds);
  auto startTime =
      transceiverInfos.begin()->second.tcvrStats()->timeCollected().value();

  transceiverInfos = waitForTransceiverInfo(transceiverIds);

  WITH_RETRIES_N_TIMED(10, std::chrono::seconds(5), {
    transceiverInfos = waitForTransceiverInfo(transceiverIds);
    auto endTime =
        transceiverInfos.begin()->second.tcvrStats()->timeCollected().value();
    ASSERT_EVENTUALLY_GT(endTime, startTime + 20);
  });

  // 3. Get the TransceiverInfo from qsfp_service
  XLOG(DBG2)
      << "opticsVdmPerformanceMonitoring: Got TransceiverInfo 20sec data from qsfp_service";

  // 4. validate the VDM Performance Monitoring parameters within the threshold
  for (const auto& tcvrId : transceiverIds) {
    auto txInfoItr = transceiverInfos.find(tcvrId);
    if (txInfoItr != transceiverInfos.end()) {
      EXPECT_TRUE(txInfoItr->second.tcvrStats()->vdmDiagsStats().has_value());
      XLOG(DBG2) << "Tcvr Id " << tcvrId
                 << " Checking for Line/Host BER, Line SNR";

      auto preFecBerMediaMax = txInfoItr->second.tcvrStats()
                                   ->vdmDiagsStats()
                                   .value()
                                   .preFecBerMediaMax()
                                   .value();
      EXPECT_LE(
          preFecBerMediaMax,
          kCmisOpticsThresholds.mediaThresholds.preFecBer.maxThreshold);

      auto preFecBerHostMax = txInfoItr->second.tcvrStats()
                                  ->vdmDiagsStats()
                                  .value()
                                  .preFecBerHostMax()
                                  .value();
      EXPECT_LE(
          preFecBerHostMax,
          kCmisOpticsThresholds.hostThresholds.preFecBer.maxThreshold);

      auto& snrMediaPerChannel = txInfoItr->second.tcvrStats()
                                     ->vdmDiagsStats()
                                     .value()
                                     .eSnrMediaChannel()
                                     .value();
      for (auto& [channel, channelSnr] : snrMediaPerChannel) {
        EXPECT_GE(
            channelSnr,
            kCmisOpticsThresholds.mediaThresholds.pam4eSnr.minThreshold);
      }
      auto& ltpMediaPerChannel = txInfoItr->second.tcvrStats()
                                     ->vdmDiagsStats()
                                     .value()
                                     .pam4LtpMediaChannel()
                                     .value();
      for (auto& [channel, channelLtp] : ltpMediaPerChannel) {
        EXPECT_GE(
            channelLtp,
            kCmisOpticsThresholds.mediaThresholds.pam4Ltp.minThreshold);
      }
    }
  }
}
