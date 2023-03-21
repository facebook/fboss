/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdStartPcapTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
  std::vector<utils::LocalOption> LocalOptions = {
      {"--name", "Name to identify packet capture"},
      {"--max", "Max number of packets to capture"},
      {"--direction", "Direction of packets (txonly, rxonly, txrx)"}};
};

class CmdStartPcap : public CmdHandler<CmdStartPcap, CmdStartPcapTraits> {
 public:
  const int32_t defaultMaxPackets_ = 10000;
  const CaptureDirection defaultDirectionVal_ = CaptureDirection::CAPTURE_TX_RX;
  using ObjectArgType = CmdStartPcapTraits::ObjectArgType;
  using RetType = CmdStartPcapTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
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
        maxPackets.empty() ? defaultMaxPackets_ : stoi(maxPackets);
    auto direction = CmdLocalOptions::getInstance()->getLocalOption(
        "start_pcap", "--direction");
    directionVal = direction.empty() ? defaultDirectionVal_
                                     : getCaptureDirection(direction);
    captureInfo.direction() = directionVal;
    directionStr = direction.empty() ? "both Tx and Rx"
                                     : getCaptureDirectionStr(directionVal);
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_startPktCapture(captureInfo);
    return fmt::format(
        "Starting {} packet capture \"{}\"", directionStr, nameStr);
  }

  void printOutput(
      const RetType& captureOutput,
      std::ostream& out = std::cout) {
    out << captureOutput << std::endl;
  }

  CaptureDirection getCaptureDirection(const std::string& direction) {
    if (direction == "txonly") {
      return CaptureDirection::CAPTURE_ONLY_TX;
    } else if (direction == "rxonly") {
      return CaptureDirection::CAPTURE_ONLY_RX;
    } else if (direction == "txrx") {
      return CaptureDirection::CAPTURE_TX_RX;
    } else {
      throw std::runtime_error("Unsupported direction: " + direction);
    }
  }

  std::string getCaptureDirectionStr(const CaptureDirection& directionVal) {
    switch (directionVal) {
      case CaptureDirection::CAPTURE_ONLY_TX:
        return "Tx only";
      case CaptureDirection::CAPTURE_ONLY_RX:
        return "Rx only";
      case CaptureDirection::CAPTURE_TX_RX:
        return "both Tx and Rx";
      default:
        throw std::runtime_error(
            "Unsupported direction value : " +
            std::to_string(static_cast<int>(directionVal)));
    }
  }
};

} // namespace facebook::fboss
