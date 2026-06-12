/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <utility>

#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#ifdef IS_OSS
#include "fboss/cli/fboss2/oss/NetTools.h"
#else
#include "neteng/fboss/bgp/cpp/lib/BgpStructs.h"
#include "neteng/fboss/bgp/cpp/lib/BgpUtil.h"
#endif

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h" // NOLINT(misc-include-cleaner)
#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/IPAddress.h"
#include "folly/Range.h"
#include "folly/String.h"
#include "folly/json.h"
#include "folly/json/dynamic.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using folly::IPAddress;
using neteng::fboss::bgp_attr::TAsPath;
using neteng::fboss::bgp_attr::TAsPathSegType;
using neteng::fboss::bgp_attr::TBgpAfi;

std::string formatBgpOrigin(int32_t origin) {
  switch (origin) {
    case 0:
      return "BGP_ORIGIN_IGP";
    case 1:
      return "BGP_ORIGIN_EGP";
    case 2:
      return "BGP_ORIGIN_INCOMPLETE";
    default:
      return fmt::format("BGP_ORIGIN_{}", origin);
  }
}

using nettools::bgplib::BgpAttrCommunityC;

using apache::thrift::Client;
using apache::thrift::util::enumNameSafe;
using facebook::fboss::utils::Table;
using facebook::neteng::fboss::bgp::thrift::TBgpAddPathNegotiated;
using facebook::neteng::fboss::bgp::thrift::TBgpService;
using facebook::neteng::fboss::bgp::thrift::TBgpSessionDetail;
using neteng::fboss::bgp::thrift::TBgpPeerState;

const std::string printCommunities(
    const std::vector<TBgpCommunity>& peer_communities,
    const HostInfo& hostInfo,
    bool isMultiLine) {
  std::vector<std::string> communities;
  communities.reserve(peer_communities.size());
  for (const auto& comm : peer_communities) {
    communities.emplace_back(
        BgpAttrCommunityC::createBgpAttrCommunity(
            folly::to<std::string>(*comm.community()))
            ->to_string());
  }

  auto communitySet = getCommunitySet(hostInfo);
  const auto& communityNames =
      nettools::bgplib::findCommunities(communities, communitySet);

  std::vector<std::string> output;
  for (const auto& [set, alias] : communityNames) {
    if (alias != nettools::bgplib::kNullMessage &&
        folly::StringPiece(alias).startsWith("COMM_")) {
      const std::string name = alias.substr(5); // Trim 'COMM_' from alias name
      output.emplace_back(name + "/" + folly::join(',', set));
    } else {
      const std::string name = alias;
      output.emplace_back(name + "/" + folly::join(',', set));
    }
  }

  return isMultiLine ? folly::join("\n", output) : folly::join(", ", output);
}

float translateLinkBandwidthValue(const uint32_t linkBandwidthValue) {
  // helper function to translate unsigned int value to float
  // background: for two-byte ASN ext community with LINK_BANDWIDTH_SUB_TYPE
  // the value of the community is an unsigned int encoding a float number
  // and this function helps translate that back to float,
  // assuming machine has single prevision binary32 float architecture.
  // Ref: IEEE-754-1985 specification and
  // BgpExtCommunityLinkBandWidthTypeC::getLBW()
  union {
    uint32_t intVal;
    float floatVal;
  } tmp;
  tmp.floatVal = 0.0f; // To avoid lint error
  tmp.intVal = linkBandwidthValue;
  return tmp.floatVal;
}

