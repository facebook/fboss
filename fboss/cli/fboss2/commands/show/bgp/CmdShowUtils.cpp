/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#ifndef IS_OSS
#include <openr/common/NetworkUtil.h>
#include <openr/if/gen-cpp2/Network_types.h>
#include <openr/if/gen-cpp2/Types_types.h>
#endif

#include <utility>

#ifndef IS_OSS
#include "common/strings/StringUtil.h"
#include "common/time/TimeUtil.h"
#include "configerator/structs/neteng/fboss/push/forwarding_stack/gen-cpp2/fbpkg_map_types.h"
#else
#include "fboss/cli/fboss2/oss/NetTools.h"
#endif
#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h" // NOLINT(misc-include-cleaner)
#include "fboss/agent/AddressUtil.h"
#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
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
#ifndef IS_OSS
#include "nettools/bgplib/BgpStructs.h"
#include "nettools/bgplib/BgpUtil.h"
#include "nettools/bgplib/if/gen-cpp2/BgpStructs_types.h"
#include "nettools/skynet/if/gen-cpp2/Query_types.h"
#include "nettools/skynet/if/gen-cpp2/SkynetStructs_types.h"
#include "nettools/skynet/lib/cpp/SkynetThriftClient.h"
#endif
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using folly::IPAddress;
using neteng::fboss::bgp_attr::TAsPath;
using neteng::fboss::bgp_attr::TAsPathSegType;
using neteng::fboss::bgp_attr::TBgpAfi;

#ifdef IS_OSS
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
#endif
using nettools::bgplib::BgpAttrCommunityC;

using apache::thrift::Client;
using apache::thrift::util::enumNameSafe;
using facebook::fboss::utils::Table;
using facebook::neteng::fboss::bgp::thrift::TBgpAddPathNegotiated;
using facebook::neteng::fboss::bgp::thrift::TBgpService;
using facebook::neteng::fboss::bgp::thrift::TBgpSessionDetail;
using neteng::fboss::bgp::thrift::TBgpPeerState;

#ifndef IS_OSS
using namespace facebook::nettools::skynet;
#endif

void cmdShowVersion(CmdShowVersionTraits::RetType& binaryVersion) {
  std::cout << "Package Name: " << binaryVersion["build_package_name"]
            << std::endl;
  std::cout << "Package Info: " << binaryVersion["build_package_info"]
            << std::endl;

#ifndef IS_OSS
  auto package_version = getPackageId(
      binaryVersion["build_package_version"],
      binaryVersion["build_package_info"]);
#else
  auto package_version = binaryVersion["build_package_version"];
#endif

  std::cout << "Package Version: " << package_version << std::endl;
  std::cout << "Build Details: " << std::endl;
  std::cout << "\t Host: " << binaryVersion["build_host"] << std::endl;
  std::cout << "\t Time: " << binaryVersion["build_time"] << std::endl;
  std::cout << "\t User: " << binaryVersion["build_user"] << std::endl;
  std::cout << "\t Path: " << binaryVersion["build_path"] << std::endl;
  std::cout << "\t Platform: " << binaryVersion["build_platform"] << std::endl;
  std::cout << "\t Revision: " << binaryVersion["build_revision"] << std::endl;
}

#ifndef IS_OSS
// Convert the package version into human readable package ID
std::string getPackageId(
    const std::string& packageVersion,
    const std::string& packageInfo) {
  using facebook::config::ConfigeratorConfig;
  // Retrieve the mapped version ID to the UUID from the config file
  constexpr std::string_view kFbpkgMapConfigPath =
      "neteng/fboss/push/forwarding_stack/fbpkg_map";
  ConfigeratorConfig<facebook::fboss::FbpkgMap> fbpkgmap_cfg(
      kFbpkgMapConfigPath);

  if (auto cfgPtr = fbpkgmap_cfg.get()) {
    const auto& map_versions = cfgPtr->versions().value();
    if (const auto& it = map_versions.find(packageInfo);
        it != map_versions.end()) {
      return it->second;
    }
  }
  return packageVersion;
}
#endif

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

