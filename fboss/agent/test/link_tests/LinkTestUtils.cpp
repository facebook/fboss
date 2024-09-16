// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include <folly/Subprocess.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/test/link_tests/LinkTestUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

DEFINE_bool(
    link_stress_test,
    false,
    "enable to run stress tests (longer duration + more iterations)");

namespace facebook::fboss::utility {
const std::vector<std::string> kRestartQsfpService = {
    "/bin/systemctl",
    "restart",
    "qsfp_service_for_testing"};

const std::string kForceColdbootQsfpSvcFileName =
    "/dev/shm/fboss/qsfp_service/cold_boot_once_qsfp_service";

// This file stores all non-validated transceiver configurations found while
// running getAllTransceiverConfigValidationStatuses() in EmptyLinkTest. Each
// configuration will be in JSON format. This file is then downloaded by the
// Netcastle test runner to parse it and log these configurations to Scuba.
// Matching file definition is located in
// fbcode/neteng/netcastle/teams/fboss/constants.py.
const std::string kTransceiverConfigJsonsForScuba =
    "/tmp/transceiver_config_jsons_for_scuba.log";

void restartQsfpService(bool coldboot) {
  if (coldboot) {
    createFile(kForceColdbootQsfpSvcFileName);
    XLOG(DBG2) << "Restarting QSFP Service in coldboot mode";
  } else {
    XLOG(DBG2) << "Restarting QSFP Service in warmboot mode";
  }

  folly::Subprocess(kRestartQsfpService).waitChecked();
}

// Retrieves the config validation status for all active transceivers. Strings
// are retrieved instead of booleans because these contain the stringified JSONs
// of each non-validated transceiver config, which will be printed to stdout and
// ingested by Netcastle.
void getAllTransceiverConfigValidationStatuses(
    const std::set<TransceiverID>& cabledTransceivers) {
  std::vector<int32_t> expectedTransceivers(
      cabledTransceivers.begin(), cabledTransceivers.end());
  std::map<int32_t, std::string> responses;

  try {
    auto qsfpServiceClient = utils::createQsfpServiceClient();
    qsfpServiceClient->sync_getTransceiverConfigValidationInfo(
        responses, expectedTransceivers, true);
  } catch (const std::exception& ex) {
    XLOG(WARN)
        << "Failed to call qsfp_service getTransceiverConfigValidationInfo(). "
        << folly::exceptionStr(ex);
  }

  createFile(kTransceiverConfigJsonsForScuba);

  std::vector<int32_t> missingTransceivers, invalidTransceivers;
  std::vector<std::string> nonValidatedConfigs;
  for (auto transceiverID : expectedTransceivers) {
    if (auto transceiverStatusIt = responses.find(transceiverID);
        transceiverStatusIt != responses.end()) {
      if (transceiverStatusIt->second != "") {
        invalidTransceivers.push_back(transceiverID);
        nonValidatedConfigs.push_back(transceiverStatusIt->second);
      }
      continue;
    }
    missingTransceivers.push_back(transceiverID);
  }
  if (nonValidatedConfigs.size()) {
    writeSysfs(
        kTransceiverConfigJsonsForScuba,
        folly::join("\n", nonValidatedConfigs));
  }
  if (!missingTransceivers.empty()) {
    XLOG(DBG2) << "Transceivers [" << folly::join(",", missingTransceivers)
               << "] are missing config validation status.";
  }
  if (!invalidTransceivers.empty()) {
    XLOG(DBG2) << "Transceivers [" << folly::join(",", invalidTransceivers)
               << "] have non-validated configurations.";
  }
  if (missingTransceivers.empty() && invalidTransceivers.empty()) {
    XLOG(DBG2) << "Transceivers [" << folly::join(",", expectedTransceivers)
               << "] all have validated configurations.";
  }
}

void waitForAllTransceiverStates(
    bool up,
    const std::set<TransceiverID>& cabledTransceivers,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) {
  waitForStateMachineState(
      cabledTransceivers,
      up ? TransceiverStateMachineState::ACTIVE
         : TransceiverStateMachineState::INACTIVE,
      retries,
      msBetweenRetry);
}

void waitForStateMachineState(
    const std::set<TransceiverID>& transceiversToCheck,
    TransceiverStateMachineState stateMachineState,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) {
  XLOG(DBG2) << "Checking qsfp TransceiverStateMachineState on "
             << folly::join(",", transceiversToCheck);

  std::vector<int32_t> expectedTransceiver;
  for (auto transceiverID : transceiversToCheck) {
    expectedTransceiver.push_back(transceiverID);
  }

  std::vector<int32_t> badTransceivers;
  while (retries--) {
    badTransceivers.clear();
    std::map<int32_t, TransceiverInfo> info;
    try {
      auto qsfpServiceClient = utils::createQsfpServiceClient();
      qsfpServiceClient->sync_getTransceiverInfo(info, expectedTransceiver);
    } catch (const std::exception& ex) {
      // We have retry mechanism to handle failure. No crash here
      XLOG(WARN) << "Failed to call qsfp_service getTransceiverInfo(). "
                 << folly::exceptionStr(ex);
    }
    // Check whether all expected transceivers have expected state
    for (auto transceiverID : expectedTransceiver) {
      // Only continue if the transceiver state machine matches
      if (auto transceiverInfoIt = info.find(transceiverID);
          transceiverInfoIt != info.end()) {
        if (auto state =
                transceiverInfoIt->second.tcvrState()->stateMachineState();
            state.has_value() && *state == stateMachineState) {
          continue;
        }
      }
      // Otherwise such transceiver is considered to be in a bad state
      badTransceivers.push_back(transceiverID);
    }

    if (badTransceivers.empty()) {
      XLOG(DBG2) << "All qsfp TransceiverStateMachineState on "
                 << folly::join(",", expectedTransceiver) << " match "
                 << apache::thrift::util::enumNameSafe(stateMachineState);
      return;
    } else {
      /* sleep override */
      std::this_thread::sleep_for(msBetweenRetry);
    }
  }

  throw FbossError(
      "Transceivers:[",
      folly::join(",", badTransceivers),
      "] don't have expected TransceiverStateMachineState:",
      apache::thrift::util::enumNameSafe(stateMachineState));
}

// Wait until we have successfully fetched transceiver info (and thus know
// which transceivers are available for testing)
std::map<int32_t, TransceiverInfo> waitForTransceiverInfo(
    std::vector<int32_t> transceiverIds,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) {
  std::map<int32_t, TransceiverInfo> info;
  while (retries--) {
    try {
      auto qsfpServiceClient = utils::createQsfpServiceClient();
      qsfpServiceClient->sync_getTransceiverInfo(info, transceiverIds);
    } catch (const std::exception& ex) {
      XLOG(WARN) << "Failed to call qsfp_service getTransceiverInfo(). "
                 << folly::exceptionStr(ex);
    }
    // Make sure we have at least one present transceiver
    for (const auto& it : info) {
      if (*it.second.tcvrState()->present()) {
        return info;
      }
    }
    XLOG(DBG2) << "TransceiverInfo was empty";
    if (retries) {
      /* sleep override */
      std::this_thread::sleep_for(msBetweenRetry);
    }
  }

  throw FbossError("TransceiverInfo was never populated.");
}

const TransceiverSpec* getTransceiverSpec(const SwSwitch* sw, PortID portId) {
  auto& platformPort = sw->getPlatformMapping()->getPlatformPort(portId);
  const auto& chips = sw->getPlatformMapping()->getChips();
  if (auto tcvrID = utility::getTransceiverId(platformPort, chips)) {
    auto transceiver = sw->getState()->getTransceivers()->getNodeIf(*tcvrID);
    return transceiver.get();
  }
  return nullptr;
}

// This should only be called by platforms that actually have
// an external phy
std::optional<int32_t> getPortExternalPhyID(const SwSwitch* sw, PortID port) {
  auto platformMapping = sw->getPlatformMapping();
  const auto& platformPortEntry = platformMapping->getPlatformPort(port);
  const auto& chips = platformMapping->getChips();
  if (chips.empty()) {
    throw FbossError("Not platform data plane phy chips");
  }

  const auto& xphy = utility::getDataPlanePhyChips(
      platformPortEntry, chips, phy::DataPlanePhyChipType::XPHY);
  if (xphy.empty()) {
    return std::nullopt;
  } else {
    // One port should only has one xphy id
    CHECK_EQ(xphy.size(), 1);
    return *xphy.begin()->second.physicalID();
  }
}
} // namespace facebook::fboss::utility
