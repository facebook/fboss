// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#ifndef IS_OSS
#include <sstream>

#include <folly/json/json.h>
#include "fboss/cli/fboss2/test/CmdShowConfigTestUtils.h"

namespace facebook::fboss::show_config_utils {

std::string getAgentRunningConfig() {
  folly::dynamic agentRunningConfig = folly::dynamic::object("version", 0)(
      "switchSettings",
      folly::dynamic::object("l2LearningMode", 1)("qcmEnable", false));

  return folly::toJson(agentRunningConfig);
}

std::string getBgpRunningConfig() {
  folly::dynamic bgpRunningConfig = folly::dynamic::object(
      "router_id", "10.170.224.128")("local_as", 65401)("local_as_4_byte", 0);

  return folly::toJson(bgpRunningConfig);
}

std::map<std::string, std::string> getConfigVersion() {
  std::map<std::string, std::string> configVer{
      {"base_hash", "ba6b9e85e2d4bdc152b5fa11d17eed58bd90b16e"},
      {"base_version_hash", "08527b0ff4c5690d9e2d764b2dd69a3b89ee786a"}};

  return configVer;
}

std::string getConfigVersionOutput() {
  std::stringstream buffer;
  auto configVerMap = getConfigVersion();
  for (auto& kv : configVerMap) {
    buffer << kv.first << ": " << kv.second << std::endl;
  }

  return buffer.str();
}

std::vector<::fboss::coop::CoopConfigPatcher> createCoopConfigPatchers() {
  ::fboss::coop::CoopConfigPatcher configPatcher1;

  configPatcher1.name() = "name1";
  configPatcher1.description() = "description1";
  configPatcher1.owner() = "owner1";

  ::fboss::coop::CoopConfigPatcher configPatcher2;
  configPatcher2.name() = "name2";
  configPatcher2.description() = "description2";
  configPatcher2.owner() = "owner2";
  return {configPatcher1, configPatcher2};
}

folly::dynamic getCoopPatcher() {
  std::string jsonStr =
      "[{\"description\" : \"description1\",\"name\" : \"name1\",\"owner\" : \"owner1\",\"persistent\" : false,\"patcher\" : {}},{\"description\" : \"description2\",\"name\" : \"name2\",\"owner\" : \"owner2\",\"persistent\" : false,\"patcher\" : {}}]";
  auto coopPatcher = folly::parseJson(jsonStr);

  return coopPatcher;
}
} // namespace facebook::fboss::show_config_utils
#endif // IS_OSS
