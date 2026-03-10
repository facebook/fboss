// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

void printShowConfigMaps(
    CmdShowConfigVersionTraits::RetType& printMap,
    std::ostream& out) {
  for (auto& kv : printMap) {
    out << kv.first << ": " << kv.second << std::endl;
  }
}

std::string getConfigHistory(
    const HostInfo& hostInfo,
    std::string config_type) {
  auto cmdToRun = utils::getCmdToRun(
      hostInfo.getName(),
      folly::to<std::string>(
          "git -C /etc/coop log --graph --boundary "
          "--pretty=tformat:\'%C(yellow)%h%Creset "
          "%C(blue)%cd%C(reset) %C(red)%an%C(reset)%C(green)%d%Creset %s\' "
          "--color=always --all --date=format:\'%F %T\' -- /etc/coop/",
          config_type,
          "/current"));
  XLOG(DBG2) << "Run cmd: " << cmdToRun;

  return utils::runCmd(cmdToRun);
}

} // namespace facebook::fboss
