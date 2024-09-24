// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_cow/gen-cpp2/patch_types.h>
#include <folly/Expected.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

enum class StorageError {
  // path is not valid for target storage type
  INVALID_PATH,
  // typed accessor was used, but the path contains a type that is
  // incompatible w/ that type. These should hopefully be caught at compile
  // time in most cases.
  TYPE_ERROR,
};

template <typename Root, typename Derived>
class Storage {
 public:
  // TODO: more flexibility here for all forward iterator types
  using ConcretePath = std::vector<std::string>;
  using PathIter = std::vector<std::string>::const_iterator;
  using ExtPath = std::vector<OperPathElem>;
  using ExtPathIter = ExtPath::const_iterator;
  using RootT = Root;

  template <typename T>
  using Result = folly::Expected<T, StorageError>;

  bool isPublished() {
    return static_cast<Derived*>(this)->isPublished_impl();
  }

  void publish() {
    static_cast<Derived*>(this)->publish_impl();
  }

  template <
      typename Path,
      typename =
          std::enable_if_t<std::is_same_v<typename Path::RootT, Root>, void>>
  Result<typename Path::DataT> get(const Path& path) const {
    return this->template get<typename Path::DataT>(path.begin(), path.end());
  }
  template <typename T>
  Result<T> get(const ConcretePath& path) const {
    return this->template get<T>(path.begin(), path.end());
  }
  template <typename T>
  Result<T> get(PathIter begin, PathIter end) const {
    return static_cast<Derived const*>(this)->template get_impl<T>(begin, end);
  }

  template <
      typename Path,
      typename =
          std::enable_if_t<std::is_same_v<typename Path::RootT, Root>, void>>
  Result<OperState> get_encoded(const Path& path, OperProtocol protocol) const {
    return this->get_encoded(path.begin(), path.end(), protocol);
  }
  Result<OperState> get_encoded(const ConcretePath& path, OperProtocol protocol)
      const {
    return this->get_encoded(path.begin(), path.end(), protocol);
  }
  Result<OperState>
  get_encoded(PathIter begin, PathIter end, OperProtocol protocol) const {
    return static_cast<Derived const*>(this)->get_encoded_impl(
        begin, end, protocol);
  }

  Result<std::vector<TaggedOperState>> get_encoded_extended(
      ExtPathIter begin,
      ExtPathIter end,
      OperProtocol protocol) const {
    return static_cast<Derived const*>(this)->get_encoded_extended_impl(
        begin, end, protocol);
  }

  template <
      typename Path,
      typename =
          std::enable_if_t<std::is_same_v<typename Path::RootT, Root>, void>>
  std::optional<StorageError> set(
      const Path& path,
      typename Path::DataT value) {
    return this->set(path.begin(), path.end(), std::move(value));
  }

  template <typename T>
  std::optional<StorageError> set(const ConcretePath& path, T&& value) {
    return this->template set(path.begin(), path.end(), std::forward<T>(value));
  }
  template <typename T>
  std::optional<StorageError> set(PathIter begin, PathIter end, T&& value) {
    return static_cast<Derived*>(this)->set_impl(
        begin, end, std::forward<T>(value));
  }

  template <
      typename Path,
      typename =
          std::enable_if_t<std::is_same_v<typename Path::RootT, Root>, void>>
  std::optional<StorageError> set_encoded(
      const Path& path,
      const OperState& state) {
    return this->set_encoded(path.begin(), path.end(), state);
  }
  std::optional<StorageError> set_encoded(
      const ConcretePath& path,
      const OperState& state) {
    return this->set_encoded(path.begin(), path.end(), state);
  }
  std::optional<StorageError>
  set_encoded(PathIter begin, PathIter end, const OperState& state) {
    return static_cast<Derived*>(this)->set_encoded_impl(begin, end, state);
  }

  std::optional<StorageError> patch(Patch&& patch) {
    return static_cast<Derived*>(this)->patch_impl(std::move(patch));
  }

  std::optional<StorageError> patch(const fsdb::OperDelta& delta) {
    return static_cast<Derived*>(this)->patch_impl(delta);
  }

  std::optional<StorageError> patch(const fsdb::TaggedOperState& state) {
    return static_cast<Derived*>(this)->patch_impl(state);
  }

  template <
      typename Path,
      typename =
          std::enable_if_t<std::is_same_v<typename Path::RootT, Root>, void>>
  std::optional<StorageError> add_encoded(
      const Path& path,
      const OperState& state) {
    return this->add_encoded(path.begin(), path.end(), state);
  }
  std::optional<StorageError> add_encoded(
      const ConcretePath& path,
      const OperState& state) {
    return this->add_encoded(path.begin(), path.end(), state);
  }
  std::optional<StorageError>
  add_encoded(PathIter begin, PathIter end, const OperState& state) {
    return static_cast<Derived*>(this)->add_encoded_impl(begin, end, state);
  }

  template <
      typename Path,
      typename =
          std::enable_if_t<std::is_same_v<typename Path::RootT, Root>, void>>
  std::optional<StorageError> remove(const Path& path) {
    return this->remove(path.begin(), path.end());
  }

  std::optional<StorageError> remove(const ConcretePath& path) {
    return this->template remove(path.begin(), path.end());
  }
  std::optional<StorageError> remove(PathIter begin, PathIter end) {
    return static_cast<Derived*>(this)->remove_impl(begin, end);
  }

  const Derived* derived() const {
    return static_cast<const Derived*>(this);
  }
};

template <typename Root, typename Derived>
bool operator==(
    const Storage<Root, Derived>& first,
    const Storage<Root, Derived>& second) {
  return *first.derived() == *second.derived();
}

template <typename Root, typename Derived>
bool operator!=(
    const Storage<Root, Derived>& first,
    const Storage<Root, Derived>& second) {
  return *first.derived() != *second.derived();
}

} // namespace facebook::fboss::fsdb
