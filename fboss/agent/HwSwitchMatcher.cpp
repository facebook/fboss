// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/HwSwitchMatcher.h"

#include "fboss/agent/FbossError.h"

#include <sstream>
#include <string>

namespace facebook::fboss {
HwSwitchMatcher::HwSwitchMatcher(const std::string& matcherString)
    : matcherString_(matcherString) {
  // matcherString is "id=1,2,3,4"
  // or "id=" followed by list of digits separated by comma
  if (matcherString.rfind("id=", 0) != 0) {
    throw FbossError("HwSwitchMatcher: invalid matcher string");
  }

  std::string_view view(matcherString);
  // chop away id=
  view.remove_prefix(3);
  std::stringstream stream;
  stream << view;

  while (stream.good()) {
    std::string npuId;
    std::getline(stream, npuId, ',');
    switchIds_.insert(static_cast<SwitchID>(std::stoi(npuId)));
  }
}

HwSwitchMatcher::HwSwitchMatcher(const std::unordered_set<SwitchID>& switchIds)
    : switchIds_(switchIds) {
  if (switchIds.empty()) {
    throw FbossError("HwSwitchMatcher: invalid npus");
  }
  std::stringstream stream;
  stream << "id=";
  std::copy(
      switchIds_.begin(),
      switchIds_.end(),
      std::ostream_iterator<int>(stream, ","));
  matcherString_ = stream.str();
  matcherString_.pop_back();
}

HwSwitchMatcher HwSwitchMatcher::defaultHwSwitchMatcher() {
  static HwSwitchMatcher kDefaultHwSwitchMatcher;
  return kDefaultHwSwitchMatcher;
}

const std::string& HwSwitchMatcher::defaultHwSwitchMatcherKey() {
  static std::string kDefaultHwSwitchMatcherKey{"id=0"};
  return kDefaultHwSwitchMatcherKey;
}

SwitchID HwSwitchMatcher::switchId() const {
  if (switchIds_.size() != 1) {
    throw FbossError(
        "HwSwitchMatcher::switchId api must be called only when there is a single switchId");
  }
  return *switchIds_.begin();
}

} // namespace facebook::fboss
