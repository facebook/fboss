/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/AlertLogger.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

// Types of alert logging
constexpr auto kFbossMiscAlert("MISC");
constexpr auto kFbossAsicAlert("ASIC");
constexpr auto kFbossServiceAlert("SERVICE");
constexpr auto kFbossPlatformAlert("PLATFORM");
constexpr auto kFbossBmcAlert("BMC");
constexpr auto kFbossKernelAlert("KERNEL");
constexpr auto kFbossPortAlert("PORT");
constexpr auto kFbossRouteAlert("ROUTE");
constexpr auto kFbossBGPAlert("BGP");

// Alert tag by type
AlertTag::AlertTag(std::string type) : type_(std::move(type)){};

MiscAlert::MiscAlert() : AlertTag(kFbossMiscAlert){};
AsicAlert::AsicAlert() : AlertTag(kFbossAsicAlert){};
ServiceAlert::ServiceAlert() : AlertTag(kFbossServiceAlert){};
PlatformAlert::PlatformAlert() : AlertTag(kFbossPlatformAlert){};
BmcAlert::BmcAlert() : AlertTag(kFbossBmcAlert){};
KernelAlert::KernelAlert() : AlertTag(kFbossKernelAlert){};
PortAlert::PortAlert() : AlertTag(kFbossPortAlert){};
RouteAlert::RouteAlert() : AlertTag(kFbossRouteAlert){};
BGPAlert::BGPAlert() : AlertTag(kFbossBGPAlert){};

// Alert param types
constexpr auto kFbossPort("port");
constexpr auto kFbossVlan("vlan");
constexpr auto kFbossIpv4Addr("ipv4");
constexpr auto kFbossIpv6Addr("ipv6");
constexpr auto kFbossMacAddr("mac");

// Alert parameter types
AlertParam::AlertParam(std::string type, std::string value)
    : type_(std::move(type)), value_(std::move(value)){};

PortParam::PortParam(std::string value) : AlertParam(kFbossPort, value){};
VlanParam::VlanParam(std::string value) : AlertParam(kFbossVlan, value){};
Ipv4Param::Ipv4Param(std::string value) : AlertParam(kFbossIpv4Addr, value){};
Ipv6Param::Ipv6Param(std::string value) : AlertParam(kFbossIpv6Addr, value){};
MacParam::MacParam(std::string value) : AlertParam(kFbossMacAddr, value){};

} // namespace facebook::fboss