const std::string printAsPath(const TAsPath& asPath) {
  const std::map<TAsPathSegType, std::string> segFormat = {
      {TAsPathSegType::AS_SEQUENCE, "{}"},
      {TAsPathSegType::AS_CONFED_SEQUENCE, "({})"},
      {TAsPathSegType::AS_SET, "{{{}}}"},
      {TAsPathSegType::AS_CONFED_SET, "({{{}}})"}};

  std::vector<std::string> segments;
  segments.reserve(asPath.size());
  for (const auto& path : asPath) {
    // TODO(michaeluw): fully deprecate asns T113736668
    // Use asns_4_byte if set, otherwise use asns
    std::string asnsStr;
    if (!path.asns_4_byte().value().empty()) {
      asnsStr = folly::join('_', path.asns_4_byte().value());
    } else {
      asnsStr = folly::join('_', path.asns().value());
    }
    segments.emplace_back(
        fmt::format(
            fmt::runtime(segFormat.at(folly::copy(path.seg_type().value()))),
            std::move(asnsStr)));
  }
  return folly::join('_', segments);
}

const std::string printLocalPrefs(
    int localPref,
    const std::map<int, std::string>& mnemonics) {
  if (nettools::bgplib::isAristaDevice()) {
    return folly::to<std::string>(localPref);
  }

  std::string prefName = nettools::bgplib::kNullMessage;
  const auto& prefMnemonic = mnemonics.find(localPref);
  if (prefMnemonic != mnemonics.end()) {
    // Strip away LOCALPREF_ from the mnemonic name
    static const std::string kLocalPref = "LOCALPREF_";
    if (folly::StringPiece(prefMnemonic->second).startsWith(kLocalPref)) {
      prefName = prefMnemonic->second.substr(kLocalPref.size());
    } else {
      prefName = prefMnemonic->second;
    }
  }
  return fmt::format("{}/{}", prefName, localPref);
}

const std::string printClusterList(const std::vector<long>& clusterList) {
  std::vector<std::string> clusters;
  clusters.reserve(clusterList.size());
  for (const auto& cluster : clusterList) {
    clusters.emplace_back(IPAddress::fromLongHBO(cluster).str());
  }
  return folly::join(',', clusters);
}

namespace {
// Static state for mnemonic caching - defined here to be accessible by both
// functions
struct MnemonicCache {
  bool firstTimeRetreivingMnemonics = true;
  std::map<int, std::string> prefsMnemonics;
  bool firstTimeGettingCommunities = true;
  std::map<std::set<std::string>, std::string> communitySetMap;
};

MnemonicCache& getMnemonicCache() {
  static MnemonicCache cache;
  return cache;
}
} // namespace

std::map<std::set<std::string>, std::string> getCommunitySet(
    const HostInfo& hostInfo) {
  auto& cache = getMnemonicCache();

  if (!cache.firstTimeGettingCommunities) {
    return cache.communitySetMap;
  }

  cache.firstTimeGettingCommunities = false;
  const auto bgp_mnemonics = getLocalBgpConfig(hostInfo);
  if (bgp_mnemonics.find("communities") == bgp_mnemonics.items().end()) {
    return cache.communitySetMap;
  }
  for (const auto& mnemonic : bgp_mnemonics.at("communities")) {
    std::set<std::string> communities;
    if (mnemonic.find("communities") == mnemonic.items().end()) {
      continue;
    }
    for (const auto& community : mnemonic.at("communities")) {
      communities.insert(community.asString());
    }
    if (mnemonic.find("name") == mnemonic.items().end()) {
      cache.communitySetMap[communities] = nettools::bgplib::kNullMessage;
    } else {
      cache.communitySetMap[communities] = mnemonic.at("name").asString();
    }
  }
  return cache.communitySetMap;
}

folly::dynamic getLocalBgpConfig(const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<TBgpService>>(hostInfo);
  std::string config;
  client->sync_getRunningConfig(config);
  return folly::parseJson(config);
}

