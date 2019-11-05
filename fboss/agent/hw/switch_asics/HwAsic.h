// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook {
namespace fboss {

class HwAsic {
 public:
  enum class Feature {
    HOSTTABLE_FOR_HOSTROUTES,
    SPAN,
    ERSPANv4,
    ERSPANv6,
    SFLOWv4,
    SFLOWv6,
    MPLS,
    MPLS_ECMP,
    TRUNCATE_MIRROR_PACKET,
    TX_VLAN_STRIPPING_ON_PORT,
  };

  enum class AsicType {
    ASIC_TYPE_FAKE,

    ASIC_TYPE_TRIDENT2,
    ASIC_TYPE_TOMAHAWK,
    ASIC_TYPE_TOMAHAWK3,

    ASIC_TYPE_GIBRALTAR,
  };

  virtual ~HwAsic() {}
  virtual bool isSupported(Feature) const = 0;
  virtual AsicType getAsicType() const = 0;
};

} // namespace fboss
} // namespace facebook
