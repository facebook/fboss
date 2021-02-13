/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiWedge400CPlatform.h"
#include "fboss/agent/hw/switch_asics/TajoAsic.h"
#include "fboss/agent/platforms/common/ebb_lab/Wedge400CEbbLabPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatformPort.h"

#include <algorithm>

namespace facebook::fboss {

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiHwPlatform(
          std::move(productInfo),
          std::make_unique<Wedge400CPlatformMapping>()) {
  asic_ = std::make_unique<TajoAsic>();
}

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<Wedge400CEbbLabPlatformMapping> mapping)
    : SaiHwPlatform(std::move(productInfo), std::move(mapping)) {
  asic_ = std::make_unique<TajoAsic>();
}

std::string SaiWedge400CPlatform::getHwConfig() {
  return *config()->thrift.platform_ref()->get_chip().get_asic().config_ref();
}

HwAsic* SaiWedge400CPlatform::getAsic() const {
  return asic_.get();
}

SaiWedge400CPlatform::~SaiWedge400CPlatform() {}

std::optional<SaiSwitchTraits::Attributes::AclFieldList>
SaiWedge400CPlatform::getAclFieldList() const {
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

SaiWedge400CEbbLabPlatform::SaiWedge400CEbbLabPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiWedge400CPlatform(
          std::move(productInfo),
          std::make_unique<Wedge400CEbbLabPlatformMapping>()) {}

} // namespace facebook::fboss