const std::map<int, std::string> getLocalPrefMnemonics(
    const HostInfo& hostInfo) {
  auto& cache = getMnemonicCache();

  if (!cache.firstTimeRetreivingMnemonics) {
    return cache.prefsMnemonics;
  }

  cache.firstTimeRetreivingMnemonics = false;
  const auto bgp_mnemonics = getLocalBgpConfig(hostInfo);

  if (bgp_mnemonics.empty()) {
    return cache.prefsMnemonics;
  }

  if (bgp_mnemonics.find("localprefs") == bgp_mnemonics.items().end()) {
    return cache.prefsMnemonics;
  }
  for (const auto& mnemonic : bgp_mnemonics.at("localprefs")) {
    if (mnemonic.find("localpref") == mnemonic.items().end()) {
      continue;
    }
    cache.prefsMnemonics[mnemonic.at("localpref").asInt()] =
        mnemonic.at("name").asString();
  }
  return cache.prefsMnemonics;
}

void resetBgpMnemonicCaches() {
  auto& cache = getMnemonicCache();
  cache.firstTimeRetreivingMnemonics = true;
  cache.prefsMnemonics.clear();
  cache.firstTimeGettingCommunities = true;
  cache.communitySetMap.clear();
}

const std::vector<std::string> printRoutesInformation(
    const std::map<TIpPrefix, std::vector<TBgpPath>>& networks,
    const HostInfo& hostInfo,
    bool showPolicy) {
  std::vector<std::string> routes;
  const auto& prefMnemonics = getLocalPrefMnemonics(hostInfo);
  for (const auto& [ipPrefix, bgpPaths] : networks) {
    for (const auto& path : bgpPaths) {
      const auto ip_address = IPAddress::fromBinary(
          folly::ByteRange(folly::StringPiece(ipPrefix.prefix_bin().value())));
      const auto prefix = fmt::format(
          "{}/{}", ip_address.str(), folly::copy(ipPrefix.num_bits().value()));

      std::string nextHop;
      if (path.next_hop().value().prefix_bin().value().empty()) {
        nextHop = (folly::copy(ipPrefix.afi().value()) == TBgpAfi::AFI_IPV4)
            ? "0.0.0.0"
            : "::";
      } else {
        nextHop = IPAddress::fromBinary(
                      folly::ByteRange(
                          folly::StringPiece(
                              path.next_hop().value().prefix_bin().value())))
                      .str();
      }

      std::string routerOriginatorId = "  --  ";
      if (const auto& originatorId =
              apache::thrift::get_pointer(path.originator_id())) {
        routerOriginatorId = IPAddress::fromLongHBO(*originatorId).str();
      } else if (
          const auto& routerId =
              apache::thrift::get_pointer(path.router_id())) {
        routerOriginatorId = IPAddress::fromLongHBO(*routerId).str();
      }

      const auto& clusterList =
          apache::thrift::get_pointer(path.cluster_list());
      std::string clusterListStr = (clusterList && !clusterList->empty())
          ? fmt::format("[{}]", printClusterList(*clusterList))
          : "[]";

      const std::string communitiesStr =
          (apache::thrift::get_pointer(path.communities()))
          ? printCommunities(
                *apache::thrift::get_pointer(path.communities()), hostInfo)
          : "";
      const std::string extCommunitiesStr =
          (apache::thrift::get_pointer(path.extCommunities()))
          ? printExtCommunities(
                *apache::thrift::get_pointer(path.extCommunities()))
          : "";
      const std::string localPrefsStr =
          (apache::thrift::get_pointer(path.local_pref()))
          ? printLocalPrefs(
                *apache::thrift::get_pointer(path.local_pref()), prefMnemonics)
          : "";
      const std::string originStr = (apache::thrift::get_pointer(path.origin()))
          ? formatBgpOrigin(*apache::thrift::get_pointer(path.origin()))
          : nettools::bgplib::kNullMessage;

      std::string routeInformation = fmt::format(
          "---\n"
          "Network: {}\n"
          "Nexthop: {}\n"
          "Router/OriginatorId: {}\n"
          "ClusterList: {}\n"
          "Communities: {}\n"
          "ExtCommunities: {}\n"
          "AsPath: {}\n"
          "LocalPref: {}\n"
          "Origin: {}\n"
          "MED: {}\n"
          "LastModified: {}",
          prefix,
          nextHop,
          routerOriginatorId.data(),
          clusterListStr,
          communitiesStr,
          extCommunitiesStr,
          printAsPath(path.as_path().value()),
          localPrefsStr,
          (originStr != nettools::bgplib::kNullMessage)
              ? originStr.substr(11) // Trim BGP_ORIGIN_ from origin string
              : originStr,
          path.med().has_value()
              ? std::to_string(*path.med())
              : (path.multi_exit_disc().has_value()
                     ? std::to_string(*path.multi_exit_disc())
                     : "Not set"),
          (folly::copy(path.last_modified_time().value()))
              ? utils::parseTimeToTimeStamp(
                    folly::copy(path.last_modified_time().value()) / 1000)
              : "Not set");

      if (showPolicy) {
        std::string policyStr = "Not set";
        if (apache::thrift::get_pointer(path.policy_name()) &&
            !apache::thrift::get_pointer(path.policy_name())->empty()) {
          policyStr = *apache::thrift::get_pointer(path.policy_name());
        }
        routeInformation += fmt::format("\nPolicy: {}", policyStr);
      }
      routes.emplace_back(routeInformation);
    }
  }
  return routes;
}

