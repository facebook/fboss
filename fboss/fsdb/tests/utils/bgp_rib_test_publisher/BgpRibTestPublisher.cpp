// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/bgp_rib_test_publisher/BgpRibTestPublisher.h"

#include <folly/logging/xlog.h>
#include <algorithm>

namespace {
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    fsdbStateRootPath;
const auto bgpPath = fsdbStateRootPath.bgp();

using k_fsdb_model = facebook::fboss::fsdb::fsdb_model_tags::strings;

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

} // namespace facebook::fboss::fsdb::test
