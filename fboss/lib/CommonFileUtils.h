// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <string>

namespace facebook::fboss {

bool removeFile(const std::string& filename);

int createFile(const std::string& filename);

} // namespace facebook::fboss
