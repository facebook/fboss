// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook {
namespace fboss {

class HwAsic {
 public:
  enum class Feature {
    INVALID,
    HOSTTABLE_FOR_HOSTROUTES,
    SPAN,
    ERSPANv4,
    ERSPANv6,
    SFLOWv4,
    SFLOWv6,
    MPLS,
    MPLS_ECMP,
    UNUSED,
  };

  virtual ~HwAsic() {}
  virtual bool isSupported(Feature) const = 0;
};

} // namespace fboss
} // namespace facebook
