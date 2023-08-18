// Copyright 2023-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/platform/data_corral_service/ChassisManager.h>

namespace facebook::fboss::platform::data_corral_service {

class Meru800biaChassisLed {
 public:
  explicit Meru800biaChassisLed(const std::string& name)
      : name_(name), color_(ChassisLedColor::OFF) {}
  void setColorPath(ChassisLedColor color, const std::string& path);
  const std::string& getName() {
    return name_;
  }
  void setColor(ChassisLedColor color);
  ChassisLedColor getColor();

 private:
  std::string name_;
  ChassisLedColor color_;
  std::unordered_map<ChassisLedColor, std::string> paths_;
};

class Meru800biaChassisManager : public ChassisManager {
 public:
  explicit Meru800biaChassisManager(int refreshInterval)
      : ChassisManager(refreshInterval) {}
  virtual void initModules() override;
  virtual void programChassis() override;

 private:
  std::unique_ptr<Meru800biaChassisLed> sysLed_;
  std::unique_ptr<Meru800biaChassisLed> fanLed_;
  std::unique_ptr<Meru800biaChassisLed> psuLed_;
  std::unique_ptr<Meru800biaChassisLed> smbLed_;
};

} // namespace facebook::fboss::platform::data_corral_service
