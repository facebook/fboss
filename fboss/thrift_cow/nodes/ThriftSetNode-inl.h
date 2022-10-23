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

#include <fatal/container/tuple.h>
#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>
#include <thrift/lib/cpp2/reflection/folly_dynamic.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"

namespace facebook::fboss::thrift_cow {

namespace set_helpers {

template <typename TC>
struct ExtractTypeClass {
  using type = TC;
};

template <typename ValueTypeClass>
struct ExtractTypeClass<apache::thrift::type_class::set<ValueTypeClass>> {
  using type = ValueTypeClass;
};

} // namespace set_helpers

template <typename TypeClass, typename TType>
struct ThriftSetFields {
  using Self = ThriftSetFields<TypeClass, TType>;
  using CowType = FieldsType;
  using ThriftType = TType;
  using ValueTypeClass =
      typename set_helpers::ExtractTypeClass<TypeClass>::type;
  using ValueTType = typename TType::value_type;

  using ValueTraits = ConvertToImmutableNodeTraits<ValueTypeClass, ValueTType>;
  using value_type = typename ValueTraits::type;
  using StorageType = std::unordered_set<value_type>;
  using iterator = typename StorageType::iterator;
  using const_iterator = typename StorageType::const_iterator;

  // constructors:
  // One takes a thrift type directly, one starts with empty vector

  ThriftSetFields() {}

  template <
      typename T,
      typename = std::enable_if_t<std::is_same<std::decay_t<T>, TType>::value>>
  explicit ThriftSetFields(T&& thrift) {
    fromThrift(std::forward<T>(thrift));
  }

  TType toThrift() const {
    TType thrift;

    for (auto&& elem : storage_) {
      thrift.emplace(elem->toThrift());
    }
    return thrift;
  }

  template <
      typename T,
      typename = std::enable_if_t<std::is_same<std::decay_t<T>, TType>::value>>
  void fromThrift(T&& thrift) {
    storage_.clear();
    for (const auto& elem : thrift) {
      emplace(elem);
    }
  }

#ifdef ENABLE_DYNAMIC_APIS

  folly::dynamic toDynamic() const {
    folly::dynamic out;
    apache::thrift::to_dynamic<TypeClass>(
        out, toThrift(), apache::thrift::dynamic_format::JSON_1);
    return out;
  }

  void fromDynamic(const folly::dynamic& value) {
    TType thrift;
    apache::thrift::from_dynamic<TypeClass>(
        thrift, value, apache::thrift::dynamic_format::JSON_1);
    fromThrift(thrift);
  }

#endif

  folly::fbstring encode(fsdb::OperProtocol proto) const {
    return serialize<TypeClass>(proto, toThrift());
  }

  void fromEncoded(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
    fromThrift(deserialize<TypeClass, TType>(proto, encoded));
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return storage_.emplace(childFactory(std::forward<Args>(args)...));
  }

  iterator erase(iterator pos) {
    return storage_.erase(pos);
  }

  iterator erase(const_iterator pos) {
    return storage_.erase(pos);
  }

  std::size_t erase(const value_type& val) {
    return storage_.erase(val);
  }

  std::size_t erase(const ValueTType& val) {
    return storage_.erase(value_type{val});
  }

  bool remove(const std::string& token) {
    if constexpr (std::is_same_v<
                      ValueTypeClass,
                      apache::thrift::type_class::enumeration>) {
      // special handling for enum keyed maps
      ValueTType enumVal;
      if (fatal::enum_traits<ValueTType>::try_parse(enumVal, token)) {
        return remove(enumVal);
      }
    } else if constexpr (std::is_same_v<
                             ValueTypeClass,
                             apache::thrift::type_class::string>) {
      return erase(token);
    }

    auto value = folly::tryTo<ValueTType>(token);
    if (value.hasValue()) {
      return remove(value.value());
    }

    return false;
  }

  template <typename T = Self>
  auto remove(const ValueTType& value) -> std::enable_if_t<
      !std::is_same_v<
          typename T::ValueTypeClass,
          apache::thrift::type_class::string>,
      bool> {
    return erase(value);
  }

  // iterators

  iterator begin() {
    return storage_.begin();
  }

  const_iterator begin() const {
    return storage_.cbegin();
  }

  iterator end() {
    return storage_.end();
  }

  const_iterator end() const {
    return storage_.end();
  }

  iterator find(const value_type& value) {
    return storage_.find(value);
  }

  const_iterator find(const value_type& value) const {
    return storage_.find(value);
  }

  iterator find(const ValueTType& value) {
    return storage_.find(value_type{value});
  }

  const_iterator find(const ValueTType& value) const {
    return storage_.find(value_type{value});
  }

  const_iterator cbegin() const {
    return storage_.cbegin();
  }

  const_iterator cend() const {
    return storage_.cend();
  }

