/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiTajoPlatform.h"
#include "fboss/agent/hw/switch_asics/TajoAsic.h"

namespace facebook::fboss {
SaiTajoPlatform::SaiTajoPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping,
    folly::MacAddress localMac)
    : SaiHwPlatform(
          std::move(productInfo),
          std::move(platformMapping),
          localMac) {
  asic_ = std::make_unique<TajoAsic>();
}

HwAsic* SaiTajoPlatform::getAsic() const {
  return asic_.get();
}

std::optional<SaiSwitchTraits::Attributes::AclFieldList>
SaiTajoPlatform::getAclFieldList() const {
  static const std::vector<sai_int32_t> kAclFieldList = {
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6,
      SAI_ACL_TABLE_ATTR_FIELD_SRC_IP,
      SAI_ACL_TABLE_ATTR_FIELD_DST_IP,
      SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL,
      SAI_ACL_TABLE_ATTR_FIELD_DSCP,
      SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE,
      SAI_ACL_TABLE_ATTR_FIELD_TTL,
      SAI_ACL_TABLE_ATTR_FIELD_FDB_DST_USER_META,
      SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META,
      SAI_ACL_TABLE_ATTR_FIELD_NEIGHBOR_DST_USER_META,
  };
  return kAclFieldList;
}

SaiTajoPlatform::~SaiTajoPlatform() {}

} // namespace facebook::fboss
