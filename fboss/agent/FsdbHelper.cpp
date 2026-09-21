// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FsdbHelper.h"
#include "fboss/fsdb/if/FsdbModel.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

std::vector<std::string> fsdbAgentDataSwitchStateRootPath() {
  static const thriftpath::RootThriftPath<
      facebook::fboss::fsdb::FsdbOperStateRoot>
      kFsdbStateRootPath;
  return kFsdbStateRootPath.agent().switchState().tokens();
}

std::string getOperPath(const std::vector<std::string>& tokens) {
  std::ostringstream joined;
  std::copy(
      tokens.begin(),
      tokens.end(),
      std::ostream_iterator<std::string>(joined, "/"));
  return joined.str();
}

void printOperDeltaPaths(const fsdb::OperDelta& operDelta) {
  for (const auto& change : *operDelta.changes()) {
    if (change.newState() && change.oldState()) {
      XLOG(INFO) << "~ : " << getOperPath(*change.path()->raw());
    } else if (change.newState()) {
      XLOG(INFO) << "+ : " << getOperPath(*change.path()->raw());
    } else if (change.oldState()) {
      XLOG(INFO) << "- : " << getOperPath(*change.path()->raw());
    }
  }
}

} // namespace facebook::fboss