#ifndef IS_OSS
/* The returned output is same as the upper function printCommunities but from
 * the source of FBNet For each records, when it's going to print "Communities:
 * COMMUNITY_NAME/12345:12345". Function printFBnetCommunities() gets the
 * COMMUNITY_NAME from FBNet rather than bgpMnemonics
 */
const std::string printFBnetCommunities(
    const std::vector<TBgpCommunity>& peer_communities) {
  std::vector<std::string> communities;
  communities.reserve(peer_communities.size());
  for (const auto& comm : peer_communities) {
    communities.emplace_back(
        BgpAttrCommunityC::createBgpAttrCommunity(
            folly::to<std::string>(folly::copy(comm.community().value())))
            ->to_string());
  }

  const auto& communityNames = findCommunitiesByFBNet(communities);

  std::vector<std::string> output;
  for (const auto& [set, alias] : communityNames) {
    const std::string name = alias;
    output.emplace_back(name + "/" + folly::join(',', set));
  }
  return folly::join(", ", output);
}
#endif // IS_OSS

#ifndef IS_OSS
// Read bgp community data DesiredCommunity from FBNet through Skynet.
// DesiredCommunity has attributes: name, value, obj_uuid, network_type,
// origin_device and prefix_type, here we only access "name" and "value".
// getSkynetData() sends query to Skynet client With a query of including
// the required attribute and its require value, and the field attribute
// that will return. After response from Skynet client, getSkynetData()
// returns a vector that contains all the records that meet the requirement
std::vector<std::string> getSkynetData(
    std::string attributeValue,
    std::string searchAttribute,
    std::string field) {
  SkynetThriftClient skynet{
      "skynet_thrift",
      "fbcode:fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.cpp"};
  std::vector<std::string> resultSet;
  auto& eventBase = *EventBaseManager::get()->getEventBase();
  Expr expr1;
  expr1.name() = searchAttribute, expr1.op() = Op::EQUAL,
  expr1.values() = std::vector<std::string>({std::move(attributeValue)});
  Query query;
  query.exprs() = std::vector<Expr>({expr1});
  skynet.getObjects<SkynetStructs::DesiredCommunity>(
      [&](ClientReceiveState&& state) {
        std::vector<SkynetStructs::DesiredCommunity> results;
        try {
          SkynetThriftClient::recv_getObjects(results, state);
          if (results.size() > 0) {
            for (const auto& community : results) {
              if (field == "name") {
                resultSet.push_back(community.name().value());
              } else if (field == "value") {
                resultSet.push_back(community.value().value());
              }
            }
          }
        } catch (std::exception& e) {
          LOG(INFO) << "error while running async: " << e.what() << std::endl;
        }
      },
      query,
      {"name", "value"});
  eventBase.loop();
  return (resultSet);
}

// Get the values from communityList and get the community source from FBNet
// Community souce are stores in doubleValueResult, singleValueResult
// for further use, and can help to mock Skynet results in unit_tests
// Get the potential names by the order:
// community with 2 values (addDoubleValueCommunity)
// community with only 1 value (addSingleValueCommunity)
// community without value (addNullCommunity)
const std::map<std::vector<std::string>, std::string> findCommunitiesByFBNet(
    std::vector<std::string> communityList) {
  std::map<std::string, std::string> singleName;
  std::map<std::vector<std::string>, std::string> resultMap;
  std::vector<std::vector<std::string>> doubleValueResult;
  for (const std::string& value : communityList) {
    std::vector<std::string> nameList = getSkynetData(value, "value", "name");
    for (const std::string& name : nameList) {
      doubleValueResult.push_back({name, value});
    }
  }
  resultMap = addDoubleValueCommunity(
      std::move(resultMap), communityList, singleName, doubleValueResult);
  std::vector<std::string> singleValueResult;
  for (std::map<std::string, std::string>::iterator singleIterator =
           singleName.begin();
       singleIterator != singleName.end();
       ++singleIterator) {
    if (getSkynetData(singleIterator->first, "name", "value").size() == 1) {
      singleValueResult.push_back(singleIterator->first);
    }
  }
  resultMap = addSingleValueCommunity(
      std::move(resultMap), communityList, singleName, singleValueResult);
  return (addNullCommunity(std::move(resultMap), communityList));
}

