// Copyright 2014-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/platform/data_corral_service/ChassisManager.h>

namespace facebook::fboss::platform::data_corral_service {

enum DarwinLedColor {
  OFF = 0,
  RED = 1,
  GREEN = 2,
};

class DarwinChassisLed {
 public:
  explicit DarwinChassisLed(const std::string& name)
      : name_(name), color_(DarwinLedColor::OFF) {}
  void setColorPath(DarwinLedColor color, const std::string& path);
  const std::string& getName() {
    return name_;
  }
  void setColor(DarwinLedColor color);
  DarwinLedColor getColor();

 private:
  std::string name_;
  DarwinLedColor color_;
  std::unordered_map<DarwinLedColor, std::string> paths_;
};

class DarwinChassisManager : public ChassisManager {
 public:
  explicit DarwinChassisManager(int refreshInterval)
      : ChassisManager(refreshInterval) {}
  virtual void initModules() override;
  virtual void programChassis() override;

 private:
  std::unique_ptr<DarwinChassisLed> sysLed_;
  std::unique_ptr<DarwinChassisLed> fanLed_;
  std::unique_ptr<DarwinChassisLed> pemLed_;
};

} // namespace facebook::fboss::platform::data_corral_service
