// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

bool HwTestThriftHandler::isMirrorProgrammed(
    std::unique_ptr<state::MirrorFields> /*mirror*/) {
  throw FbossError("isMirrorProgrammed not implemented");
}

bool HwTestThriftHandler::isPortMirrored(
    int32_t /*port*/,
    std::unique_ptr<std::string> /*mirror*/,
    bool /*ingress*/) {
  throw FbossError("isPortMirrored not implemented");
}

bool HwTestThriftHandler::isPortSampled(
    int32_t /*port*/,
    std::unique_ptr<std::string> /*mirror*/,
    bool /*ingress*/) {
  throw FbossError("isPortSampled not implemented");
}

bool HwTestThriftHandler::isAclEntryMirrored(
    std::unique_ptr<std::string> /*aclEntry*/,
    std::unique_ptr<std::string> /*mirror*/,
    bool /*ingress*/) {
  throw FbossError("isAclEntryMirrored not implemented");
}

} // namespace facebook::fboss::utility
