// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/data_corral_service/FruPresenceExplorer.h"

#include <fb303/ServiceData.h>
#include <fmt/format.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/lib/CommonFileUtils.h"

namespace {
auto constexpr kFruPresence = "fru_presence_explorer.{}.presence";
auto constexpr kSystem = "SYSTEM";
} // namespace

namespace facebook::fboss::platform::data_corral_service {

FruPresenceExplorer::FruPresenceExplorer(
    const std::vector<FruConfig>& fruConfigs,
    std::shared_ptr<LedManager> ledManager)
    : fruConfigs_(fruConfigs), ledManager_(ledManager) {}

void FruPresenceExplorer::detectFruPresence() {
  XLOG(INFO) << "Detecting presence of FRUs";

  for (const auto& fruConfig : fruConfigs_) {
    auto fruType = *fruConfig.fruType();
    if (fruTypePresence_.find(fruType) == fruTypePresence_.end()) {
      fruTypePresence_[fruType] = true;
    }
    try {
      auto value = std::stoi(
          readSysfs(*fruConfig.presenceSysfsPath()).c_str(),
          nullptr,
          0 /*determine base by format*/);
      auto present = value > 0 ? true : false;
      if (!present) {
        fruTypePresence_[fruType] = false;
      }
      XLOG(INFO) << fmt::format(
          "Detected that {} is {}",
          *fruConfig.fruName(),
          present ? "present" : "absent");
    } catch (const std::exception& ex) {
      XLOG(ERR) << fmt::format(
          "Fail to detect presence of {} because of {}",
          *fruConfig.fruName(),
          ex.what());
      fruTypePresence_[fruType] = false;
    }
  }

  bool allFrusPresent = true;
  for (const auto& [fruType, presence] : fruTypePresence_) {
    if (!presence) {
      allFrusPresent = false;
    }
    fb303::fbData->setCounter(fmt::format(kFruPresence, fruType), presence);
    ledManager_->programFruLed(fruType, presence);
  }
  fb303::fbData->setCounter(fmt::format(kFruPresence, kSystem), allFrusPresent);
  ledManager_->programSystemLed(allFrusPresent);
}

bool FruPresenceExplorer::isPresent(const std::string& fruType) const {
  if (fruTypePresence_.find(fruType) == fruTypePresence_.end()) {
    return false;
  }
  return fruTypePresence_.at(fruType);
}

bool FruPresenceExplorer::isAllPresent() const {
  return std::all_of(
      fruTypePresence_.begin(), fruTypePresence_.end(), [](const auto& pair) {
        return pair.second;
      });
}

} // namespace facebook::fboss::platform::data_corral_service
