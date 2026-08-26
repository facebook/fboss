// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/lib/if/LinkQsfpTestMachineInfo.h"

namespace facebook::fboss::utility {

// FbWhoAmI and NetWhoAmI are Meta-internal. The Scuba dump these feed is also
// Meta-internal, so leaving these empty in OSS costs nothing.
std::string getMachineName() {
  return "";
}

std::string getMachineType() {
  return "";
}

} // namespace facebook::fboss::utility
