/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/PathValidator.h"
#include <folly/json/DynamicConverter.h>
#include <folly/json/json.h>
#include <re2/re2.h>
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/thrift_visitors/NameToPathVisitor.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

namespace {

template <typename ThriftTreeT>
bool isPathValid(const std::vector<std::string>& path) {
  thriftpath::RootThriftPath<ThriftTreeT> root;
  auto result =
      RootNameToPathVisitor<thriftpath::RootThriftPath<ThriftTreeT>>::visit(
          root, path.begin(), path.begin(), path.end(), [](auto&&) {});
  return result == NameToPathResult::OK;
}

template <typename ThriftTreeT>
void validateRawPath(const std::vector<std::string>& path) {
  thriftpath::RootThriftPath<ThriftTreeT> root;
  auto result =
      RootNameToPathVisitor<thriftpath::RootThriftPath<ThriftTreeT>>::visit(
          root, path.begin(), path.begin(), path.end(), [](auto&&) {});
  if (result != NameToPathResult::OK) {
    auto errorName = apache::thrift::util::enumNameSafe(result);
    auto pathStr = folly::toJson(folly::toDynamic(path));
    throw Utils::createFsdbException(
        FsdbErrorCode::INVALID_PATH,
        "InvalidPath: ",
        errorName,
        ". Received Path: ",
        pathStr);
  }
}

void validateRegexesInExtendedPath(const std::vector<OperPathElem>& elems) {
  // First check all regexes are parseable
  for (const auto& elem : elems) {
    if (auto regexStr = elem.regex_ref()) {
      const auto& regex = re2::RE2(*regexStr);
      if (!regex.ok()) {
        throw Utils::createFsdbException(
            FsdbErrorCode::INVALID_PATH,
            "InvalidPath: Unparseable regex",
            ". Error: ",
            regex.error());
      }
    }
  }
}

template <typename ThriftTreeT>
void validateExtendedPath(const ExtendedOperPath& path) {
  thriftpath::RootThriftPath<ThriftTreeT> root;
  const auto& elems = *path.path();
  validateRegexesInExtendedPath(elems);

  auto result = RootNameToPathVisitor<thriftpath::RootThriftPath<ThriftTreeT>>::
      visitExtended(root, elems.begin(), elems.end(), [](auto&&) {});
  if (result != NameToPathResult::OK) {
    auto errorName = apache::thrift::util::enumNameSafe(result);
    throw Utils::createFsdbException(
        FsdbErrorCode::INVALID_PATH, "InvalidPath: ", errorName);
    // TODO: include path in error string
  }
}

} // namespace

void PathValidator::validateStatePath(const std::vector<std::string>& path) {
  validateRawPath<FsdbOperStateRoot>(path);
}

void PathValidator::validateStatsPath(const std::vector<std::string>& path) {
  validateRawPath<FsdbOperStatsRoot>(path);
}

void PathValidator::validateExtendedStatePath(const ExtendedOperPath& path) {
  validateExtendedPath<FsdbOperStateRoot>(path);
}

void PathValidator::validateExtendedStatsPath(const ExtendedOperPath& path) {
  validateExtendedPath<FsdbOperStatsRoot>(path);
}

bool PathValidator::isStatePathValid(const std::vector<std::string>& path) {
  return isPathValid<FsdbOperStateRoot>(path);
}

bool PathValidator::isStatsPathValid(const std::vector<std::string>& path) {
  return isPathValid<FsdbOperStatsRoot>(path);
}

void PathValidator::validateExtendedStatePaths(
    const std::vector<ExtendedOperPath>& paths) {
  for (const auto& path : paths) {
    PathValidator::validateExtendedStatePath(path);
  }
}
void PathValidator::validateExtendedStatsPaths(
    const std::vector<ExtendedOperPath>& paths) {
  for (const auto& path : paths) {
    PathValidator::validateExtendedStatsPath(path);
  }
}

} // namespace facebook::fboss::fsdb
