// Copyright 2021-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss {

std::string getBcmKmodParam(std::string param);
bool setBcmKmodParam(std::string param, std::string val);

} // namespace facebook::fboss
