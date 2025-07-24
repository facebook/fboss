// Copyright 2021-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss {

std::string getBcmKmodParam(const std::string& param);
bool setBcmKmodParam(const std::string& param, const std::string& val);

} // namespace facebook::fboss
