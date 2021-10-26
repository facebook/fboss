// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/ScdHelper.h"
#include <fcntl.h>
#include <folly/String.h>
#include <glog/logging.h>
#include <sys/mman.h>
#include <sstream>

namespace facebook::fboss::platform::helpers {

ScdHelper::ScdHelper() {
  std::string sysfsPath;
  std::array<char, 64> busId;

  FILE* fp = popen("lspci -d 3475:0001 | sed -e \'s/ .*//\'", "r");
  if (!fp) {
    throw std::runtime_error("SCD FPGA not found on PCI bus");
  }
  fgets(busId.data(), 64, fp);
  std::istringstream iStream(folly::to<std::string>(busId));
  std::string busIdStr;
  iStream >> busIdStr;

  // Build the sysfs file path for resource0 where all registers are mapped
  sysfsPath = folly::to<std::string>(
      "/sys/bus/pci/devices/0000:", busIdStr, "/resource0");

  pclose(fp);

  // Open the sysfs file to access the device
  fd_ = open(sysfsPath.c_str(), O_RDWR | O_SYNC);
  if (fd_ == -1) {
    throw std::runtime_error(
        std::string("Unable to open up the sysfs file %s") + sysfsPath);
  }

  // Mmeory map the device's 512KB memory register space to our
  // user space
  mapSize_ = 0x80000;
  mapAddr_ = (uint8_t*)mmap(
      nullptr, mapSize_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (mapAddr_ == MAP_FAILED) {
    close(fd_);
    throw std::runtime_error("mmap failed for resource");
  }

  return;
}

/*
 * Disable Watchdog Timer
 */
void ScdHelper::disable_wdt() {
  uint32_t regVal = readReg(0x0120);

  regVal &= 0x7FFFFFFF;
  writeReg(0x0120, regVal);
}

/*
 * Kick watchdog with desired timeout value in seconds
 */
void ScdHelper::kick_wdt(uint32_t seconds) {
  if (seconds > 655) {
    LOG(INFO)
        << "Timeout value can not be greater than 655s, set to maximum 655s";
    seconds = 655;
  }
  uint32_t regVal = 0xC0000000 + seconds * 100;
  writeReg(0x0120, regVal);
}

ScdHelper::~ScdHelper() {
  if (fd_) {
    close(fd_);
  }
}

uint32_t ScdHelper::readReg(uint32_t regAddr) {
  if (regAddr >= mapSize_ - 3) {
    // Reg offset should not be more than map size
    throw std::runtime_error("Register offset to read exceeds memory map size");
  }
  LOG(INFO) << "Reg 0x:" << std::hex << regAddr << "0x"
            << *(volatile uint32_t*)(mapAddr_ + regAddr);
  return *(volatile uint32_t*)(mapAddr_ + regAddr);
}

void ScdHelper::writeReg(uint32_t regAddr, uint32_t val) {
  if (regAddr >= mapSize_ - 3) {
    throw std::runtime_error(
        "Register offset to write exceeds memory map size");
    return;
  }

  LOG(INFO) << "Reg 0x:" << std::hex << regAddr << " original value is 0x"
            << *(volatile unsigned int*)(mapAddr_ + regAddr)
            << ", will write 0x" << val;
  *(volatile unsigned int*)(mapAddr_ + regAddr) = val;
  LOG(INFO) << "Reg 0x:" << std::hex << regAddr
            << " current value after updating is 0x"
            << *(volatile unsigned int*)(mapAddr_ + regAddr);
}

} // namespace facebook::fboss::platform::helpers
