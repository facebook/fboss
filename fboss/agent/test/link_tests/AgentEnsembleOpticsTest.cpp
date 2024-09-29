// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"
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
  OpticsThresholdRange fecTailMax;
};

struct OpticsPerformanceMonitoringThresholds {
  OpticsSidePerformanceMonitoringThresholds mediaThresholds;
  OpticsSidePerformanceMonitoringThresholds hostThresholds;
};

// CMIS optics thresholds
struct OpticsPerformanceMonitoringThresholds kCmisOpticsThresholds = {
    .mediaThresholds =
        {
            .pam4eSnr = {FLAGS_link_stress_test ? 20.0 : 19.0, 49.0},
            .pam4Ltp = {33.0, 99.0},
            .preFecBer = {0, FLAGS_link_stress_test ? 5.0e-7 : 2.4e-5},
            .fecTailMax = {0, FLAGS_link_stress_test ? 11.0 : 14.0},
        },
    .hostThresholds =
        {
            .pam4eSnr = {FLAGS_link_stress_test ? 20.0 : 19.0, 49.0},
            .pam4Ltp = {33.0, 99.0},
            .preFecBer = {0, FLAGS_link_stress_test ? 5.0e-7 : 2.4e-5},
            .fecTailMax = {0, FLAGS_link_stress_test ? 11.0 : 14.0},
        },
};

void validateVdm(
    const std::map<int, TransceiverInfo>& transceiverInfos,
    const std::vector<int>& tcvrsToTest) {
  auto validatePerfMon =
      [](const std::string& portName,
         phy::Side side,
         const VdmPerfMonitorPortSideStats& vdmPerfMon,
         OpticsSidePerformanceMonitoringThresholds thresholds) {
        auto& preFecBer = vdmPerfMon.get_datapathBER();
        // Fec tail is not implemented on all modules that support VDM. FEC tail
        // is available starting CMIS 5.0 (2x400G-[D|F]R4)
        auto fecTailMax = vdmPerfMon.fecTailMax().value_or({});
        auto& laneSnr = vdmPerfMon.get_laneSNR();
        auto& lanePam4Ltp = vdmPerfMon.get_lanePam4LTP();

        XLOG(DBG2) << "Validating VDM performance monitoring for " << portName
                   << ", side: " << apache::thrift::util::enumNameSafe(side);
        EXPECT_LE(preFecBer.get_max(), thresholds.preFecBer.maxThreshold)
            << folly::sformat(
                   "PreFecBer Max for {} is {}", portName, preFecBer.get_max());
        EXPECT_LE(fecTailMax, thresholds.fecTailMax.maxThreshold)
            << folly::sformat("FecTail Max for {} is {}", portName, fecTailMax);
        for (auto& [lane, snr] : laneSnr) {
          EXPECT_GE(snr, thresholds.pam4eSnr.minThreshold) << folly::sformat(
              "SNR for lane {} on {} is {}", lane, portName, snr);
        }
        for (auto& [lane, ltp] : lanePam4Ltp) {
          EXPECT_GE(ltp, thresholds.pam4Ltp.minThreshold) << folly::sformat(
              "LTP for lane {} on {} is {}", lane, portName, ltp);
        }
      };

  for (const auto& tcvrId : tcvrsToTest) {
    auto txInfoItr = transceiverInfos.find(tcvrId);
    ASSERT_TRUE(txInfoItr != transceiverInfos.end());
    auto vdmPerfMonitorStats =
        txInfoItr->second.tcvrStats()->vdmPerfMonitorStats();
    ASSERT_TRUE(vdmPerfMonitorStats.has_value());
    auto& mediaStats = vdmPerfMonitorStats->get_mediaPortVdmStats();
    auto& hostStats = vdmPerfMonitorStats->get_hostPortVdmStats();

    auto validateSideStats =
        [&, validatePerfMon](
            const std::map<std::string, VdmPerfMonitorPortSideStats>& sideStat,
            phy::Side side,
            OpticsSidePerformanceMonitoringThresholds threshold) {
          for (auto& [portName, vdmPerfMon] : sideStat) {
            validatePerfMon(portName, side, vdmPerfMon, threshold);
          }
        };
    validateSideStats(
        mediaStats, phy::Side::LINE, kCmisOpticsThresholds.mediaThresholds);
    validateSideStats(
        hostStats, phy::Side::SYSTEM, kCmisOpticsThresholds.hostThresholds);
  }
}

} // namespace

class AgentEnsembleOpticsTest : public AgentEnsembleLinkTest {
 public:
  std::set<std::pair<PortID, PortID>> getConnectedOpticalPortPairs() const {
    // TransceiverFeature::NONE will get us all optical pairs.
    return getConnectedOpticalPortPairWithFeature(
        TransceiverFeature::NONE,
        phy::Side::LINE /* side doesn't matter when feature is None */);
  }
};

