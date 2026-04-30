// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <chrono> // NOLINT(misc-include-cleaner)
#include <optional>
#include <string>

#include <fboss/bgp/if/gen-cpp2/bgp_thrift_types.h>
#ifndef IS_OSS
#include "configerator/distribution/api/ScopedConfigeratorFake.h"
#endif

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using namespace neteng::fboss::bgp_attr;
using namespace std::chrono;
#ifndef IS_OSS
using configerator::ScopedConfigeratorFake;
#endif

// Test constants.
extern const std::string kBestGroupName;
extern const std::string kDefaultGroupName;
extern const std::string kIpAddress;
extern const std::string kPeerId;
extern const std::string kPeerDescription;
extern const std::string kNextHop;
extern const std::string kRibEntryMarkersHeader;
constexpr std::uint32_t kClusterList = 0x02010101;
constexpr std::uint32_t kOriginatorId = 0x03020202;

// Common BGP peer state constants
extern const TBgpPeerState kEstablishedPeerState;
extern const TBgpPeerState kIdlePeerState;

// Common established session constants
extern const int kEstablishedRemoteAS;
extern const std::string_view kEstablishedPeerAddress;
extern const std::string_view kEstablishedRemoteBgpId;
extern const std::string_view kEstablishedDescription;
extern const int kUptime;
extern const int kEstablishedDowntime;
extern const int kEstablishedNumResets;
extern const bool kEstablishedIsGraceful;
extern const int kEstablishedPrePolicyRcvdPrefixCount;
extern const int kEstablishedPostPolicySentPrefixCount;
extern const int kEstablishedRecvUpdateMsgs;
extern const int kEstablishedSentUpdateMsgs;
extern const int kEstablishedBackpressureEvents;
extern const int kEstablishedSuppressedUpdates;

// Common idle session constants
extern const int kIdleRemoteAS;
extern const std::string_view kIdlePeerAddress;
extern const std::string_view kIdleRemoteBgpId;
extern const std::string_view kIdleDescription;
extern const bool kIdleIsGraceful;
extern const int kIdlePrePolicyRcvdPrefixCount;
extern const int kIdlePostPolicySentPrefixCount;
extern const int kIdleDowntime;
extern const int kIdleNumResets;
extern const int kIdleRecvUpdateMsgs;
extern const int kIdleSentUpdateMsgs;
extern const int kIdleBackpressureEvents;
extern const int kIdleSuppressedUpdates;

// Common description constants
extern const std::string_view kIbgpDescription;

// Common group name constants
extern const std::string_view kEstablishedGroupName;
extern const std::string_view kIdleGroupName;
extern const std::string_view kIbgpGroupName;

// Common socket write constants
extern const int64_t kEstablishedLastSocketWrite;
extern const int64_t kIdleLastSocketWrite;

TIpPrefix getPrefix(const std::string& ipAddress = kIpAddress);
TBgpPath buildPath(
    const std::string& prefixAddress = kIpAddress,
    const std::string& nextHop = kNextHop,
    const std::string& peerId = kPeerId,
    const std::string& peerDescription = kPeerDescription,
    bool setCommunity = true,
    bool setAsPath = true,
    bool setExtCommunity = false,
    const std::optional<uint32_t>& clusterList = std::nullopt,
    const std::optional<uint32_t>& originatorId = std::nullopt,
    bool setPolicy = true,
    const std::optional<uint32_t>& nextHopWeight = std::nullopt,
    const std::optional<bool> isBestPath = std::nullopt,
    bool setMed = true,
    bool setReceivedPathId = true,
    bool setPathIdToSend = true,
    bool setWeight = true,
    bool setIgpCost = true);

std::map<TIpPrefix, std::vector<TBgpPath>> getReceivedNetworks(
    const std::string& prefixAddress = kIpAddress,
    const std::string& nextHopAddress = kNextHop,
    bool setCommunity = true,
    bool setAsPath = true,
    bool setExtCommunity = false,
    const std::optional<uint32_t>& clusterList = std::nullopt,
    const std::optional<uint32_t>& originatorId = std::nullopt,
    bool setPolicy = true,
    bool setMed = true);

TRibEntry buildEntry(
    const std::string& ipAddress,
    const std::string& nextHop,
    const std::map<std::string, std::vector<TBgpPath>>& paths,
    // if set to true, there's no best path selected for advertisement
    bool omitBestPath = false);

TRibEntry buildEntry(
    const std::string& ipAddress = kIpAddress,
    const std::string& nextHop = kNextHop,
    const std::string& peerId = kPeerId,
    const std::string& peerDescription = kPeerDescription,
    const std::optional<uint32_t>& pathNextHopWeight = std::nullopt,
    // if set to true, there's no best path selected for advertisement
    bool omitBestPath = false,
    // if set to true, there's no best paths installed to FIB
    bool omitBestPaths = false);

// Mask the date from bgp table output. Since the last modified time
// duration will change every time the test is ran, we need to mask
// that part of the output to avoid conflicts with the tests.
//
// output: received output
void maskDateInOutput(std::string& output);
TRibEntry buildEntryByCommunity(const std::string& name, int asn, int value);
TBgpStreamSession buildStreamSession(
    std::string_view name = "myStreamName.01.abcd2.facebook.com",
    int uptime = 1000,
    int32_t peerId = 1,
    std::string_view peerAddr = kPeerId,
    int16_t prefixCount = 14);
} // namespace facebook::fboss
