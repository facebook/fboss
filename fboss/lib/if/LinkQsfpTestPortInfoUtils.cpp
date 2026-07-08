// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/lib/if/LinkQsfpTestPortInfoUtils.h"

#include "fboss/lib/if/gen-cpp2/link_qsfp_test_port_info_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss::utility {

void populateTransceiverInfoFields(
    LinkQsfpTestPortInfo& portInfo,
    const TcvrState& tcvrState,
    const std::string& portName) {
  if (tcvrState.vendor().has_value()) {
    const auto& vendor = *tcvrState.vendor();
    portInfo.vendorName() = *vendor.name();
    portInfo.vendorPartNumber() = *vendor.partNumber();
    portInfo.vendorSerialNumber() = *vendor.serialNumber();
  }
  if (tcvrState.status().has_value() &&
      tcvrState.status()->fwStatus().has_value()) {
    const auto& fwStatus = *tcvrState.status()->fwStatus();
    if (fwStatus.version().has_value()) {
      portInfo.fwVersion() = *fwStatus.version();
    }
    if (fwStatus.dspFwVer().has_value()) {
      portInfo.dspFwVersion() = *fwStatus.dspFwVer();
    }
  }
  if (tcvrState.moduleMediaInterface().has_value()) {
    portInfo.moduleMediaInterface() = *tcvrState.moduleMediaInterface();
  }
  // Per-port media interface: look up a media lane used by this port and index
  // into the transceiver's per-lane mediaInterface settings.
  if (!portName.empty()) {
    const auto& portToMediaLanes = *tcvrState.portNameToMediaLanes();
    auto laneIt = portToMediaLanes.find(portName);
    if (laneIt != portToMediaLanes.end() && !laneIt->second.empty() &&
        tcvrState.settings().has_value() &&
        tcvrState.settings()->mediaInterface().has_value()) {
      auto lane = laneIt->second.front();
      for (const auto& mediaIf : *tcvrState.settings()->mediaInterface()) {
        if (*mediaIf.lane() == lane) {
          portInfo.portMediaInterface() = *mediaIf.code();
          break;
        }
      }
    }
  }
}

} // namespace facebook::fboss::utility
