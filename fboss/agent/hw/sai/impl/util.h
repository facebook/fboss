// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <cstdint>
#include <string>

namespace facebook::fboss {

#if !defined(ASIC_SDK_VERSION)
#define ASIC_SDK_VERSION(major, minor, micro) \
  (100000 * (major) + 1000 * (minor) + (micro))
#endif

void findMajorMinorVersions(
    const std::string& ver,
    std::string& major,
    std::string& minor,
    int majorLength);

uint64_t getAsicSdkVersion(const std::string& sdkVersion);

} // namespace facebook::fboss
