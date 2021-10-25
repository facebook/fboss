// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <array>
#include <string>

namespace facebook::fboss::platform::helpers {
constexpr uint32_t MAP_SIZE = 4096;
constexpr uint32_t MAP_MASK = MAP_SIZE - 1;

std::string execCommand(const std::string& cmd, int& out_exitStatus);
uint32_t mmap_read(uint32_t address, char acc_type);
int mmap_write(uint32_t address, char acc_type, uint32_t val);
} // namespace facebook::fboss::platform::helpers