// read the names at each value
// if it's a new name, store it to singleName
// if it's a second time, which means that it's a valid community
// remove from singleName and store it to resultMap
std::map<std::vector<std::string>, std::string> addDoubleValueCommunity(
    std::map<std::vector<std::string>, std::string> resultMap,
    std::vector<std::string>& communityList,
    std::map<std::string, std::string>& singleName,
    const std::vector<std::vector<std::string>>& doubleValueResult) {
  for (std::vector<std::string> skynetData : doubleValueResult) {
    std::string communityName = *skynetData.begin();
    std::string communityValue = *(++skynetData.begin());
    std::map<std::string, std::string>::iterator singleIterator =
        singleName.find(communityName);
    if (singleIterator != singleName.end()) {
      resultMap.insert(
          std::pair<std::vector<std::string>, std::string>(
              {singleIterator->second, communityValue}, communityName));
      singleName.erase(communityName);
      if (auto it = std::find(
              communityList.begin(),
              communityList.end(),
              singleName.find(communityName)->second);
          it != communityList.end()) {
        communityList.erase(it);
      }
      if (auto it = std::find(
              communityList.begin(), communityList.end(), communityValue);
          it != communityList.end()) {
        communityList.erase(it);
      }
    } else {
      singleName.insert(
          std::pair<std::string, std::string>(communityName, communityValue));
    }
  }
  return (resultMap);
}

// Read the names in singleValueResult if they only have 1 value,
// which means they are valid community with only one value,
// store them at resultMap
std::map<std::vector<std::string>, std::string> addSingleValueCommunity(
    std::map<std::vector<std::string>, std::string> resultMap,
    std::vector<std::string>& communityList,
    std::map<std::string, std::string>& singleName,
    const std::vector<std::string>& singleValueResult) {
  for (const std::string& name : singleValueResult) {
    resultMap.insert(
        std::pair<std::vector<std::string>, std::string>(
            {singleName.find(name)->second}, name));
    if (auto it = std::find(
            communityList.begin(),
            communityList.end(),
            singleName.find(name)->second);
        it != communityList.end()) {
      communityList.erase(it);
    }
  }
  return (resultMap);
}

// Store the names that are not valid with 1 or 2 names to NA
std::map<std::vector<std::string>, std::string> addNullCommunity(
    std::map<std::vector<std::string>, std::string> resultMap,
    std::vector<std::string>& communityList) {
  const std::string nullMessage = "(NA)";
  for (const std::string& value : communityList) {
    resultMap.insert(
        std::pair<std::vector<std::string>, std::string>({value}, nullMessage));
  }
  return (resultMap);
}
#endif // IS_OSS - End of Skynet functions

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

