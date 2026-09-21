/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <folly/json/json.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss::utils {

/*
 * Common serialization and filesystem primitives for config workflows.
 * Service-specific code remains responsible for constructing config contents;
 * this layer only defines their JSON representation and safe output behavior.
 */

// The SimpleJSON round trip supports Thrift maps with non-string key types,
// which cannot be passed directly to folly::toPrettyJson().
template <typename ThriftConfig>
std::string serializeToPrettyJson(const ThriftConfig& config) {
  auto serialized =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(config);
  return folly::toPrettyJson(folly::parseJson(serialized));
}

// Creates or validates the requested directory. Without an explicit path, a
// private unique directory is created under /tmp/fboss2/config-gen. Active
// service configuration under /etc/coop is never accepted as an output.
std::filesystem::path prepareOutputDirectory(
    const std::optional<std::filesystem::path>& outputDirectory = std::nullopt);

// Uses create-only semantics so generation cannot replace an existing config
// or follow a final-component symlink. A partially written file is removed.
void writeFileWithoutOverwrite(
    const std::filesystem::path& path,
    std::string_view contents);

} // namespace facebook::fboss::utils
