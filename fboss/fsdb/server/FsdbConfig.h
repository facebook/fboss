// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <functional>
#include "fboss/fsdb/if/gen-cpp2/fsdb_config_types.h"

namespace facebook::fboss::fsdb {

class FsdbConfig {
 public:
  FsdbConfig() = default;
  explicit FsdbConfig(Config thrift) : thrift_(std::move(thrift)) {}

  const Config& getThrift() const {
    return thrift_;
  }

  // creators
  static std::shared_ptr<FsdbConfig> fromDefaultFile();
  static std::shared_ptr<FsdbConfig> fromFile(folly::StringPiece path);
  static std::shared_ptr<FsdbConfig> fromRaw(const std::string& raw);

  // serialize
  std::string configRaw() const;

  // helpers
  const std::reference_wrapper<const PathConfig> getPathConfig(
      const PublisherId& id,
      const std::vector<std::string>& path) const;

  const std::optional<
      std::pair<SubscriberId, std::reference_wrapper<const SubscriberConfig>>>
  getSubscriberConfig(const SubscriberId& id) const;

 private:
  const Config thrift_;
};

} // namespace facebook::fboss::fsdb
