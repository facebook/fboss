// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/QsfpConfig.h"
#include "fboss/agent/FbossError.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DEFINE_string(
    qsfp_config,
    "/etc/coop/qsfp.conf",
    "The path to the local JSON configuration file");

// TODO(joseph5wu) Will deprecate this file and use symlink directly
DEFINE_string(
    qsfp_config_current,
    "/etc/coop/qsfp/current",
    "[Deprecated] The path to the local JSON configuration file");

namespace facebook::fboss {

std::unique_ptr<QsfpConfig> QsfpConfig::fromDefaultFile() {
  try {
    return fromFile(FLAGS_qsfp_config);
  } catch (const FbossError& err) {
    XLOG(WARN) << " Can't get qsfp config from " << FLAGS_qsfp_config << ": "
               << *err.message_ref() << ". Switch back to use "
               << FLAGS_qsfp_config_current;
    return fromFile(FLAGS_qsfp_config_current);
  }
}

std::unique_ptr<QsfpConfig> QsfpConfig::fromFile(folly::StringPiece path) {
  std::string configStr;
  if (!folly::readFile(path.data(), configStr)) {
    throw FbossError("unable to read ", path);
  }
  XLOG(DBG2) << "Read qsfp config from " << path;
  return fromRawConfig(std::move(configStr));
}

std::unique_ptr<QsfpConfig> QsfpConfig::fromRawConfig(
    const std::string& configStr) {
  cfg::QsfpServiceConfig qsfpConfig;

  apache::thrift::SimpleJSONSerializer::deserialize<cfg::QsfpServiceConfig>(
      configStr, qsfpConfig);

  return std::make_unique<QsfpConfig>(std::move(qsfpConfig), configStr);
}

void QsfpConfig::dumpConfig(folly::StringPiece path) const {
  folly::writeFile(raw, path.data());
}

std::unique_ptr<facebook::fboss::QsfpConfig> createEmptyQsfpConfig() {
  facebook::fboss::cfg::QsfpServiceConfig qsfpCfg;
  return createFakeQsfpConfig(qsfpCfg);
}

std::unique_ptr<facebook::fboss::QsfpConfig> createFakeQsfpConfig(
    facebook::fboss::cfg::QsfpServiceConfig& qsfpCfg) {
  return std::make_unique<facebook::fboss::QsfpConfig>(
      qsfpCfg,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(qsfpCfg));
}
} // namespace facebook::fboss
