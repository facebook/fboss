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

  std::vector<TransceiverConfigRow> tlList;
  int nLine = 0;
  for (auto& line : lines) {
    if (nLine++ < HEADER_OFFSET || line.empty()) {
      continue;
    }

    auto tl = getTransceiverConfigRowFromCsvLine(line);
    tlList.push_back(tl);
  }

  return tlList;
}

} // namespace facebook::fboss
