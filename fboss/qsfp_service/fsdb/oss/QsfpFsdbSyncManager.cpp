// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/fsdb/QsfpFsdbSyncManager.h"
#include "fboss/fsdb/if/FsdbModel.h"

namespace {

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStatsRoot>
    statsRoot;

} // anonymous namespace

namespace facebook {
namespace fboss {

std::vector<std::string> QsfpFsdbSyncManager::getStatePath() {
  return stateRoot.qsfp_service().tokens();
}

std::vector<std::string> QsfpFsdbSyncManager::getStatsPath() {
  return statsRoot.qsfp_service().tokens();
}

std::vector<std::string> QsfpFsdbSyncManager::getConfigPath() {
  return stateRoot.qsfp_service().config().tokens();
}

} // namespace fboss
} // namespace facebook
