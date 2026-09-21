// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <nlohmann/json.hpp>

namespace rackmonsvc {

enum RackmonPlsLineType { POWER, REDUNDANCY };

struct GpioLine {
  // "gpioChip" and "offset" are used to identify a specific GPIO line.
  // "name" is used mainly for logging purpose.
  std::string gpioChip{};
  int offset = -1;
  std::string name{};

  // "consumer" and "activeLow" are used by "gpiod_ctxless_get_value".
  // TODO: ideally we should allow callers to set "activeLow" at open time.
  std::string consumer{};
  bool activeLow = false;

  void open(const std::string& gpioConsumer);
  int getValue();

 private:
  void parseOffset();
};
void from_json(const nlohmann::json& j, GpioLine& gpio);

struct RackmonPlsPort {
  std::string name{};
  GpioLine power;
  GpioLine redundancy;
  const std::string portOwner{"rackmond"};

  void open();
  std::pair<bool, bool> getPowerState();

  RackmonPlsPort& operator=(const nlohmann::json& j) {
    j.at("name").get_to(name);

    for (auto& gpioLine : j["lines"]) {
      RackmonPlsLineType type = gpioLine["type"];

      if (type == RackmonPlsLineType::POWER) {
        gpioLine.get_to(power);
        power.name += ("@" + name);
      } else if (type == RackmonPlsLineType::REDUNDANCY) {
        gpioLine.get_to(redundancy);
        redundancy.name += ("@" + name);
      }
    }
    return *this;
  }
};

// RackmonPlsManager is designed to read Rackmon PLS (Power Loss Siren)
// signals.
// Each TOR has 3 rackmon (RJ45) ports, and each port provides below 2
// GPIO lines for Power Loss Siren:
//   - power line: active high (1 for good, 0 indicates power loss).
//   - redundancy line: active high (1 for good, 0 indicates redundancy loss).
class RackmonPlsManager {
  std::vector<std::unique_ptr<RackmonPlsPort>> plsPorts_ = {};

 public:
  // Load Interface configuration.
  void loadPlsConfig(const nlohmann::json& config);

  std::vector<std::pair<bool, bool>> getPowerState();
};

} // namespace rackmonsvc
