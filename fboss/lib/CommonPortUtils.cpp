// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/lib/CommonPortUtils.h"
#include <folly/logging/xlog.h>
#include <re2/re2.h>

namespace {
constexpr auto kFbossPortNameRegex = "(eth|fab)(\\d+)/(\\d+)/1";
}

namespace facebook::fboss {

/*
 * getPimID
 *
 * A helper function to get the PIM ID for a given port name
 */
int getPimID(const std::string& portName) {
  int pimID = 0;
  int transceiverID = 0;
  re2::RE2 portNameRe(kFbossPortNameRegex);
  if (!re2::RE2::FullMatch(portName, portNameRe, &pimID, &transceiverID)) {
    return -1;
  }
  CHECK_GT(pimID, 0);
  return pimID;
}

/*
 * getTransceiverIndexInPim
 *
 * A helper function to get the transceiver index in the PIM for a given port
 * name. This function returns 0 based transceiver index.
 */
int getTransceiverIndexInPim(const std::string& portName) {
  int pimID = 0;
  int transceiverID = 0;
  re2::RE2 portNameRe(kFbossPortNameRegex);
  if (!re2::RE2::FullMatch(portName, portNameRe, &pimID, &transceiverID)) {
    return -1;
  }
  CHECK_GE(transceiverID, 1);
  return transceiverID - 1;
}

} // namespace facebook::fboss