const std::string printExtCommunities(
    const std::vector<TBgpExtCommunity>& extCommunities) {
  if (extCommunities.empty()) {
    return "";
  }

  std::vector<std::string> commStrs;
  for (const auto& community : extCommunities) {
    const auto& communityUnion = community.u().value();
    if (communityUnion.getType() != TBgpExtCommUnion::Type::two_byte_asn) {
      commStrs.emplace_back("non-two-byte-asn");
      continue;
    }

    const auto& extCommunity = communityUnion.get_two_byte_asn();
    if (folly::copy(extCommunity.type().value()) ==
        static_cast<int>(TBgpTwoByteAsnExtCommType::LINK_BANDWIDTH_TYPE)) {
      if (folly::copy(extCommunity.sub_type().value()) ==
          static_cast<int>(
              TBgpTwoByteAsnExtCommSubType::LINK_BANDWIDTH_SUB_TYPE)) {
        auto decodeLbwExtComm = CmdLocalOptions::getInstance()->getLocalOption(
            "show_bgp_neighbors", kGar);
        if (decodeLbwExtComm.empty()) {
          decodeLbwExtComm = CmdLocalOptions::getInstance()->getLocalOption(
              "show_bgp_table", kGar);
        }
        if (decodeLbwExtComm == "true") {
#ifndef IS_OSS
          bgp::nsf_policy::NsfTeWeightEncoding encoding;
          encoding.l2_encoding() = bgp::nsf_policy::NsfL2TeWeightEncoding();
          encoding.l2_encoding()->rack_id() = 4;
          encoding.l2_encoding()->plane_id() = 4;
          encoding.l2_encoding()->remote_rack_capacity() = 8;
          encoding.l2_encoding()->spine_capacity() = 8;
          encoding.l2_encoding()->local_rack_capacity() = 8;
          auto decodedValues =
              bgp::decodeValues(extCommunity.value().value(), encoding);
          commStrs.emplace_back(
              nettools::bgplib::BgpPathC::topoInfoToStr(decodedValues));
#else
          commStrs.emplace_back(
              fmt::format(
                  "LBW{}:AS({}):RawValue({}):LbwValue({})",
                  ((folly::copy(extCommunity.type().value()) & 0x40) == 0)
                      ? "(T)"
                      : "",
                  folly::copy(extCommunity.asn().value()),
                  folly::copy(extCommunity.value().value()),
                  utils::formatBandwidth(translateLinkBandwidthValue(
                      folly::copy(extCommunity.value().value())))));
#endif
        } else {
          commStrs.emplace_back(
              fmt::format(
                  "LBW{}:AS({}):RawValue({}):LbwValue({})",
                  ((folly::copy(extCommunity.type().value()) & 0x40) == 0)
                      ? "(T)"
                      : "",
                  folly::copy(extCommunity.asn().value()),
                  folly::copy(extCommunity.value().value()),
                  utils::formatBandwidth(translateLinkBandwidthValue(
                      folly::copy(extCommunity.value().value())))));
        }
      } else {
        commStrs.emplace_back(
            fmt::format(
                "Type({}):SubType({}):AS({}):Value({})",
                folly::copy(extCommunity.type().value()),
                folly::copy(extCommunity.sub_type().value()),
                folly::copy(extCommunity.asn().value()),
                folly::copy(extCommunity.value().value())));
      }
    }
  }
  return folly::join(" ", commStrs);
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
#ifndef IS_OSS
          ? apache::thrift::util::enumNameSafe(
                static_cast<nettools::bgplib::BgpAttrOrigin>(
                    *apache::thrift::get_pointer(path.origin())))
#else
          ? formatBgpOrigin(*apache::thrift::get_pointer(path.origin()))
#endif
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

void printBgpNeighborsOutput(
    const std::vector<TBgpSession>& neighbors,
    std::ostream& out) {
  constexpr int kKeepAliveRobustCount = 3;
  if (neighbors.empty()) {
    return;
  }
  uint32_t neighborIndex = 0;
  for (auto const& neighbor : neighbors) {
    const std::string& myAddr = *neighbor.my_addr();
    const std::string& peerAddr = *neighbor.peer_addr();
    // Make a mutable copy so we can trim whitespace
    std::string description =
        folly::trimWhitespace(*neighbor.description()).str();
    apache::thrift::optional_field_ref<const TBgpSessionDetail&> details =
        neighbor.details();
    const TBgpPeer& peer = *neighbor.peer();
    const TBgpPeerState peerState = *peer.peer_state();
    const int holdTime = *peer.hold_time();

    if (myAddr.size() == 0 || !details.has_value()) {
      out << "Missing details or IP Address: " << myAddr << " " << peerAddr;
      return;
    }

    // prevent printing number with comma, e.g: 65,000
    out.imbue(std::locale("C"));
    if (neighbors.size() > 1) {
      out << "------------------------------------------------------------------"
          << std::endl;
      out << fmt::format(
                 "neighbor {} of {}", neighborIndex + 1, neighbors.size())
          << std::endl;
    }

    // TODO(michaeluw): T113736668 deprecate i32 asns field
    uint32_t remote_as = *peer.remote_as_4_byte() > 0 ? *peer.remote_as_4_byte()
                                                      : *peer.remote_as();
    out << "BGP neighbor is " << peerAddr << ", remote AS " << remote_as
        << ", ";
    out << "Confed Peer: " << (*details->confed_peer() ? "" : "not ")
        << "configured" << std::endl;
    out << "  Description: " << description << std::endl;
    out << "  BGP version 4, remote router ID "
        << IPAddress::fromLong(*details->remote_bgp_id()) << std::endl;
    if (neighbor.rib_version().has_value() && *neighbor.rib_version() > 0) {
      out << "  RIB version: " << *neighbor.rib_version() << std::endl;
    }
    out << "  Hold time is " << holdTime << "s, KeepAlive interval is "
        << holdTime / kKeepAliveRobustCount << "s" << std::endl;

    if (peerState == neteng::fboss::bgp::thrift::TBgpPeerState::ESTABLISHED) {
      out << "  BGP state is " << enumNameSafe(peerState) << ", up for "
          << utils::getPrettyElapsedTime(
                 utils::getEpochFromDuration(*neighbor.uptime()))
          << std::endl;

      if (*neighbor.num_resets() > 0) {
        out << "  Flapped " << *neighbor.num_resets() << " times, last flap "
            << utils::getPrettyElapsedTime(
                   utils::getEpochFromDuration(*neighbor.reset_time()))
            << " ago" << std::endl;
        out << "  Last reset reason: " << *neighbor.last_reset_reason()
            << std::endl;
      }
    } else {
      out << "  BGP state is " << enumNameSafe(peerState) << std::endl;
      if (peerState >= neteng::fboss::bgp::thrift::TBgpPeerState::ACTIVE &&
          details->active_time().has_value()) {
        out << "  The TCP socket has been up for "
            << utils ::getPrettyElapsedTime(
                   utils::getEpochFromDuration(*details->active_time()))
            << "." << std::endl;
      }
      if (*neighbor.num_resets() > 0) {
        out << "  Flapped " << *neighbor.num_resets() << " times";
        if (*neighbor.reset_time() > 0) {
          out << ", last went down "
              << utils ::getPrettyElapsedTime(
                     utils::getEpochFromDuration(*neighbor.reset_time()))
              << " ago";
        }
        out << std::endl;
        out << "  Last reset reason: " << *neighbor.last_reset_reason()
            << std::endl;
      }
    }

    out << "  UCMP Link Bandwidth:" << std::endl;
    out << "    Advertise: "
        << enumNameSafe(*neighbor.advertise_link_bandwidth()) << std::endl;
    out << "    Receive  : " << enumNameSafe(*neighbor.receive_link_bandwidth())
        << std::endl;
    if (neighbor.link_bandwidth_bps().has_value()) {
      out << "    Value    : "
          << utils::formatBandwidth(*neighbor.link_bandwidth_bps())
          << std::endl;
    }

    printBgpCapabilities(*details, out);
    out << "    Add Path: ";
    if (peer.add_path().has_value()) {
      out << "Configured - " << enumNameSafe(*peer.add_path());
      printAddPathCapability(*details->add_path_capabilities(), out);
      out << std::endl;
    } else {
      out << "DISABLED\n" << std::endl;
    }

    const auto rcvdPrefix = *neighbor.prepolicy_rcvd_prefix_count();
    const auto sentPrefix = *neighbor.postpolicy_sent_prefix_count();

    Table table;
    table.setHeader({"Prefix statistics:", "Sent", "Rcvd"});
    table.addRow(
        {"IPv4, IPv6 Unicast:",
         folly::to<std::string>(sentPrefix),
         folly::to<std::string>(rcvdPrefix)});

    out << table << std::endl;

    if (auto sentAnnoucementsIpv4 =
            *details->sent_update_announcements_ipv4()) {
      out << "BGP update announcements sent ipv4: " << sentAnnoucementsIpv4
          << std::endl;
    }
    if (auto sentAnnoucementsIpv6 =
            *details->sent_update_announcements_ipv6()) {
      out << "BGP update announcements sent ipv6: " << sentAnnoucementsIpv6
          << std::endl;
    }
    if (auto sentWithdrawals = *details->sent_update_withdrawals()) {
      out << "BGP update withdrawals sent: " << sentWithdrawals << std::endl;
    }
    if (auto recvdAnnoucementsIpv4 =
            *details->recv_update_announcements_ipv4()) {
      out << "BGP update announcements received ipv4: " << recvdAnnoucementsIpv4
          << std::endl;
    }
    if (auto recvdAnnoucementsIpv6 =
            *details->recv_update_announcements_ipv6()) {
      out << "BGP update announcements received ipv6: " << recvdAnnoucementsIpv6
          << std::endl;
    }
    if (auto recvdWithdrawals = *details->recv_update_withdrawals()) {
      out << "BGP update withdrawals received: " << recvdWithdrawals
          << std::endl;
    }
    if (auto enforceFirstAsRejects = *details->enforce_first_as_rejects()) {
      out << "Number of enforce-first-as validation rejections: "
          << enforceFirstAsRejects << std::endl;
    }
    // TODO(michaeluw): T113736668 deprecate i32 asns field
    uint32_t local_as = *peer.local_as_4_byte() > 0 ? *peer.local_as_4_byte()
                                                    : *peer.local_as();
    out << "Local AS is " << local_as << ", ";
    out << "local router ID " << *details->local_router_id() << std::endl;
    out << "Local TCP address is " << myAddr << ", local port is "
        << ntohs(*details->local_port()) << std::endl;
    out << "Remote TCP address is " << peerAddr << ", remote port is "
        << ntohs(*details->peer_port()) << std::endl;

    out << "TCP connection-mode is " << enumNameSafe(*details->connect_mode())
        << std::endl;
    if (auto flaps = *details->num_of_flaps()) {
      out << "Number of session terminations: " << flaps << std::endl;
    }

    if (const auto lastResetHoldTimer = *peer.lastResetHoldTimer()) {
      printTimestamp(lastResetHoldTimer, "HoldTimer last reset at", out);
    }
    if (const auto lastSentKeepAlive = *peer.lastSentKeepAlive()) {
      printTimestamp(lastSentKeepAlive, "KeepAlive last received at", out);
    }
    if (const auto lastRcvdKeepAlive = *peer.lastRcvdKeepAlive()) {
      printTimestamp(lastRcvdKeepAlive, "KeepAlive last sent at", out);
    }
    if (peerState == TBgpPeerState::ESTABLISHED) {
      if (const auto eorSentTime = *details->eor_sent_time()) {
        printTimestamp(eorSentTime, "EOR Sent at", out);
      } else {
        out << "EOR not yet sent" << std::endl;
      }
      if (const auto eorRcvdTime = *details->eor_received_time()) {
        printTimestamp(eorRcvdTime, "EOR Received at", out);
      } else {
        out << "EOR not yet received" << std::endl;
      }
    }
    if (neighbors.size() > 1) {
      out << "------------------------------------------------------------------"
          << std::endl;
    }
    out << std::endl;
    ++neighborIndex;
  }
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

void printRIBEntries(
    std::ostream& out,
    TRibEntryWithHost& data,
    const bool detail,
    const std::string& originator,
    bool fbnetsource) {
  out << "Markers: * - One of the best entries, @ - The best entry, % - Pending best path selection"
      << std::endl;
  out << "Acronyms: ASP - AS Path, LP - Local Preference, LBW - Link Bandwidth, LM - Last Modified"
      << std::endl;
  auto& entries = *data.tRibEntries();
  sortEntries(entries);
  const HostInfo hostInfo(
      *data.host(), *data.oobName(), folly::IPAddress(*data.ip()));
  const auto& prefMnemonics = getLocalPrefMnemonics(hostInfo);

  for (const auto& entry : entries) {
    const auto& bestGroup = entry.best_group().value();
    const auto& bestNextHop = entry.best_next_hop().value();
    const bool entryHasBestPath = hasBestPath(entry);
    int totalPaths = 0, ecmpPaths = 0;
    std::vector<std::string> pathsToPrint;

    for (const auto& [group, paths] : entry.paths().value()) {
      // Accounting for ecmp and total paths
      const bool isEcmpPath = (group == bestGroup);
      totalPaths += paths.size();
      if (isEcmpPath) {
        ecmpPaths += paths.size();
      }

      for (const auto& path : paths) {
        const auto& nextHop = path.next_hop().value();

        // If the TRibEntry contains a best path as indicated by bgp++'s RIB
        // then use best_path indicator; otherwise infer from best_next_hop on
        // the TBgpPath obj. This only applies for ECMP paths.
        const bool bestEcmpPath = entryHasBestPath
            ? path.is_best_path().value_or(false)
            : (isEcmpPath && bestNextHop == nextHop);

        std::string marker = (isEcmpPath) ? "*" : " ";
        marker += (bestEcmpPath) ? "@" : " ";

        std::string peerIdStr = "--";
        if (const auto& peerId = apache::thrift::get_pointer(path.peer_id())) {
          peerIdStr = IPAddress::fromBinary(
                          folly::ByteRange(
                              folly::StringPiece(peerId->prefix_bin().value())))
                          .str();
        }

        std::string peerDescriptionStr =
            path.peer_description() ? *path.peer_description() : "--";

        std::string asPathStr = "[]";
        const auto& asPath = path.as_path().value();
        if (!asPath.empty()) {
          asPathStr = printAsPath(asPath);
        }

        // Router or Originator ID - Set the following to originator id
        // if populated (i.e. route-reflector case) else use router-id.
        std::string routerOriginatorIdStr = "--";
        if (const auto& originatorId =
                apache::thrift::get_pointer(path.originator_id())) {
          routerOriginatorIdStr =
              IPAddress::fromLongHBO((uint32_t)*originatorId).str();
        } else if (
            const auto& routerId =
                apache::thrift::get_pointer(path.router_id())) {
          routerOriginatorIdStr =
              IPAddress::fromLongHBO((uint32_t)*routerId).str();
        }

        if (!originator.empty() && originator != routerOriginatorIdStr) {
          continue;
        }

        const auto& clusterList =
            apache::thrift::get_pointer(path.cluster_list());
        std::string clusterListStr = (clusterList && !clusterList->empty())
            ? fmt::format("[{}]", printClusterList(*clusterList))
            : "[]";

        const auto& nextHopStr = (folly::copy(nextHop.num_bits().value()) != 0)
            ? IPAddress::fromBinary(
                  folly::ByteRange(
                      folly::StringPiece(nextHop.prefix_bin().value())))
                  .str()
            : "SELF";

        const std::string originStr =
            (apache::thrift::get_pointer(path.origin()))
#ifndef IS_OSS
            ? apache::thrift::util::enumNameSafe(
                  static_cast<nettools::bgplib::BgpAttrOrigin>(
                      *apache::thrift::get_pointer(path.origin())))
#else
            ? formatBgpOrigin(*apache::thrift::get_pointer(path.origin()))
#endif
            : nettools::bgplib::kNullMessage;

        std::string lbwStr = "None";
        std::string extCommunitiesStr;
        if (const auto& extCommunities =
                apache::thrift::get_pointer(path.extCommunities())) {
          float linkBandwidth =
              getLinkBandwidthFromExtCommunities(*extCommunities);
          if (linkBandwidth > 0.0f) {
            lbwStr = utils::formatBandwidth(linkBandwidth);
          }
          extCommunitiesStr = printExtCommunities(*extCommunities);
        }

        std::string pathToPrint = fmt::format(
            "{} from {} ({}) via {} | LBW: {} | Origin: {} | "
            "LP: {} | ASP: {} | LM: {} | NH Weight: {} | MED: {} | ID: {} (rcvd) {} (sent) | Weight: {}{} | IgpCost: {}",
            marker,
            peerIdStr,
            peerDescriptionStr,
            nextHopStr,
            lbwStr,
            (originStr != nettools::bgplib::kNullMessage)
                ? originStr.substr(11) // Trim BGP_ORIGIN_ from origin string
                : originStr,
            printLocalPrefs(
                *apache::thrift::get_pointer(path.local_pref()), prefMnemonics),
            asPathStr,
            utils::getPrettyElapsedTime(
                folly::copy(path.last_modified_time().value()) / 1000000),
            path.next_hop_weight() ? std::to_string(*path.next_hop_weight())
                                   : "N/A",
            path.med().has_value()
                ? std::to_string(*path.med())
                : (path.multi_exit_disc().has_value()
                       ? std::to_string(*path.multi_exit_disc())
                       : "N/A"),
            path.path_id() ? std::to_string(*path.path_id()) : "N/A",
            path.path_id_to_send() ? std::to_string(*path.path_id_to_send())
                                   : "N/A",
            path.weight() ? std::to_string(*path.weight()) : "0",
            path.topologyInfo() ? fmt::format(
                                      " | {}",
                                      nettools::bgplib::BgpPathC::topoInfoToStr(
                                          *path.topologyInfo()))
                                : "",
            path.igp_cost() ? std::to_string(*path.igp_cost()) : "N/A");

        std::string communityToPrint;
#ifndef IS_OSS
        if (fbnetsource) {
          communityToPrint = printFBnetCommunities(
              *apache::thrift::get_pointer(path.communities()));
        } else {
#endif
          const HostInfo hostInfo(
              *data.host(), *data.oobName(), folly::IPAddress(*data.ip()));
          communityToPrint = printCommunities(
              *apache::thrift::get_pointer(path.communities()), hostInfo);
#ifndef IS_OSS
        }
#endif

        if (detail) {
          pathToPrint += fmt::format(
              "\n    Router/Originator: {} | ClusterList: {}"
              "\n    Communities: {}",
              routerOriginatorIdStr,
              clusterListStr,
              communityToPrint);
          if (!extCommunitiesStr.empty()) {
            pathToPrint +=
                fmt::format("\n    ExtCommunities: {}", extCommunitiesStr);
          }

          const auto& filterDescrp =
              apache::thrift::get_pointer(path.bestpath_filter_descr());
          if (filterDescrp && !filterDescrp->empty()) {
            pathToPrint += fmt::format(
                "\n    BestPath Rejection Reason: {}", *filterDescrp);
          }
        }
        pathsToPrint.emplace_back(pathToPrint);
      }
    }
    const auto entryIp = IPAddress::fromBinary(
        folly::ByteRange(
            folly::StringPiece(entry.prefix().value().prefix_bin().value())));
    const auto prefix = fmt::format(
        "{}/{}",
        entryIp.str(),
        folly::copy(entry.prefix().value().num_bits().value()));
    // Check if path selection is pending. This can happen when IGP cost
    // has changed but best-path hasn't been recalculated yet. The "%" marker
    // indicates the path-selection results displayed are in a transient state.
    // Wait for the final result after the marker is cleared.
    std::string pendingMarker;
    if (entry.path_selection_pending().has_value() &&
        entry.path_selection_pending().value()) {
      pendingMarker = "%";
    }
    out << fmt::format(
        "\n>{} {}, Selected {}/{} paths\n",
        pendingMarker,
        prefix,
        ecmpPaths,
        totalPaths);
    out << folly::join('\n', pathsToPrint) << std::endl;
  }
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
