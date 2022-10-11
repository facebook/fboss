// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <glog/logging.h>

#include "fboss/platform/fw_util/firmware_helpers/Utils.h"

namespace facebook::fboss::platform::fw_util {
std::string readSysfs(std::string);
void checkCmdStatus(const std::string&, int);
bool isFilePresent(std::string);
void i2cRegWrite(std::string, std::string, std::string, uint8_t);
void jamUpgrade(std::string, std::string, std::string);
void xappUpgrade(std::string, std::string, std::string);
void flashromBiosUpgrade(std::string, std::string, std::string, std::string);
int runCmd(const std::string&);
std::string i2cRegRead(std::string, std::string, std::string);
std::string toLower(std::string);
void printUsage(std::string);
bool checkFileExists(std::string);
} // namespace facebook::fboss::platform::fw_util
