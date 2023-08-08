// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss {

void findMajorMinorVersions(
    const std::string& ver,
    std::string& major,
    std::string& minor,
    int majorLength);

} // namespace facebook::fboss
