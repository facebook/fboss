// Copyright 2021-present Facebook. All Rights Reserved.
#include "RackmonPlsManager.h"
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <filesystem>

#ifdef IS_OSS
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#else // !IS_OSS
#include <gpiod.h>
#endif // IS_OSS

using nlohmann::json;

namespace rackmonsvc {

NLOHMANN_JSON_SERIALIZE_ENUM(
    RackmonPlsLineType,
    {
        {RackmonPlsLineType::POWER, "power"},
        {RackmonPlsLineType::REDUNDANCY, "redundancy"},
    })

// FIXME: we failed to enable "libgpiod" in FBOSS OSS build environment
// due to compilation errors. In order to unblock rackmon deployment,
// below workaround (#ifdef IS_OSS) is applied, and the workaround should
// be removed when "libgpiod" is added to FBOSS OSS dependencies list.
// Check T126740581 for details.
#ifdef IS_OSS
void GpioLine::parseOffset() {
  int fd = ::open(gpioChip.c_str(), O_RDWR | O_CLOEXEC);
  if (fd < 0) {
    throw std::runtime_error(gpioChip + ": open failed");
  }

  struct gpiochip_info info = {};
  int rv = ::ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
  close(fd);

  if (rv < 0) {
    throw std::runtime_error(gpioChip + ": GPIO_GET_CHIPINFO_IOCTL failed");
  }
  if (offset >= info.lines) {
    throw std::invalid_argument(name + ": gpio offset exceeds limits");
  }
}

int GpioLine::getValue() {
  std::string cmd = "gpioget " + gpioChip + " " + std::to_string(offset);
  std::unique_ptr<FILE, decltype(&pclose)> pipe(
      popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed: " + cmd);
  }

  char buf[128] = {};
  std::string output;
  while (fgets(buf, sizeof(buf), pipe.get()) != nullptr) {
    output += buf;
  }

  return std::stoi(output);
}
#else // !IS_OSS
void GpioLine::parseOffset() {
  struct gpiod_chip* chip = gpiod_chip_open(gpioChip.c_str());
  if (!chip) {
    throw std::runtime_error(name + ": failed to open " + gpioChip);
  }

  int numLines = gpiod_chip_num_lines(chip);
  gpiod_chip_close(chip);

  if (offset >= numLines) {
    throw std::invalid_argument(name + ": gpio offset exceeds limits");
  }
}

int GpioLine::getValue() {
  int val = gpiod_ctxless_get_value(
      gpioChip.c_str(), offset, activeLow, consumer.c_str());
  if (val < 0) {
    throw std::runtime_error("failed to read gpio " + name);
  }

  return val;
}
#endif // IS_OSS

void GpioLine::open(const std::string& gpioConsumer) {
  if (gpioChip.empty()) {
    throw std::invalid_argument(name + ": gpiochip not specified");
  }
  if (!std::filesystem::exists(gpioChip)) {
    throw std::invalid_argument(name + ": gpiochip file does not exist");
  }
  if (offset < 0) {
    throw std::invalid_argument(name + ": invalid (negative) gpio offset");
  }

  // The gpioChip in the config file is usually a symbolic link pointing
  // to /dev/gpiochip#, because gpiochip number is dynamically allocated.
  // The symlink will be resolved to the target file when config file is
  // loaded.
  if (std::filesystem::is_symlink(gpioChip)) {
    gpioChip = std::filesystem::read_symlink(gpioChip).string();
  }

  parseOffset();

  consumer = gpioConsumer;
  LOG(INFO) << name << " PLS: gpioChip=" << gpioChip << ", offset=" << offset
            << std::endl;
}

void RackmonPlsPort::open() {
  power.open(portOwner);
  redundancy.open(portOwner);
}

std::pair<bool, bool> RackmonPlsPort::getPowerState() {
  std::pair<bool, bool> ret = {
      power.getValue() == 0, redundancy.getValue() == 0};
  return ret;
}

void RackmonPlsManager::loadPlsConfig(const nlohmann::json& config) {
  for (auto& entry : config["ports"]) {
    std::unique_ptr<RackmonPlsPort> port = std::make_unique<RackmonPlsPort>();
    *port = entry;
    port->open();
    plsPorts_.push_back(std::move(port));
  }

  if (plsPorts_.size() != 3) {
    throw std::runtime_error("unexpected number of ports in config file");
  }
}

std::vector<std::pair<bool, bool>> RackmonPlsManager::getPowerState() {
  std::vector<std::pair<bool, bool>> ret = {};

  for (auto& port : plsPorts_) {
    ret.push_back(port->getPowerState());
  }

  return ret;
}

void from_json(const json& j, GpioLine& gpio) {
  j.at("gpioChip").get_to(gpio.gpioChip);
  j.at("offset").get_to(gpio.offset);
  j.at("type").get_to(gpio.name);
}

} // namespace rackmonsvc
