// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss::platform::fw_util {

/*
 * exec command, return contents of stdout and fill in exit status
 */
std::string execCommandUnchecked(const std::string& cmd, int& exitStatus);
/*
 * exec command, return contents of stdout. Throw on command failing (exit
 * status != 0)
 */
std::string execCommand(const std::string& cmd);
uint32_t mmap_read(uint32_t address, char acc_type);
int mmap_write(uint32_t address, char acc_type, uint32_t val);
void showDeviceInfo();

/*
 * Search Flash Type from flashrom output, e.g. flashrom -p internal
 * If it can not find, return empty string
 */
std::string getFlashType(const std::string& str);

uint64_t nowInSecs();
} // namespace facebook::fboss::platform::fw_util
