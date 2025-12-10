// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/common/PathHelpers.h"
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/String.h>
#include <folly/Unit.h>
#include <re2/re2.h>
#include <string>

namespace thriftpath {

#define STRUCT_CHILD_GETTERS(child, childId)                        \
  TypeFor<strings::child> child() const& {                          \
    const std::string childIdStr = folly::to<std::string>(childId); \
    facebook::fboss::fsdb::OperPathElem elem;                       \
    elem.set_raw(childIdStr);                                       \
    return TypeFor<strings::child>(                                 \
        copyAndExtendVec(this->tokens_, #child),                    \
        copyAndExtendVec(this->idTokens_, childIdStr),              \
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),   \
        this->hasWildcards_);                                       \
  }                                                                 \
  TypeFor<strings::child> child() && {                              \
    this->tokens_.push_back(#child);                                \
    const std::string childIdStr = folly::to<std::string>(childId); \
    this->idTokens_.push_back(childIdStr);                          \
    facebook::fboss::fsdb::OperPathElem elem;                       \
    elem.set_raw(childIdStr);                                       \
    this->extendedTokens_.push_back(std::move(elem));               \
    return TypeFor<strings::child>(                                 \
        std::move(this->tokens_),                                   \
        std::move(this->idTokens_),                                 \
        std::move(this->extendedTokens_),                           \
        this->hasWildcards_);                                       \
  }
#define CONTAINER_CHILD_GETTERS(key_type)                             \
  Child operator[](key_type token) const& {                           \
    const std::string strToken = folly::to<std::string>(token);       \
    facebook::fboss::fsdb::OperPathElem elem;                         \
    elem.set_raw(strToken);                                           \
    return Child(                                                     \
        copyAndExtendVec(this->tokens_, strToken),                    \
        copyAndExtendVec(this->idTokens_, strToken),                  \
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),     \
        this->hasWildcards_);                                         \
  }                                                                   \
  Child operator[](key_type token) && {                               \
    const std::string strToken = folly::to<std::string>(token);       \
    this->tokens_.push_back(strToken);                                \
    this->idTokens_.push_back(strToken);                              \
    facebook::fboss::fsdb::OperPathElem elem;                         \
    elem.set_raw(strToken);                                           \
    this->extendedTokens_.push_back(std::move(elem));                 \
    return Child(                                                     \
        std::move(this->tokens_),                                     \
        std::move(this->idTokens_),                                   \
        std::move(this->extendedTokens_),                             \
        this->hasWildcards_);                                         \
  }                                                                   \
  Child operator[](facebook::fboss::fsdb::OperPathElem elem) const& { \
    return Child(                                                     \
        copyAndExtendVec(this->tokens_, pathElemToString(elem)),      \
        copyAndExtendVec(this->idTokens_, pathElemToString(elem)),    \
        copyAndExtendVec(this->extendedTokens_, std::move(elem)),     \
        true /* hasWildcards */);                                     \
  }                                                                   \
  Child operator[](facebook::fboss::fsdb::OperPathElem elem) && {     \
    this->tokens_.push_back(pathElemToString(elem));                  \
    this->idTokens_.push_back(pathElemToString(elem));                \
    this->extendedTokens_.push_back(std::move(elem));                 \
    return Child(                                                     \
        std::move(this->tokens_),                                     \
        std::move(this->idTokens_),                                   \
        std::move(this->extendedTokens_),                             \
        true /* hasWildcards */);                                     \
  }

class BasePath {
 public:
  BasePath(
      std::vector<std::string> tokens,
      std::vector<std::string> idTokens,
      std::vector<facebook::fboss::fsdb::OperPathElem> extendedTokens,
      bool hasWildcards)
      : tokens_(std::move(tokens)),
        idTokens_(std::move(idTokens)),
        extendedTokens_(std::move(extendedTokens)),
        hasWildcards_(hasWildcards) {}

  auto begin() const {
    return tokens_.cbegin();
  }

  auto end() const {
    return tokens_.cend();
  }

  const std::vector<std::string>& tokens() const {
    if (hasWildcards_) {
      throw std::runtime_error("Cannot get raw tokens if path has wildcards");
    }
    return tokens_;
  }

  const std::vector<std::string>& idTokens() const {
    if (hasWildcards_) {
      throw std::runtime_error("Cannot get raw tokens if path has wildcards");
    }
    return idTokens_;
  }

  const std::vector<facebook::fboss::fsdb::OperPathElem>& extendedTokens()
      const {
    return extendedTokens_;
  }

  facebook::fboss::fsdb::ExtendedOperPath extendedPath() const {
    facebook::fboss::fsdb::ExtendedOperPath path;
    path.path() = extendedTokens_;
    return path;
  }

  bool matchesPath(const std::vector<std::string>& other) const {
    if (other.size() != extendedTokens_.size()) {
      return false;
    }
    using OperPathElem = facebook::fboss::fsdb::OperPathElem;
    for (int i = 0; i < other.size(); i++) {
      const auto& elem = extendedTokens_.at(i);
      const auto& token = other.at(i);
      if (elem.getType() == OperPathElem::Type::raw) {
        if (token != idTokens_.at(i) && token != tokens_.at(i)) {
          // raw token didn't match either id or name token
          return false;
        }
      } else if (elem.getType() == OperPathElem::Type::regex) {
        if (!re2::RE2::FullMatch(token, *elem.regex())) {
          return false;
        }
      } else if (elem.getType() == OperPathElem::Type::any) {
        // always match
      }
    }
    // no violations
    return true;
  }

  std::string str() const {
    // TODO: better format
    return "/" + folly::join('/', tokens_.begin(), tokens_.end());
  }

 protected:
  std::vector<std::string> tokens_;
  std::vector<std::string> idTokens_;
  // ids by default
  std::vector<facebook::fboss::fsdb::OperPathElem> extendedTokens_;
  bool hasWildcards_;
};

template <
    typename _DataT,
    typename _RootT,
    typename _TC,
    typename _Tag,
    typename _ParentT>
class Path : public BasePath {
 public:
  using DataT = _DataT;
  using RootT = _RootT;
  using TC = _TC;
  using Tag = _Tag;
  using ParentT = _ParentT;

  using BasePath::BasePath;
};

template <typename T>
class RootThriftPath {
 public:
  // While this is always-false, it is dependent and therefore fires only
  // at instantiation time.
  static_assert(
      !std::is_same<T, T>::value,
      "You need to include the header file that the thriftpath plugin "
      "generated for T in order to use RootThriftPath<T>. Also ensure that "
      "you have annotated your root struct with (thriftpath.root)");
};

template <typename T, typename Root, typename Parent>
class ChildThriftPath {
 public:
  // While this is always-false, it is dependent and therefore fires only
  // at instantiation time.
  static_assert(
      !std::is_same<T, T>::value,
      "You need to include the header file that the thriftpath plugin "
      "generated for T in order to use ChildThriftPath<T>.");
};

std::vector<std::string> copyAndExtendVec(
    const std::vector<std::string>& parents,
    std::string last);
std::vector<facebook::fboss::fsdb::OperPathElem> copyAndExtendVec(
    const std::vector<facebook::fboss::fsdb::OperPathElem>& parents,
    facebook::fboss::fsdb::OperPathElem last);

std::string pathElemToString(const facebook::fboss::fsdb::OperPathElem& elem);

} // namespace thriftpath
