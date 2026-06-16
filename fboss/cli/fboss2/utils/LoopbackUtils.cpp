// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/LoopbackUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <fmt/format.h>
#include <folly/MapUtil.h>

namespace facebook::fboss::loopback_utils {

std::map<int32_t, PortInfoThrift> fetchAllPortInfo(
    apache::thrift::Client<FbossCtrl>* agent) {
  std::map<int32_t, PortInfoThrift> portEntries;
  agent->sync_getAllPortInfo(portEntries);
  return portEntries;
}

int32_t resolveTransceiverId(
    apache::thrift::Client<FbossCtrl>* agent,
    const std::string& portName,
    const std::map<int32_t, PortInfoThrift>& portEntries) {
  int32_t portId = -1;
  for (const auto& [id, info] : portEntries) {
    if (*info.name() == portName) {
      portId = id;
      break;
    }
  }
  if (portId < 0) {
    throw FbossError("Port '", portName, "' not found");
  }

  std::map<int32_t, PortStatus> portStatusEntries;
  std::vector<int32_t> portIds = {portId};
  agent->sync_getPortStatus(portStatusEntries, portIds);

  auto* statusPtr = folly::get_ptr(portStatusEntries, portId);
  if (!statusPtr || !statusPtr->transceiverIdx().has_value() ||
      !statusPtr->transceiverIdx()->transceiverId().has_value()) {
    throw FbossError("No transceiver found for port '", portName, "'");
  }
  return *statusPtr->transceiverIdx()->transceiverId();
}

LoopbackCapability fetchLoopbackCapability(
    apache::thrift::Client<QsfpService>* qsfpService,
    int32_t transceiverId) {
  std::map<int32_t, TransceiverInfo> transceiverEntries;
  qsfpService->sync_getTransceiverInfo(
      transceiverEntries, std::vector<int32_t>{transceiverId});

  auto* tcvrInfo = folly::get_ptr(transceiverEntries, transceiverId);
  if (!tcvrInfo) {
    throw FbossError("No transceiver info for transceiver ID ", transceiverId);
  }

  const auto& diags = *tcvrInfo->tcvrState()->diagCapability();
  return LoopbackCapability{
      .capSystem = *diags.loopbackSystem(),
      .capLine = *diags.loopbackLine(),
  };
}

uint8_t readOneByte(
    apache::thrift::Client<QsfpService>* qsfpService,
    int32_t transceiverId,
    int page,
    int offset) {
  ReadRequest req;
  TransceiverIOParameters param;
  req.ids() = {transceiverId};
  param.offset() = offset;
  param.length() = 1;
  if (page >= 0) {
    param.page() = page;
  }
  req.parameter() = param;

  std::map<int32_t, ReadResponse> resp;
  qsfpService->sync_readTransceiverRegister(resp, req);
  auto* respPtr = folly::get_ptr(resp, transceiverId);
  if (!respPtr || respPtr->data()->length() < 1) {
    throw FbossError(
        "Failed to read byte at page 0x",
        fmt::format("{:02X}", page),
        " offset ",
        offset);
  }
  return respPtr->data()->data()[0];
}

std::string formatState(uint8_t mediaNear, uint8_t mediaFar) {
  std::string out;
  out += fmt::format(
      "  system (media-far):   0x{:02X}  {}\n",
      mediaFar,
      mediaFar ? "enabled" : "disabled");
  out += fmt::format(
      "  line (media-near):    0x{:02X}  {}\n",
      mediaNear,
      mediaNear ? "enabled" : "disabled");
  return out;
}

} // namespace facebook::fboss::loopback_utils
