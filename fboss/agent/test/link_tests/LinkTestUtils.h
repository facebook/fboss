// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types_custom_protocol.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::utility {
DECLARE_string(oob_asset);
DECLARE_string(oob_flash_device_name);
DECLARE_string(openbmc_password);
DECLARE_bool(enable_lldp);
DECLARE_bool(tun_intf);

void restartQsfpService(bool coldboot);
void getAllTransceiverConfigValidationStatuses(
    const std::set<TransceiverID>& cabledTransceivers);
void waitForAllTransceiverStates(
    bool enabled,
    const std::set<TransceiverID>& cabledTransceivers,
    uint32_t retries = 60,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
        std::chrono::milliseconds(1000));
void waitForStateMachineState(
    const std::set<TransceiverID>& transceiversToCheck,
    TransceiverStateMachineState stateMachineState,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry);
void includeLpoTransceivers(
    std::map<int32_t, TransceiverInfo>& infos,
    bool includeLpo);
void waitForPortStateMachineState(
    bool enabled,
    const std::vector<PortID>& portsToCheck,
    uint32_t retries = 60,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
        std::chrono::milliseconds(1000));
std::map<int32_t, TransceiverInfo> waitForTransceiverInfo(
    std::vector<int32_t> transceiverIds,
    bool includeLpo = true,
    uint32_t retries = 2,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::seconds(10)));
const TransceiverSpec* getTransceiverSpec(const SwSwitch* sw, PortID portId);
std::optional<int32_t> getPortExternalPhyID(const SwSwitch* sw, PortID port);
std::map<std::string, MediaInterfaceCode> getPortToMediaInterface();
} // namespace facebook::fboss::utility
