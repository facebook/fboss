// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/if/FsdbModel.h>
#include <fboss/thrift_visitors/NameToPathVisitor.h>

#include <string>
#include <vector>

namespace facebook::fboss::fsdb {

template <typename RootT>
class PathConverter {
 public:
  static std::vector<std::string> pathToIdTokens(
      const std::vector<std::string>& tokens) {
    std::vector<std::string> converted;
    visitThriftPath(tokens, [&](auto&& path) { converted = path.idTokens(); });
    return converted;
  }

  static std::vector<std::string> pathToNameTokens(
      const std::vector<std::string>& tokens) {
    std::vector<std::string> converted;
    visitThriftPath(tokens, [&](auto&& path) { converted = path.tokens(); });
    return converted;
  }

  static std::vector<OperPathElem> extPathToIdTokens(
      const std::vector<OperPathElem>& path) {
    std::vector<std::string> converted;
    extVisitThriftPath(path, [&](auto&& path) { converted = path.idTokens(); });
    return mergeOverWildcards(path, converted);
  }

  static std::vector<OperPathElem> extPathToNameTokens(
      const std::vector<OperPathElem>& path) {
    std::vector<std::string> converted;
    extVisitThriftPath(path, [&](auto&& path) { converted = path.tokens(); });
    return mergeOverWildcards(path, converted);
  }

 private:
  template <typename Func>
  static void visitThriftPath(
      const std::vector<std::string>& tokens,
      Func&& f) {
    thriftpath::RootThriftPath<RootT> root;
    auto result =
        RootNameToPathVisitor<thriftpath::RootThriftPath<RootT>>::visit(
            root,
            tokens.begin(),
            tokens.begin(),
            tokens.end(),
            std::forward<Func>(f));
    if (result != NameToPathResult::OK) {
      throw std::runtime_error("Invalid path " + folly::join("/", tokens));
    }
  }

  template <typename Func>
  static void extVisitThriftPath(
      const std::vector<OperPathElem>& path,
      Func&& f) {
    thriftpath::RootThriftPath<RootT> root;
    auto result =
        RootNameToPathVisitor<thriftpath::RootThriftPath<RootT>>::visitExtended(
            root, path.begin(), path.end(), std::forward<Func>(f));
    if (result != NameToPathResult::OK) {
      // TODO: include path in error message
      throw std::runtime_error("Invalid extended path");
    }
  }

  // wildcards elems get wiped during conversion using NameToPathVisitor. To get
  // them back, copy over the wildcard elems from the ext path. This works
  // strictly because path elems are only coverted between name/id for
  // struct/variant nodes and wildcards are only valid for other containers
  static std::vector<OperPathElem> mergeOverWildcards(
      const std::vector<OperPathElem>& orig,
      const std::vector<std::string>& tokens) {
    DCHECK_EQ(orig.size(), tokens.size());
    std::vector<OperPathElem> path;
    path.reserve(orig.size());
    for (auto i = 0; i < orig.size(); ++i) {
      if (!orig[i].raw_ref()) {
        // orig was not a raw elem, so just copy over it
        path.push_back(orig[i]);
      } else {
        // use the converted raw path
        OperPathElem el;
        el.set_raw(tokens[i]);
        path.push_back(std::move(el));
      }
    }
    return path;
  }
};

} // namespace facebook::fboss::fsdb