void printBgpCapabilities(const TBgpSessionDetail& details, std::ostream& out) {
  out << "  Neighbor Capabilities:" << std::endl;
  if (folly::copy(details.ipv4_unicast().value())) {
    out << "    Multiprotocol IPv4 Unicast: negotiated" << std::endl;
  }
  if (folly::copy(details.ipv6_unicast().value())) {
    out << "    Multiprotocol IPv6 Unicast: negotiated" << std::endl;
  }
  if (folly::copy(details.rr_client().value())) {
    out << "    Route Refresh: advertised" << std::endl;
  }
  if (const auto remoteRestartTime =
          folly::copy(details.gr_remote_restart_time().value())) {
    out << "    Graceful Restart: received, peer restart-time is "
        << remoteRestartTime << "s" << std::endl;
  } else {
    out << "    Graceful Restart: not received" << std::endl;
  }
  if (const auto restartTime = folly::copy(details.gr_restart_time().value())) {
    out << "    Graceful Restart: sent, local restart-time is " << restartTime
        << "s" << std::endl;
  } else {
    out << "    Graceful Restart: NOT sent" << std::endl;
  }
}

void printAddPathCapability(
    const std::vector<TBgpAddPathNegotiated>& capabilities,
    std::ostream& out) {
  std::vector<std::string> negotiations;
  negotiations.reserve(capabilities.size());
  for (const auto& capability : capabilities) {
    const auto afi = enumNameSafe(folly::copy(capability.afi().value()));
    const auto addPathCapability =
        enumNameSafe(folly::copy(capability.add_path().value()));
    negotiations.emplace_back(
        fmt::format("{} {}", afi, addPathCapability).data());
  }
  if (!negotiations.empty()) {
    out << ", Negotiated - " << folly::join(", ", negotiations) << std::endl;
  }
}

void printRouteHeader(std::string& out, bool detailed) {
  // Helper function to construct print lines of header for advertised and
  // received rooutes. Add marker information
  out =
      "Markers: * - Best entries (used for forwarding), @ - Entry used to advertise across area\n";
  if (!detailed) {
    out +=
        "Acronyms: SP - Source Preference, PP - Path Preference, D - Distance\n"
        "          MN - Min-Nexthops\n";
  }
}

const std::string formatBytes(size_t n) {
  const std::vector<std::string> suffix{"B", "KB", "MB", "GB"};
  size_t i = 0;
  double out = n;
  for (i = 0; i < 4 && n >= 1024; i++) {
    out = n / 1024.0;
    n /= 1024;
  }
  out = (static_cast<int>(out * 100.0)) / 100.0;
  return fmt::format("{} {}", out, suffix[i]);
}

