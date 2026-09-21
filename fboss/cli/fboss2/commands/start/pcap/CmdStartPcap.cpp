// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/start/pcap/CmdStartPcap.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"

namespace {

const int32_t kDefaultMaxPackets = 10000;
const facebook::fboss::CaptureDirection kDefaultDirectionVal =
    facebook::fboss::CaptureDirection::CAPTURE_TX_RX;

facebook::fboss::CaptureDirection getCaptureDirection(
    const std::string& direction) {
  if (direction == "txonly") {
    return facebook::fboss::CaptureDirection::CAPTURE_ONLY_TX;
  } else if (direction == "rxonly") {
    return facebook::fboss::CaptureDirection::CAPTURE_ONLY_RX;
  } else if (direction == "txrx") {
    return facebook::fboss::CaptureDirection::CAPTURE_TX_RX;
  } else {
    throw std::runtime_error("Unsupported direction: " + direction);
  }
}

std::string getCaptureDirectionStr(
    const facebook::fboss::CaptureDirection& directionVal) {
  switch (directionVal) {
    case facebook::fboss::CaptureDirection::CAPTURE_ONLY_TX:
      return "Tx only";
    case facebook::fboss::CaptureDirection::CAPTURE_ONLY_RX:
      return "Rx only";
    case facebook::fboss::CaptureDirection::CAPTURE_TX_RX:
      return "both Tx and Rx";
    default:
      throw std::runtime_error(
          "Unsupported direction value : " +
          std::to_string(static_cast<int>(directionVal)));
  }
}

} // namespace

namespace facebook::fboss {

CmdStartPcap::RetType CmdStartPcap::queryClient(const HostInfo& hostInfo) {
  std::string nameStr;
  CaptureDirection directionVal;
  std::string directionStr;
  CaptureInfo captureInfo;
  auto name =
      CmdLocalOptions::getInstance()->getLocalOption("start_pcap", "--name");
  nameStr = name.empty() ? "packet_capture" : name;
  captureInfo.name() = nameStr;
  auto maxPackets =
      CmdLocalOptions::getInstance()->getLocalOption("start_pcap", "--max");
  captureInfo.maxPackets() =
      maxPackets.empty() ? kDefaultMaxPackets : stoi(maxPackets);
  auto direction = CmdLocalOptions::getInstance()->getLocalOption(
      "start_pcap", "--direction");
  directionVal =
      direction.empty() ? kDefaultDirectionVal : getCaptureDirection(direction);
  captureInfo.direction() = directionVal;
  directionStr = direction.empty() ? "both Tx and Rx"
                                   : getCaptureDirectionStr(directionVal);
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_startPktCapture(captureInfo);
  return fmt::format(
      "Starting {} packet capture \"{}\"", directionStr, nameStr);
}

void CmdStartPcap::printOutput(
    const RetType& captureOutput,
    std::ostream& out) {
  out << captureOutput << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdStartPcap, CmdStartPcapTraits>::run();

} // namespace facebook::fboss
