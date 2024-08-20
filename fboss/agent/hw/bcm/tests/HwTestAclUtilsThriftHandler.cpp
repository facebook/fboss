// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook {
namespace fboss {
namespace utility {

int32_t HwTestThriftHandler::getDefaultAclTableNumAclEntries() {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

  bcm_field_group_t gid =
      bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID();
  int size;

  auto rv =
      bcm_field_entry_multi_get(bcmSwitch->getUnit(), gid, 0, nullptr, &size);
  bcmCheckError(
      rv,
      "failed to get field group entry count, gid=",
      folly::to<std::string>(gid));
  return size;
}

} // namespace utility
} // namespace fboss
} // namespace facebook