timeval splitFractionalSecondsFromTimer(const long timer) {
  constexpr int kOneSecond = 1000;
  struct timeval tv;
  tv.tv_sec = timer / kOneSecond; // get total amount of seconds
  tv.tv_usec = (timer % kOneSecond); // get fractional seconds

  tv.tv_usec = lrint(tv.tv_usec); // round fs to nearest decimal
  return tv;
}

void printTimestamp(
    const long timeToParse,
    const std::string& outputMessage,
    std::ostream& out) {
  constexpr std::string_view kTimeFormat = "%Y-%m-%d %H:%M:%S %Z";
  const auto splitTime = splitFractionalSecondsFromTimer(timeToParse);
  std::string formattedTime; // Timestamp storage
  stringFormatTimestamp(splitTime.tv_sec, kTimeFormat.data(), &formattedTime);

  // stringFormatTimestamp returns a formatted time like the following:
  //      2021-10-27 00:00:06 PDT
  //
  // In order to add the fractional seconds, we find the last space before the
  // timestamp's timezone and concatenate the fractional seconds.
  //      2021-10-27 00:00:06 PDT -> 2021-10-27 00:00:06.475 PDT
  int index = formattedTime.find_last_of(' ');
  out << outputMessage
      << fmt::format(
             " {}.{} {}",
             formattedTime.substr(0, index), // Date and time
             splitTime.tv_usec, // fractional seconds
             formattedTime.substr(index + 1)) // timezone
      << std::endl;
}

std::map<TIpPrefix, std::vector<TBgpPath>> filterNetworks(
    const std::map<TIpPrefix, std::vector<TBgpPath>>& networks,
    const std::vector<std::string>& prefixes) {
  std::vector<TIpPrefix> prefixList;
  for (const auto& ip : prefixes) {
    // Split the Ip Address and subnet mask
    std::size_t maskIndex = ip.find_last_of('/');
    const auto address = folly::IPAddress(ip.substr(0, maskIndex));
    const auto binaryAddress = facebook::network::toBinaryAddress(address);

    TIpPrefix ipPrefix;
    ipPrefix.prefix_bin() = binaryAddress.addr().value().toStdString();
    ipPrefix.num_bits() = folly::to<int>(ip.substr(maskIndex + 1));
    ipPrefix.afi() = (address.isV4()) ? TBgpAfi::AFI_IPV4 : TBgpAfi::AFI_IPV6;

    prefixList.emplace_back(ipPrefix);
  }

  std::map<TIpPrefix, std::vector<TBgpPath>> filteredNetworks;
  for (const auto& prefix : prefixList) {
    if (const auto& it = networks.find(prefix); it != networks.end()) {
      filteredNetworks.try_emplace(it->first, it->second);
    }
  }
  return filteredNetworks;
}

const std::map<TIpPrefix, std::vector<TBgpPath>> getRejectedNetworks(
    const std::map<TIpPrefix, std::vector<TBgpPath>>& receivedNetworks,
    const std::map<TIpPrefix, std::vector<TBgpPath>>& acceptedNetworks) {
  std::map<TIpPrefix, std::vector<TBgpPath>> rejectedNetworks;
  for (const auto& [route, path] : receivedNetworks) {
    if (acceptedNetworks.find(route) == acceptedNetworks.end()) {
      rejectedNetworks.try_emplace(route, path);
    }
  }
  return rejectedNetworks;
}

const std::map<TIpPrefix, std::vector<TBgpPath>> vectorizePaths(
    const std::map<TIpPrefix, TBgpPath>& networks) {
  std::map<TIpPrefix, std::vector<TBgpPath>> vectorizedNetworks;
  for (const auto& [route, path] : networks) {
    std::vector<TBgpPath> vectorizedPath = {path};
    vectorizedNetworks.try_emplace(route, std::move(vectorizedPath));
  }
  return vectorizedNetworks;
}

