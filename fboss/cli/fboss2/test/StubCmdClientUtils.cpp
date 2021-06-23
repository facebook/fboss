// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/MockClients.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss {

// stub definitions
template <>
std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> utils::createClient(
    const std::string&) {
  return std::make_unique<MockFbossCtrlAsyncClient>();
}

} // namespace facebook::fboss
