// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/bspmapping/Parser.h"
#include <folly/FileUtil.h>
#include <folly/Range.h>
#include <folly/String.h>
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"
#include "fboss/lib/platforms/PlatformMode.h"

namespace facebook::fboss {

std::string Parser::getNameFor(PlatformType platform) {
  std::string name = facebook::fboss::toString(platform);
  folly::toLowerAscii(name);
  return name;
}

TransceiverConfigRow Parser::getTransceiverConfigRowFromCsvLine(
    const std::string_view& line) {
  std::vector<std::string_view> parts;
  folly::split(',', line, parts);
  if (parts.size() != 17 && parts.size() != 18) {
    throw std::runtime_error("Invalid line format");
  }
  TransceiverConfigRow tcr;
  tcr.tcvrId() = folly::to<int>(parts[0]);
  if (parts[1] != "") {
    tcr.tcvrLaneIdList() = Parser::getTransceiverLaneIdList(parts[1]);
  }

  tcr.pimId() = folly::to<int>(parts[2]);
  tcr.accessCtrlId() = parts[3];
  tcr.accessCtrlType() = Parser::getAccessCtrlTypeFromString(parts[4]);
  tcr.resetPath() = parts[5];
  tcr.resetMask() = folly::to<int>(parts[6]);
  tcr.resetHoldHi() = folly::to<int>(parts[7]);
  tcr.presentPath() = parts[8];
  tcr.presentMask() = folly::to<int>(parts[9]);
  tcr.presentHoldHi() = folly::to<int>(parts[10]);
  tcr.ioCtrlId() = parts[11];
  tcr.ioCtrlType() = Parser::getTransceiverIOTypeFromString(parts[12]);
  tcr.ioPath() = parts[13];
  if (parts[14] != "") {
    tcr.ledId() = folly::to<int>(parts[14]);
  }
  if (parts[15] != "") {
    tcr.ledBluePath() = parts[15];
  }

  if (parts[16] != "") {
    tcr.ledYellowPath() = parts[16];
  }

  return tcr;
}

std::vector<int> Parser::getTransceiverLaneIdList(
    const std::string_view& entry) {
  std::vector<std::string_view> parts;
  folly::split(' ', entry, parts);
  std::vector<int> laneList;
  for (auto& part : parts) {
    laneList.push_back(folly::to<int>(part));
  }
  return laneList;
}

ResetAndPresenceAccessType Parser::getAccessCtrlTypeFromString(
    const std::string_view& entry) {
  if (entry == "CPLD") {
    return ResetAndPresenceAccessType::CPLD;
  } else if (entry == "GPIO") {
    return ResetAndPresenceAccessType::GPIO;
  } else {
    return ResetAndPresenceAccessType::UNKNOWN;
  }
}

TransceiverIOType Parser::getTransceiverIOTypeFromString(
    const std::string_view& entry) {
  if (entry == "I2C") {
    return TransceiverIOType::I2C;
  } else {
    return TransceiverIOType::UNKNOWN;
  }
}

std::vector<TransceiverConfigRow> Parser::getTransceiverConfigRowsFromCsv(
    folly::StringPiece csv) {
  std::string data;
  folly::readFile(csv.data(), data);
  std::vector<std::string_view> lines;
  folly::split('\n', data, lines);

  std::vector<TransceiverConfigRow> tcrList;
  int nLine = 0;
  for (auto& line : lines) {
    if (nLine++ < HEADER_OFFSET || line.empty()) {
      continue;
    }

    auto tl = getTransceiverConfigRowFromCsvLine(line);
    tcrList.push_back(tl);
  }

  return tcrList;
}

BspPlatformMappingThrift Parser::getBspPlatformMappingFromCsv(
    folly::StringPiece csv) {
  auto tcrList = getTransceiverConfigRowsFromCsv(csv);
  std::map<int, std::vector<TransceiverConfigRow>> tcrMap;
  for (auto& tl : tcrList) {
    tcrMap[folly::copy(tl.tcvrId().value())].push_back(tl);
  }

  std::map<int, BspTransceiverMapping> tcvrMap;
  for (auto& [tcvrId, tcrList] : tcrMap) {
    auto first = tcrList[0];
    BspResetPinInfo resetPinInfo;
    resetPinInfo.sysfsPath() = first.resetPath().value();
    resetPinInfo.mask() = folly::copy(first.resetMask().value());
    resetPinInfo.gpioOffset() = 0;
    resetPinInfo.resetHoldHi() = folly::copy(first.resetHoldHi().value());

    BspPresencePinInfo presentInfo;
    presentInfo.sysfsPath() = first.presentPath().value();
    presentInfo.mask() = folly::copy(first.presentMask().value());
    presentInfo.gpioOffset() = 0;
    presentInfo.presentHoldHi() = folly::copy(first.presentHoldHi().value());

    BspTransceiverAccessControllerInfo accessControllerInfo;
    accessControllerInfo.controllerId() = first.accessCtrlId().value();
    accessControllerInfo.type() = folly::copy(first.accessCtrlType().value());
    accessControllerInfo.reset() = resetPinInfo;
    accessControllerInfo.presence() = presentInfo;
    accessControllerInfo.gpioChip() = "";

    BspTransceiverIOControllerInfo ioControlInfo;
    ioControlInfo.controllerId() = first.ioCtrlId().value();
    ioControlInfo.type() = folly::copy(first.ioCtrlType().value());
    ioControlInfo.devicePath() = first.ioPath().value();
    std::map<int, int> laneToLedMap;
    for (auto& ledLine : tcrList) {
      if (apache::thrift::get_pointer(ledLine.tcvrLaneIdList()) == nullptr) {
        continue;
      }
      for (auto iter =
               (*apache::thrift::get_pointer(ledLine.tcvrLaneIdList())).begin();
           iter !=
           (*apache::thrift::get_pointer(ledLine.tcvrLaneIdList())).end();
           iter++) {
        int laneId = *iter;
        if (laneId < MIN_LANE_ID || laneId > MAX_LANE_ID) {
          throw std::runtime_error("Invalid lane id");
        }
        if (apache::thrift::get_pointer(ledLine.ledId()) != nullptr) {
          laneToLedMap[laneId] = *apache::thrift::get_pointer(ledLine.ledId());
        }
      }
    }

    BspTransceiverMapping singleTcvrMap;
    singleTcvrMap.tcvrId() = folly::copy(first.tcvrId().value());
    singleTcvrMap.accessControl() = accessControllerInfo;
    singleTcvrMap.io() = ioControlInfo;
    singleTcvrMap.tcvrLaneToLedId() = laneToLedMap;

    tcvrMap[folly::copy(first.tcvrId().value())] = singleTcvrMap;
  }

  std::map<int, LedMapping> ledsMap;
  for (auto& tl : tcrList) {
    if (apache::thrift::get_pointer(tl.ledId()) == nullptr) {
      continue;
    }

    LedMapping singleLedMap;
    singleLedMap.id() = *apache::thrift::get_pointer(tl.ledId());
    singleLedMap.bluePath() = *apache::thrift::get_pointer(tl.ledBluePath());
    singleLedMap.yellowPath() =
        *apache::thrift::get_pointer(tl.ledYellowPath());
    singleLedMap.transceiverId() = folly::copy(tl.tcvrId().value());
    ledsMap[*apache::thrift::get_pointer(tl.ledId())] = singleLedMap;
  }

  BspPimMapping bspPimMapping;
  bspPimMapping.pimID() = folly::copy(tcrList.back().pimId().value());
  bspPimMapping.tcvrMapping() = tcvrMap;
  bspPimMapping.phyMapping() = {};
  bspPimMapping.phyIOControllers() = {};
  bspPimMapping.ledMapping() = ledsMap;

  std::map<int, BspPimMapping> bspPlatformMap;
  bspPlatformMap[folly::copy(tcrList.back().pimId().value())] = bspPimMapping;
  BspPlatformMappingThrift bspPlatformMapping;
  bspPlatformMapping.pimMapping() = bspPlatformMap;

  return bspPlatformMapping;
}

} // namespace facebook::fboss
