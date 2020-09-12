// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"

#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss {

void SaiBcmPlatform::initWedgeLED(int led, folly::ByteRange code) {
  if (code.empty()) {
    XLOG(WARNING) << "attempting to initialize led with zero code";
    return;
  }
  SaiSwitch* saiSwitch = static_cast<SaiSwitch*>(SaiPlatform::getHwSwitch());
  auto switchID = saiSwitch->getSwitchId();

  // disable LED processor
  std::array<sai_uint32_t, 2> disable{};
  disable[0] = led;
  disable[1] = 0; // disable
  SaiApiTable::getInstance()->switchApi().setAttribute(
      switchID,
      SaiSwitchTraits::Attributes::LedReset{
          std::vector<sai_uint32_t>(disable.begin(), disable.end())});

  // load coode
  std::array<sai_uint32_t, 4> program{};
  for (auto offset = 0; offset < code.size(); offset++) {
    program[0] = led;
    // TODO(pshaikh): define symbols to replace magic number usage
    program[1] = 2; // program bank
    program[2] = offset;
    program[3] = code[offset];
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switchID,
        SaiSwitchTraits::Attributes::Led{
            std::vector<sai_uint32_t>(program.begin(), program.end())});
  }

  // enable LED processor
  std::array<sai_uint32_t, 2> enable{};
  enable[0] = led;
  enable[1] = 1; // enable
  SaiApiTable::getInstance()->switchApi().setAttribute(
      switchID,
      SaiSwitchTraits::Attributes::LedReset{
          std::vector<sai_uint32_t>(enable.begin(), enable.end())});
}
} // namespace facebook::fboss
