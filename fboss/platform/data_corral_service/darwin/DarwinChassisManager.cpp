// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/darwin/DarwinChassisManager.h>
#include <fboss/platform/data_corral_service/darwin/DarwinFruModule.h>
#include <fboss/platform/data_corral_service/darwin/DarwinPlatformConfig.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss::platform::data_corral_service {

void DarwinChassisLed::setColorPath(
    DarwinLedColor color,
    const std::string& path) {
  XLOG(DBG2) << "led " << name_ << ", color " << color << ", sysfs path "
             << path;
  paths_[color] = path;
}

void DarwinChassisLed::setColor(DarwinLedColor color) {
  // TODO(daiweix): set color through writeSysfs(path)
}

DarwinLedColor DarwinChassisLed::getColor() {
  // TODO(daiweix): read color through readSysfs(path)
  return color_;
}

void DarwinChassisManager::initModules() {
  XLOG(DBG2) << "instantiate fru modules and chassis leds";
  auto platformConfig = apache::thrift::SimpleJSONSerializer::deserialize<
      DataCorralPlatformConfig>(getDarwinPlatformConfig());
  for (auto fru : *platformConfig.fruModules()) {
    auto name = *fru.name();
    auto fruModule = std::make_unique<DarwinFruModule>(name);
    fruModule->init(*fru.attributes());
    fruModules_.emplace(name, std::move(fruModule));
  }

  sysLed_ = std::make_unique<DarwinChassisLed>("SystemLed");
  fanLed_ = std::make_unique<DarwinChassisLed>("FanLed");
  pemLed_ = std::make_unique<DarwinChassisLed>("PemLed");
  rackmonLed_ = std::make_unique<DarwinChassisLed>("RackmonLed");
  for (auto attr : *platformConfig.chassisAttributes()) {
    if (*attr.name() == "SystemLedRed") {
      sysLed_->setColorPath(DarwinLedColor::RED, *attr.path());
    } else if (*attr.name() == "SystemLedGreen") {
      sysLed_->setColorPath(DarwinLedColor::GREEN, *attr.path());
    } else if (*attr.name() == "FanLedRed") {
      fanLed_->setColorPath(DarwinLedColor::RED, *attr.path());
    } else if (*attr.name() == "FanLedGreen") {
      fanLed_->setColorPath(DarwinLedColor::GREEN, *attr.path());
    } else if (*attr.name() == "PemLedRed") {
      fanLed_->setColorPath(DarwinLedColor::RED, *attr.path());
    } else if (*attr.name() == "PemLedGreen") {
      fanLed_->setColorPath(DarwinLedColor::GREEN, *attr.path());
    } else if (*attr.name() == "RackmonLedRed") {
      fanLed_->setColorPath(DarwinLedColor::RED, *attr.path());
    } else if (*attr.name() == "RackmonLedGreen") {
      fanLed_->setColorPath(DarwinLedColor::GREEN, *attr.path());
    }
  }
}

void DarwinChassisManager::programChassis() {
  XLOG(DBG4) << "program Darwin system LED based on updated Fru module state";
}

} // namespace facebook::fboss::platform::data_corral_service