  std::size_t size() const {
    return storage_.size();
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {
    // sets can't have complex children types
  }

 private:
  template <typename... Args>
  value_type childFactory(Args&&... args) {
    return std::make_optional<typename value_type::value_type>(
        std::forward<Args>(args)...);
  }

  StorageType storage_;
};

template <typename TypeClass, typename TType>
class ThriftSetNode : public NodeBaseT<
                          ThriftSetNode<TypeClass, TType>,
                          ThriftSetFields<TypeClass, TType>> {
 public:
  using Self = ThriftSetNode<TypeClass, TType>;
  using Fields = ThriftSetFields<TypeClass, TType>;
  using ThriftType = typename Fields::ThriftType;
  using BaseT = NodeBaseT<ThriftSetNode<TypeClass, TType>, Fields>;
  using CowType = NodeType;
  using value_type = typename Fields::value_type;
  using ValueTType = typename Fields::ValueTType;
  using PathIter = typename std::vector<std::string>::const_iterator;
  using NodeConstIter = typename Fields::const_iterator;
  using NodeIter = typename Fields::iterator;

  using BaseT::BaseT;

  TType toThrift() const {
    return this->getFields()->toThrift();
  }

  void fromThrift(const TType& thrift) {
    return this->writableFields()->fromThrift(thrift);
  }

#ifdef ENABLE_DYNAMIC_APIS
  folly::dynamic toFollyDynamic() const override {
    return this->getFields()->toDynamic();
  }

  void fromFollyDynamic(const folly::dynamic& value) {
    return this->writableFields()->fromDynamic(value);
  }
#else
  folly::dynamic toFollyDynamic() const override {
    return {};
  }
#endif

  folly::fbstring encode(fsdb::OperProtocol proto) const {
    return this->getFields()->encode(proto);
  }

  void fromEncoded(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
    return this->writableFields()->fromEncoded(proto, encoded);
  }

  template <typename... Args>
  typename std::pair<typename Fields::iterator, bool> emplace(Args&&... args) {
    return this->writableFields()->emplace(std::forward<Args>(args)...);
  }

  typename Fields::iterator erase(typename Fields::iterator pos) {
    return this->writableFields()->erase(pos);
  }

  typename Fields::iterator erase(typename Fields::const_iterator pos) {
    return this->writableFields()->erase(pos);
  }

  std::size_t erase(const value_type& val) {
    return this->writableFields()->erase(val);
  }

  std::size_t erase(const ValueTType& val) {
    return this->writableFields()->erase(val);
  }

  // iterators

  typename Fields::iterator begin() {
    return this->writableFields()->begin();
  }

  typename Fields::const_iterator begin() const {
    return this->getFields()->begin();
  }

  typename Fields::iterator end() {
    return this->writableFields()->end();
  }
  typename Fields::const_iterator end() const {
    return this->getFields()->end();
  }

  typename Fields::const_iterator cbegin() const {
    return this->getFields()->cbegin();
  }

  typename Fields::const_iterator cend() const {
    return this->getFields()->cend();
  }

  typename Fields::iterator find(const value_type& value) {
    return this->writableFields->find(value);
  }

  typename Fields::const_iterator find(const value_type& value) const {
    return this->getFields()->find(value);
  }

  typename Fields::iterator find(const ValueTType& value) {
    return this->writableFields()->find(value_type{value});
  }

  typename Fields::const_iterator find(const ValueTType& value) const {
    return this->getFields()->find(value_type{value});
  }

  std::size_t size() const {
    return this->getFields()->size();
  }

  bool remove(const std::string& token) {
    return this->writableFields()->remove(token);
  }

  template <typename T = Fields>
  auto remove(const ValueTType& value) -> std::enable_if_t<
      !std::is_same_v<
          typename T::ValueTypeClass,
          apache::thrift::type_class::string>,
      bool> {
    // TODO: better handling of set<string> tc so we don't need
    // special impl remove fn
    return this->writableFields()->remove(value);
  }

  void modify(const std::string& token) {
    if constexpr (std::is_same_v<
                      typename Fields::ValueTypeClass,
                      apache::thrift::type_class::enumeration>) {
      // special handling for enum keyed maps
      ValueTType enumVal;
      if (fatal::enum_traits<ValueTType>::try_parse(enumVal, token)) {
        modifyImpl(enumVal);
        return;
      }
    }

    auto value = folly::tryTo<ValueTType>(token);
    if (value.hasValue()) {
      modifyImpl(value.value());
      return;
    }

    throw std::runtime_error(folly::to<std::string>("Invalid key: ", token));
  }

  void modifyImpl(const ValueTType& value) {
    DCHECK(!this->isPublished());
    if (auto it = this->find(value); it == this->end()) {
      this->emplace(value);
    }
  }

  static void modify(std::shared_ptr<Self>* node, const std::string& token) {
    auto newNode = ((*node)->isPublished()) ? (*node)->clone() : *node;
    newNode->modify(token);
    node->swap(newNode);
  }

  /*
   * Visitors by string path
   */

  template <typename Func>
  inline ThriftTraverseResult
  visitPath(PathIter begin, PathIter end, Func&& f) {
    return PathVisitor<TypeClass>::visit(
        *this, begin, end, PathVisitMode::LEAF, std::forward<Func>(f));
  }

  template <typename Func>
  inline ThriftTraverseResult visitPath(PathIter begin, PathIter end, Func&& f)
      const {
    return PathVisitor<TypeClass>::visit(
        *this, begin, end, PathVisitMode::LEAF, std::forward<Func>(f));
  }

  template <typename Func>
  inline ThriftTraverseResult cvisitPath(PathIter begin, PathIter end, Func&& f)
      const {
    return PathVisitor<TypeClass>::visit(
        *this, begin, end, PathVisitMode::LEAF, std::forward<Func>(f));
  }

 private:
  friend class CloneAllocator;
};

} // namespace facebook::fboss::thrift_cow