TEST_F(AgentEnsembleOpticsTest, verifyTxRxLatches) {
  /*
   * 1. Filter out ports with optics
   * 2. Set ASIC port status to false on A side
   * 3. Expect TX_LOS, TX_LOL on A side, RX_LOL on Z side
   * 4. Set ASIC port status to true on A side
   * 5. Expect TX_LOS, TX_LOL, RX_LOL to be cleared on A and Z sides
   * 6. Repeat steps 2-5 by flipping A and Z sides
   */
  auto opticalPortPairs = getConnectedOpticalPortPairs();
  CHECK(!opticalPortPairs.empty());

  std::set<int32_t> allTcvrIds;
  // Gather list of all transceiverIDs so that we get their corresponding
  // transceiverInfo in one shot
  for (auto [portID1, portID2] : opticalPortPairs) {
    allTcvrIds.insert(int32_t(
        getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(portID1)));
    allTcvrIds.insert(int32_t(
        getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(portID2)));
  }

  auto allTcvrInfos = utility::waitForTransceiverInfo(
      std::vector<int32_t>(allTcvrIds.begin(), allTcvrIds.end()));

  // Cache the host and media lanes for each port because once the ports are
  // disabled, transceiver state machine moves to discovered state and we would
  // lose this information in transceiverInfo
  std::map<std::string, std::vector<int>> cachedHostLanes;
  std::map<std::string, std::vector<int>> cachedMediaLanes;
  for (auto& tcvrInfo : allTcvrInfos) {
    auto& tcvrState = *tcvrInfo.second.tcvrState();
    cachedHostLanes.insert(
        tcvrState.get_portNameToHostLanes().begin(),
        tcvrState.get_portNameToHostLanes().end());
    cachedMediaLanes.insert(
        tcvrState.get_portNameToMediaLanes().begin(),
        tcvrState.get_portNameToMediaLanes().end());
  }

  // Pause remediation because we don't want transceivers to remediate while
  // they are down and then interfere with latches
  auto qsfpServiceClient = utils::createQsfpServiceClient();
  qsfpServiceClient->sync_pauseRemediation(24 * 60 * 60 /* 24hrs */, {});

  auto verifyLatches =
      [this, cachedHostLanes, cachedMediaLanes](
          std::unordered_map<int32_t, std::string>& transceiverIds,
          bool txLatch,
          bool rxLatch) {
        std::vector<int32_t> onlyTcvrIds;
        for (const auto& tcvrId : transceiverIds) {
          onlyTcvrIds.push_back(int32_t(tcvrId.first));
        }
        WITH_RETRIES_N_TIMED(10, std::chrono::seconds(10), {
          auto transceiverInfos = utility::waitForTransceiverInfo(onlyTcvrIds);
          for (const auto& tcvrId : onlyTcvrIds) {
            auto& portName = transceiverIds[TransceiverID(tcvrId)];
            auto tcvrInfoInfoItr = transceiverInfos.find(tcvrId);
            ASSERT_EVENTUALLY_TRUE(tcvrInfoInfoItr != transceiverInfos.end());

            auto& tcvrState = *tcvrInfoInfoItr->second.tcvrState();
            auto mediaInterface = tcvrState.moduleMediaInterface().value_or({});
            auto& hostLanes = cachedHostLanes.at(portName);
            auto& mediaLanes = cachedMediaLanes.at(portName);
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
    std::unordered_map<int32_t, std::string> portsWithTxDown;
    std::unordered_map<int32_t, std::string> portsWithRxDown;
    for (const auto& port : opticalPortPairs) {
      auto tcvrIdA =
          getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(port.first);
      auto portNameA = getPortName(port.first);
      auto tcvrIdZ = getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(
          port.second);
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
TEST_F(AgentEnsembleLinkTest, opticsVdmPerformanceMonitoring) {
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

  std::unordered_set<int32_t> transceiverIdSet;
  for (const auto& port : allTestPorts) {
    auto tcvrId =
        getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(port);
    transceiverIdSet.insert(tcvrId);
  }
  std::vector<int32_t> transceiverIds(
      transceiverIdSet.begin(), transceiverIdSet.end());
  auto transceiverInfos = utility::waitForTransceiverInfo(transceiverIds);

  transceiverInfos = utility::waitForTransceiverInfo(transceiverIds);

  std::time_t startTime = std::time(nullptr);
  // 2. Wait for a VDM interval to begin starting now and a transceiverInfo
  // update to finish after the start of VDM interval. This skips any noise from
  // the initial interval during the time of link up
  WITH_RETRIES_N_TIMED(20, std::chrono::seconds(5), {
    transceiverInfos = utility::waitForTransceiverInfo(transceiverIds);
    auto vdmStat =
        transceiverInfos.begin()->second.tcvrStats()->vdmPerfMonitorStats();
    ASSERT_EVENTUALLY_TRUE(vdmStat.has_value());
    ASSERT_EVENTUALLY_GT(vdmStat->get_intervalStartTime(), startTime);
    ASSERT_EVENTUALLY_GT(
        vdmStat->get_statsCollectionTme(), vdmStat->get_intervalStartTime());
  });

  // 4. validate the VDM Performance Monitoring parameters within the threshold
  int testIterations = FLAGS_link_stress_test ? 60 : 1;
  do {
    validateVdm(transceiverInfos, transceiverIds);
    /* sleep override */ std::this_thread::sleep_for(10s);
    transceiverInfos = utility::waitForTransceiverInfo(transceiverIds);
  } while (testIterations-- && !::testing::Test::HasFailure());
}
