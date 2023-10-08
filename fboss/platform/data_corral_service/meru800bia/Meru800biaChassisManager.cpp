// Copyright 2023-present Facebook. All Rights Reserved.

#include "fboss/platform/data_corral_service/meru800bia/Meru800biaChassisManager.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/platform/data_corral_service/meru800bia/Meru800biaFruModule.h"
#include "fboss/platform/data_corral_service/meru800bia/Meru800biaPlatformConfig.h"

namespace {
// TODO: support modules

// leds in meru800bia chassis
const std::string kSystemLed = "SystemLed";
const std::string kFanLed = "FanLed";
const std::string kPsuLed = "PsuLed";
const std::string kSmbLed = "SmbLed";

// colors available in leds
const std::string kLedBlue = "Blue";
const std::string kLedAmber = "Amber";

const std::string kSetColorFailure = "set.color.failure";
} // namespace

namespace facebook::fboss::platform::data_corral_service {

void Meru800biaChassisLed::setColorPath(
    ChassisLedColor color,
    const std::string& path) {
  XLOG(DBG2) << "led " << name_ << ", color " << color << ", sysfs path "
             << path;
  paths_[color] = path;
}

void Meru800biaChassisLed::setColor(ChassisLedColor color) {
  if (color_ != color) {
    if (!facebook::fboss::writeSysfs(paths_[color], "1")) {
      XLOG(ERR) << "failed to set color " << color << " for led " << name_;
      fb303::fbData->setCounter(kSetColorFailure, 1);
      return;
    }
    for (auto const& colorPath : paths_) {
      if (colorPath.first != color) {
        if (!facebook::fboss::writeSysfs(colorPath.second, "0")) {
          XLOG(ERR) << "failed to unset color " << color << " for led "
                    << name_;
          fb303::fbData->setCounter(kSetColorFailure, 1);
          return;
        }
      }
    }
    XLOG(INFO) << "Set led " << name_ << " from color " << color_
               << " to color " << color;
    color_ = color;
  }
  fb303::fbData->setCounter(kSetColorFailure, 0);
}

ChassisLedColor Meru800biaChassisLed::getColor() {
  for (auto const& colorPath : paths_) {
    std::string brightness = facebook::fboss::readSysfs(colorPath.second);
    try {
      if (std::stoi(brightness) > 0) {
        color_ = colorPath.first;
        return color_;
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "failed to parse present state from " << colorPath.second
                << " where the value is " << brightness;
      throw;
    }
  }
  return ChassisLedColor::OFF;
}

void Meru800biaChassisManager::initModules() {
  XLOG(DBG2) << "instantiate fru modules and chassis leds";
  auto platformConfig = apache::thrift::SimpleJSONSerializer::deserialize<
      DataCorralPlatformConfig>(getMeru800biaPlatformConfig());
  for (auto fru : *platformConfig.fruModules()) {
    auto name = *fru.name();
    auto fruModule = std::make_unique<Meru800biaFruModule>(name);
    fruModule->init(*fru.attributes());
    fruModules_.emplace(name, std::move(fruModule));
  }

  sysLed_ = std::make_unique<Meru800biaChassisLed>(kSystemLed);
  fanLed_ = std::make_unique<Meru800biaChassisLed>(kFanLed);
  psuLed_ = std::make_unique<Meru800biaChassisLed>(kPsuLed);
  smbLed_ = std::make_unique<Meru800biaChassisLed>(kSmbLed);
  for (auto attr : *platformConfig.chassisAttributes()) {
    if (*attr.name() == kSystemLed + kLedAmber) {
      sysLed_->setColorPath(ChassisLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kSystemLed + kLedBlue) {
      sysLed_->setColorPath(ChassisLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kFanLed + kLedAmber) {
      fanLed_->setColorPath(ChassisLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kFanLed + kLedBlue) {
      fanLed_->setColorPath(ChassisLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPsuLed + kLedAmber) {
      psuLed_->setColorPath(ChassisLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPsuLed + kLedBlue) {
      psuLed_->setColorPath(ChassisLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kSmbLed + kLedAmber) {
      smbLed_->setColorPath(ChassisLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kSmbLed + kLedBlue) {
      smbLed_->setColorPath(ChassisLedColor::BLUE, *attr.path());
    }
  }
}

void Meru800biaChassisManager::programChassis() {
  ChassisLedColor sysLedColor = ChassisLedColor::BLUE;
  ChassisLedColor fanLedColor = ChassisLedColor::BLUE;
  ChassisLedColor psuLedColor = ChassisLedColor::BLUE;
  ChassisLedColor smbLedColor = ChassisLedColor::BLUE;

  for (auto& fru : fruModules_) {
    if (!fru.second->isPresent()) {
      XLOG(DBG2) << "Fru module " << fru.first << " is absent";
      sysLedColor = ChassisLedColor::AMBER;
    }
  }
  if (sysLedColor == ChassisLedColor::BLUE) {
    XLOG(DBG4) << "All fru modules are present";
  }
  fb303::fbData->setCounter(fmt::format("{}.color", kSystemLed), sysLedColor);
  fb303::fbData->setCounter(fmt::format("{}.color", kFanLed), fanLedColor);
  fb303::fbData->setCounter(fmt::format("{}.color", kPsuLed), psuLedColor);
  fb303::fbData->setCounter(fmt::format("{}.color", kSmbLed), smbLedColor);
  sysLed_->setColor(sysLedColor);
  fanLed_->setColor(fanLedColor);
  psuLed_->setColor(psuLedColor);
  smbLed_->setColor(smbLedColor);
}

} // namespace facebook::fboss::platform::data_corral_service
