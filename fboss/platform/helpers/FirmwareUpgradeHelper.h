// Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <folly/system/Shell.h>
#include <glog/logging.h>
#include <stdint.h>
#include <stdlib.h>
#include <filesystem>
#include <iostream>
#include "fboss/platform/helpers/Utils.h"

namespace facebook::fboss::platform::helpers {

void checkCmdStatus(const std::string&, int);
bool isFilePresent(std::string);
void i2cRegWrite(std::string, std::string, std::string, uint8_t);
void jamUpgrade(std::string, std::string, std::string);
void xappUpgrade(std::string, std::string, std::string);
void flashromBiosUpgrade(std::string, std::string, std::string, std::string);
} // namespace facebook::fboss::platform::helpers
