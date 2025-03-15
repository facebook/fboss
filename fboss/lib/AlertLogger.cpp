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

namespace facebook::fboss {

// Prefix for fboss alert tag
constexpr auto kFbossAlertPrefix("FBOSS_EVENT");
constexpr auto kLinkSnapshotAlertPrefix("LINK_SNAPSHOT_EVENT");
constexpr auto kBgpAlertPrefix("BGP_EVENT");
constexpr auto kMkaAlertPrefix("MKA_EVENT");

// Sub-types for alert logging
constexpr auto kFbossMiscAlert("MISC");
constexpr auto kFbossAsicAlert("ASIC");
constexpr auto kFbossServiceAlert("SERVICE");
constexpr auto kFbossPlatformAlert("PLATFORM");
constexpr auto kFbossBmcAlert("BMC");
constexpr auto kFbossKernelAlert("KERNEL");
constexpr auto kFbossPortAlert("PORT");
constexpr auto kFbossRouteAlert("ROUTE");
constexpr auto kFbossLinkSnapshotAlert("LINK_SNAPSHOT");
constexpr auto kFbossQsfpTransceiverValidationAlert("TRANSCEIVER_VALIDATION");

// Alert tag by type
AlertTag::AlertTag(std::string prefix, std::string sub_type)
    : prefix_(std::move(prefix)), sub_type_(std::move(sub_type)) {}

MiscAlert::MiscAlert() : AlertTag(kFbossAlertPrefix, kFbossMiscAlert) {}
AsicAlert::AsicAlert() : AlertTag(kFbossAlertPrefix, kFbossAsicAlert) {}
ServiceAlert::ServiceAlert()
    : AlertTag(kFbossAlertPrefix, kFbossServiceAlert) {}
PlatformAlert::PlatformAlert()
    : AlertTag(kFbossAlertPrefix, kFbossPlatformAlert) {}
BmcAlert::BmcAlert() : AlertTag(kFbossAlertPrefix, kFbossBmcAlert) {}
KernelAlert::KernelAlert() : AlertTag(kFbossAlertPrefix, kFbossKernelAlert) {}
PortAlert::PortAlert() : AlertTag(kFbossAlertPrefix, kFbossPortAlert) {}
RouteAlert::RouteAlert() : AlertTag(kFbossAlertPrefix, kFbossRouteAlert) {}
BGPAlert::BGPAlert() : AlertTag(kBgpAlertPrefix) {}
MKAAlert::MKAAlert() : AlertTag(kMkaAlertPrefix) {}
LinkSnapshotAlert::LinkSnapshotAlert()
    : AlertTag(kLinkSnapshotAlertPrefix, kFbossLinkSnapshotAlert) {}
TransceiverValidationAlert::TransceiverValidationAlert()
    : AlertTag(kFbossAlertPrefix, kFbossQsfpTransceiverValidationAlert) {}

// Alert param types
constexpr auto kFbossPort("port");
constexpr auto kFbossVlan("vlan");
constexpr auto kFbossIpv4Addr("ipv4");
constexpr auto kFbossIpv6Addr("ipv6");
constexpr auto kFbossMacAddr("mac");
constexpr auto kFbossLinkSnapshot("linkSnapshot");

// Alert parameter types
AlertParam::AlertParam(std::string type, std::string value)
    : type_(std::move(type)), value_(std::move(value)) {}

PortParam::PortParam(std::string value) : AlertParam(kFbossPort, value) {}
VlanParam::VlanParam(std::string value) : AlertParam(kFbossVlan, value) {}
Ipv4Param::Ipv4Param(std::string value) : AlertParam(kFbossIpv4Addr, value) {}
Ipv6Param::Ipv6Param(std::string value) : AlertParam(kFbossIpv6Addr, value) {}
MacParam::MacParam(std::string value) : AlertParam(kFbossMacAddr, value) {}
LinkSnapshotParam::LinkSnapshotParam(std::string value)
    : AlertParam(kFbossLinkSnapshot, value) {}

} // namespace facebook::fboss
