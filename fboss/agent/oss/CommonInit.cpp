// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/CommonInit.h"

namespace facebook::fboss {

void initializeBitsflow(
    const std::optional<std::string>& /*bitsflowAclFileSuffix*/) {}

void setVersionInfo(const std::string& /*version*/) {}

void fbossFinalize() {}

} // namespace facebook::fboss
