// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/if/FsdbModel.h>
#include <fboss/thrift_visitors/NameToPathVisitor.h>

#include <string>
#include <vector>

namespace facebook::fboss::fsdb {

namespace pc_detail {
template <typename PathElem>
struct Paths {
  std::vector<PathElem> tokens;
  std::vector<PathElem> idTokens;
};

// wildcards elems get wiped during conversion using NameToPathVisitor. To get
// them back, copy over the wildcard elems from the ext path. This works
// strictly because path elems are only coverted between name/id for
// struct/variant nodes and wildcards are only valid for other containers
std::vector<OperPathElem> mergeOverWildcards(
    const std::vector<OperPathElem>& orig,
    const std::vector<std::string>& tokens);
} // namespace pc_detail

template <typename RootT>
class PathConverter {
 public:
  static std::vector<std::string> pathToIdTokens(
      const std::vector<std::string>& tokens);

  static std::vector<std::string> pathToNameTokens(
      const std::vector<std::string>& tokens);

  static std::vector<OperPathElem> extPathToIdTokens(
      const std::vector<OperPathElem>& path);

  static std::vector<OperPathElem> extPathToNameTokens(
      const std::vector<OperPathElem>& path);

 private:
  static pc_detail::Paths<std::string> getPaths(
      const std::vector<std::string>& path);

  static pc_detail::Paths<OperPathElem> getExtendedPaths(
      const std::vector<OperPathElem>& path);
};

// To avoid compiler inlining these heavy functions and allow for caching
// template instantiations, these need to be implemented outside the class body
template <typename RootT>
std::vector<std::string> PathConverter<RootT>::pathToIdTokens(
    const std::vector<std::string>& tokens) {
  return getPaths(tokens).idTokens;
}

template <typename RootT>
std::vector<std::string> PathConverter<RootT>::pathToNameTokens(
    const std::vector<std::string>& tokens) {
  return getPaths(tokens).tokens;
}

template <typename RootT>
std::vector<OperPathElem> PathConverter<RootT>::extPathToIdTokens(
    const std::vector<OperPathElem>& path) {
  return getExtendedPaths(path).idTokens;
}

template <typename RootT>
std::vector<OperPathElem> PathConverter<RootT>::extPathToNameTokens(
    const std::vector<OperPathElem>& path) {
  return getExtendedPaths(path).tokens;
}

template <typename RootT>
pc_detail::Paths<std::string> PathConverter<RootT>::getPaths(
    const std::vector<std::string>& path) {
  thriftpath::RootThriftPath<RootT> root;
  pc_detail::Paths<std::string> paths;
  auto result = RootNameToPathVisitor<thriftpath::RootThriftPath<RootT>>::visit(
      root,
      path.begin(),
      path.begin(),
      path.end(),
      [&](const ::thriftpath::BasePath& path) {
        paths.tokens = path.tokens();
        paths.idTokens = path.idTokens();
      });
  if (result != NameToPathResult::OK) {
    // TODO: include path in error message
    throw std::runtime_error("Invalid extended path");
  }
  return paths;
}

template <typename RootT>
pc_detail::Paths<OperPathElem> PathConverter<RootT>::getExtendedPaths(
    const std::vector<OperPathElem>& path) {
  thriftpath::RootThriftPath<RootT> root;
  pc_detail::Paths<std::string> rawPaths;
  auto result =
      RootNameToPathVisitor<thriftpath::RootThriftPath<RootT>>::visitExtended(
          root,
          path.begin(),
          path.end(),
          [&](const ::thriftpath::BasePath& path) {
            rawPaths.tokens = path.tokens();
            rawPaths.idTokens = path.idTokens();
          });
  if (result != NameToPathResult::OK) {
    // TODO: include path in error message
    throw std::runtime_error("Invalid extended path");
  }
  pc_detail::Paths<OperPathElem> paths;
  paths.tokens = pc_detail::mergeOverWildcards(path, rawPaths.tokens);
  paths.idTokens = pc_detail::mergeOverWildcards(path, rawPaths.idTokens);
  return paths;
}

} // namespace facebook::fboss::fsdb
