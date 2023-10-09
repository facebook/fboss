// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/data_corral_service/FruPresenceExplorer.h"

#include <fb303/ServiceData.h>
#include <fmt/format.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss::platform::data_corral_service {

FruPresenceExplorer::FruPresenceExplorer(
    const std::vector<FruConfig>& fruConfigs,
    std::shared_ptr<LedManager> ledManager)
    : fruConfigs_(fruConfigs), ledManager_(ledManager) {}

void FruPresenceExplorer::detectFruPresence() const {
  XLOG(INFO) << "Detecting presence of FRUs";

  std::unordered_map<std::string, bool> fruTypePresence;
  for (const auto& fruConfig : fruConfigs_) {
    auto fruType = *fruConfig.fruType();
    if (fruTypePresence.find(fruType) == fruTypePresence.end()) {
      fruTypePresence[fruType] = true;
    }
    try {
      auto value = std::stoi(readSysfs(*fruConfig.presenceSysfsPath()));
      auto present = value > 0 ? true : false;
      if (!present) {
        fruTypePresence[fruType] = false;
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
      fruTypePresence[fruType] = false;
    }
  }

  bool allFrusPresent = true;
  for (const auto& [fruType, presence] : fruTypePresence) {
    if (!presence) {
      allFrusPresent = false;
      fb303::fbData->setCounter(
          fmt::format(
              "fru_presence_explorer.{}.presence_detection_fail", fruType),
          1);
    } else {
      fb303::fbData->setCounter(
          fmt::format(
              "fru_presence_explorer.{}.presence_detection_fail", fruType),
          0);
    }

    if (!ledManager_->programFruLed(fruType, presence)) {
      fb303::fbData->setCounter(
          fmt::format("fru_presence_explorer.{}.program_led_fail", fruType), 1);
    } else {
      fb303::fbData->setCounter(
          fmt::format("fru_presence_explorer.{}.program_led_fail", fruType), 0);
    }
  }
  bool systemLedProgramSuccess = ledManager_->programSystemLed(allFrusPresent);
  if (!systemLedProgramSuccess) {
    fb303::fbData->setCounter(
        "fru_presence_explorer.system.program_led_fail", 1);
  } else {
    fb303::fbData->setCounter(
        "fru_presence_explorer.system.program_led_fail", 0);
  }
}

} // namespace facebook::fboss::platform::data_corral_service
