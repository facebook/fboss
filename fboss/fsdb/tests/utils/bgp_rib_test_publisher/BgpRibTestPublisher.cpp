// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/bgp_rib_test_publisher/BgpRibTestPublisher.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include <algorithm>

namespace {
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    fsdbStateRootPath;
const auto bgpPath = fsdbStateRootPath.bgp();

using k_fsdb_model = facebook::fboss::fsdb::fsdb_model_tags::strings;

using TBgpAfi = facebook::neteng::fboss::bgp_attr::TBgpAfi;

} // namespace

namespace facebook::fboss::fsdb::test {

BgpRibTestPublisher::BgpRibTestPublisher(const std::string& publisherId)
    : fsdbPubSubMgr_(std::make_unique<FsdbPubSubManager>(publisherId)),
      stateSyncer_(
          std::make_unique<FsdbSyncManager<BgpData, true>>(
              publisherId,
              bgpPath.tokens(),
              false /* isStats */,
              getFsdbStatePubType())),
      publisherId_(publisherId) {
  XLOG(INFO) << "BgpRibTestPublisher created with publisherId: " << publisherId;
}

BgpRibTestPublisher::~BgpRibTestPublisher() {
  if (stateSyncer_) {
    stop();
  }
}

void BgpRibTestPublisher::start() {
  XLOG(INFO) << "Starting BgpRibTestPublisher";
  stateSyncer_->start();
}

void BgpRibTestPublisher::stop() {
  XLOG(INFO) << "Stopping BgpRibTestPublisher";
  if (stateSyncer_) {
    stateSyncer_->stop();
    stateSyncer_.reset();
  }
  if (fsdbPubSubMgr_) {
    fsdbPubSubMgr_.reset();
  }
}

void BgpRibTestPublisher::publishRibMap(
    const std::map<std::string, bgp_thrift::TRibEntry>& ribMap) {
  XLOG(INFO) << "Publishing full ribMap with " << ribMap.size() << " entries";

  stateSyncer_->updateState([ribMap = ribMap](const auto& oldState) {
    auto newState = oldState->clone();
    newState->template modify<k_fsdb_model::ribMap>();
    newState->template ref<k_fsdb_model::ribMap>()->fromThrift(ribMap);
    return std::move(newState);
  });

  publishedPrefixes_.clear();
  for (const auto& [prefix, _] : ribMap) {
    publishedPrefixes_.push_back(prefix);
  }
}

void BgpRibTestPublisher::publishRoute(
    const std::string& prefix,
    const bgp_thrift::TRibEntry& entry) {
  XLOG(INFO) << "Publishing route: " << prefix;

  stateSyncer_->updateState([prefix, entry](const auto& oldState) {
    auto newState = oldState->clone();
    newState->template modify<k_fsdb_model::ribMap>();
    newState->template ref<k_fsdb_model::ribMap>()->emplace(prefix, entry);
    return std::move(newState);
  });

  if (std::find(publishedPrefixes_.begin(), publishedPrefixes_.end(), prefix) ==
      publishedPrefixes_.end()) {
    publishedPrefixes_.push_back(prefix);
  }
}

void BgpRibTestPublisher::withdrawRoute(const std::string& prefix) {
  XLOG(INFO) << "Withdrawing route: " << prefix;

  stateSyncer_->updateState([prefix](const auto& oldState) {
    auto newState = oldState->clone();
    newState->template modify<k_fsdb_model::ribMap>();
    newState->template ref<k_fsdb_model::ribMap>()->remove(prefix);
    return std::move(newState);
  });

  publishedPrefixes_.erase(
      std::remove(publishedPrefixes_.begin(), publishedPrefixes_.end(), prefix),
      publishedPrefixes_.end());
}

void BgpRibTestPublisher::withdrawAllRoutes() {
  XLOG(INFO) << "Withdrawing all " << publishedPrefixes_.size() << " routes";

  const auto prefixesToWithdraw = publishedPrefixes_;
  for (const auto& prefix : prefixesToWithdraw) {
    withdrawRoute(prefix);
  }
}

bgp_thrift::TRibEntry BgpRibTestPublisher::createTestRibEntry(
    const std::string& prefix,
    const std::string& nextHop,
    int32_t localPref,
    const std::vector<int32_t>& asPath) {
  using namespace facebook::neteng::fboss::bgp::thrift;
  using namespace facebook::neteng::fboss::bgp_attr;

  bgp_thrift::TRibEntry ribEntry;

  auto network = folly::IPAddress::createNetwork(prefix);
  auto prefixBin = network.first.isV4() ? network.first.asV4().toBinary().str()
                                        : network.first.asV6().toBinary().str();

  ribEntry.prefix()->afi() =
      network.first.isV4() ? TBgpAfi::AFI_IPV4 : TBgpAfi::AFI_IPV6;
  ribEntry.prefix()->num_bits() = network.second;
  ribEntry.prefix()->prefix_bin() = prefixBin;

  auto nextHopAddr = folly::IPAddress(nextHop);
  auto nextHopBin = nextHopAddr.isV4() ? nextHopAddr.asV4().toBinary().str()
                                       : nextHopAddr.asV6().toBinary().str();

  ribEntry.best_next_hop()->afi() =
      nextHopAddr.isV4() ? TBgpAfi::AFI_IPV4 : TBgpAfi::AFI_IPV6;
  ribEntry.best_next_hop()->num_bits() = nextHopAddr.isV4() ? 32 : 128;
  ribEntry.best_next_hop()->prefix_bin() = nextHopBin;

  TBgpPath bgpPath;

  bgpPath.next_hop()->afi() =
      nextHopAddr.isV4() ? TBgpAfi::AFI_IPV4 : TBgpAfi::AFI_IPV6;
  bgpPath.next_hop()->num_bits() = nextHopAddr.isV4() ? 32 : 128;
  bgpPath.next_hop()->prefix_bin() = nextHopBin;

  bgpPath.local_pref() = localPref;
  bgpPath.is_best_path() = true;
  bgpPath.last_modified_time() =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  TAsPathSeg segment;
  segment.seg_type() = TAsPathSegType::AS_SEQUENCE;
  for (auto asn : asPath) {
    segment.asns_4_byte()->push_back(asn);
  }
  bgpPath.as_path()->push_back(segment);

  std::string groupName = "best";
  ribEntry.paths()[groupName].push_back(bgpPath);
  ribEntry.best_group() = groupName;
  ribEntry.rib_version() = 1;

  return ribEntry;
}

std::map<std::string, bgp_thrift::TRibEntry>
BgpRibTestPublisher::createTestRibMap(
    int numRoutes,
    const std::string& prefixBase,
    const std::string& nextHopBase) {
  std::map<std::string, bgp_thrift::TRibEntry> ribMap;

  for (int i = 0; i < numRoutes; ++i) {
    std::string prefix = fmt::format("{}{}::/64", prefixBase, i);
    std::string nextHop = fmt::format("{}{}", nextHopBase, i + 1);

    ribMap[prefix] = createTestRibEntry(
        prefix,
        nextHop,
        100, // localPref
        {65001, 65002}); // asPath
  }

  XLOG(INFO) << "Created test ribMap with " << numRoutes << " entries";
  return ribMap;
}

} // namespace facebook::fboss::fsdb::test