void sortEntries(std::vector<TRibEntry>& entries) {
  // Sort the entries by their prefix
  std::sort(
      entries.begin(),
      entries.end(),
      [](const TRibEntry& entryA, const TRibEntry& entryB) {
        return entryA.prefix().value() < entryB.prefix().value();
      });

  for (auto& entry : entries) {
    for (auto& [group, path] : *entry.paths()) {
      // Sort RIB entries by nexthop
      std::sort(
          path.begin(),
          path.end(),
          [](const TBgpPath& pathA, const TBgpPath& pathB) {
            return pathA.next_hop().value() < pathB.next_hop().value();
          });
    }
  }
}

float getLinkBandwidthFromExtCommunities(
    const std::vector<TBgpExtCommunity>& extCommunities) {
  for (const auto& comm : extCommunities) {
    const auto& commUnion = comm.u().value();

    if (commUnion.getType() != TBgpExtCommUnion::Type::two_byte_asn) {
      // not a two-byte ASN
      continue;
    }
    const auto& asn = commUnion.get_two_byte_asn();

    if (folly::copy(asn.type().value()) !=
        static_cast<int>(TBgpTwoByteAsnExtCommType::LINK_BANDWIDTH_TYPE)) {
      // not a link bandwidth value
      continue;
    }

    if ((folly::copy(asn.sub_type().value()) ==
         static_cast<int>(
             TBgpTwoByteAsnExtCommSubType::LINK_BANDWIDTH_SUB_TYPE))) {
      return translateLinkBandwidthValue(folly::copy(asn.value().value()));
    }
  }
  return -1.0f;
}

bool hasBestPath(const TRibEntry& entry) {
  for (const auto& [group, paths] : entry.paths().value()) {
    for (const auto& path : paths) {
      if (path.is_best_path().value_or(false /* default */)) {
        return true;
      }
    }
  }
  return false;
}

bool isValidCommunity(std::string_view communityName) {
  const size_t delimiter = communityName.find(':');
  if (delimiter != std::string::npos) {
    const auto& it = communityName.begin();
    return std::all_of(it, it + delimiter - 1, ::isdigit) &&
        std::all_of(it + delimiter + 1, communityName.end(), ::isdigit);
  }
  return false;
}

const std::vector<std::string> communityNameToValue(
    const std::string& communityName,
    const HostInfo& hostInfo) {
  static const auto& bgpMnemonics = getLocalBgpConfig(hostInfo);

  // First Case: The community name comes in format 'asn:value'.
  // if so, we need to make sure the values are valid (integers).
  if (isValidCommunity(communityName)) {
    return {communityName};
  } else {
    // Second Case: The community name is sometimes prefixed with
    // COMM_.
    // example: COMM_ADMIT_CONTROLLER or
    //          ADMIT_CONTROLLER
    // If so we need to make sure that the name is present on bgp mnemonics
    // to return the right community set values.
    std::vector<std::string> communitySet;
    const auto& communities = bgpMnemonics.at("communities");
    const std::string commNamePrefix = "COMM_";
    for (const auto& community : communities) {
      if (community["name"] == communityName ||
          community["name"] ==
              fmt::format("{}{}", commNamePrefix, communityName)) {
        for (const auto& curCommunity : community["communities"]) {
          communitySet.emplace_back(curCommunity.asString());
        }
        return communitySet;
      }
    }
  }
  throw std::invalid_argument(
      fmt::format("Unknown Community Name: {}", communityName));
}

const std::string communityToString(const TBgpCommunity& community) {
  return fmt::format(
      "{}:{}",
      folly::copy(community.asn().value()),
      folly::copy(community.value().value()));
}

void stringFormatTimestamp(
    time_t timestamp,
    const char* format,
    std::string* output) {
  std::tm tm_info;
  localtime_r(&timestamp, &tm_info);
  std::ostringstream oss;
  oss << std::put_time(&tm_info, format);
  *output = oss.str();
}

} // namespace facebook::fboss
