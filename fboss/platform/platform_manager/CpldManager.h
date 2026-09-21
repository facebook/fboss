// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "fboss/platform/platform_manager/I2cAddr.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/platform_manager/uapi/fbcpld-ioctl.h"

namespace facebook::fboss::platform::platform_manager {

// Construct char dev path: /dev/fbcpld-<bus>-<addr>
std::string getCpldCharDevPath(uint16_t busNum, const I2cAddr& addr);

// Map of valid CPLD sysfs attribute flag names to ioctl flag values.
const std::unordered_map<std::string, uint32_t>& getCpldFlagMap();

// Build the ioctl request struct from CpldSysfsAttr thrift objects.
struct fbcpld_ioctl_create_request buildCpldIoctlRequest(
    const std::vector<CpldSysfsAttr>& cpldSysfsAttrs);

// Create sysfs attributes on a CPLD device.
// Opens the char device, converts attrs to ioctl struct, issues ioctl.
void createCpldSysfsAttrs(
    uint16_t busNum,
    const I2cAddr& addr,
    const std::vector<CpldSysfsAttr>& cpldSysfsAttrs);

} // namespace facebook::fboss::platform::platform_manager
