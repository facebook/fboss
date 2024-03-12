// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/FsdbConfig.h"
#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <optional>
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

DEFINE_string(
    fsdb_config,
    "/etc/coop/fsdb/current",
    "The path to the local JSON configuration file");

namespace facebook::fboss::fsdb {

// static
std::shared_ptr<FsdbConfig> FsdbConfig::fromDefaultFile() {
  return fromFile(FLAGS_fsdb_config);
}

// static
std::shared_ptr<FsdbConfig> FsdbConfig::fromFile(folly::StringPiece path) {
  std::string raw;
  if (!folly::readFile(path.data(), raw)) {
    FsdbException e;
    e.message_ref() = folly::to<std::string>("unable to read ", path);
    throw e;
  }

  return fromRaw(raw);
}

// static
std::shared_ptr<FsdbConfig> FsdbConfig::fromRaw(const std::string& raw) {
  Config thrift;
  apache::thrift::SimpleJSONSerializer::deserialize<Config>(raw, thrift);
  return std::make_shared<FsdbConfig>(std::move(thrift));
}

std::string FsdbConfig::configRaw() const {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(thrift_);
}

const std::reference_wrapper<const PathConfig> FsdbConfig::getPathConfig(
    const PublisherId& id,
    const std::vector<std::string>& path) const {
  const auto logPrefix = fmt::format("[P:{}]", id);
  const auto& publishers = thrift_.publishers();
  auto it = publishers->find(id);
  if (it == publishers->end()) {
    throw Utils::createFsdbException(
        FsdbErrorCode::UNKNOWN_PUBLISHER, logPrefix, " unknown publisher");
  }

  for (const auto& pathConfig : *it->second.paths()) {
    if (Utils::isPrefixOf(*pathConfig.path()->raw(), path)) {
      return pathConfig;
    }
  }

  throw Utils::createFsdbException(
      FsdbErrorCode::PUBLISHER_NOT_PERMITTED,
      logPrefix,
      " publisher does not have permission for path: ",
      folly::join("/", path));
}

const std::optional<
    std::pair<SubscriberId, std::reference_wrapper<const SubscriberConfig>>>
FsdbConfig::getSubscriberConfig(const SubscriberId& id) const {
  const auto& subscribers = thrift_.subscribers();
  auto it = subscribers->find(id);
  if (it != subscribers->end()) {
    return std::
        pair<SubscriberId, std::reference_wrapper<const SubscriberConfig>>(
            it->first, it->second);
  }

  // fallback: id contains :<ConfiguredSubscriberIdSubstring>
  for (const auto& [key, value] : *subscribers) {
    if ((key.rfind(":") != std::string::npos) &&
        (id.find(key) != std::string::npos)) {
      return std::
          pair<SubscriberId, std::reference_wrapper<const SubscriberConfig>>(
              key, value);
    }
  }

  return std::nullopt;
}

} // namespace facebook::fboss::fsdb
