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

#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/op/Get.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/thrift_cow/nodes/Types.h"

#include <variant>

namespace facebook::fboss::thrift_cow {

template <typename Fields>
struct ThriftUnionStorage;

template <typename Fields>
struct ThriftUnionStorage {
  using TType = typename Fields::ThriftType;
  using TypeEnum = typename Fields::TypeEnum;
  using StorageType = typename Fields::Storage;

  template <typename Id>
  using TypeFor = typename Fields::template TypeFor<Id>;
  template <typename Id>
  using ThriftTypeFor = apache::thrift::op::get_native_type<TType, Id>;

  template <typename Id>
  TypeFor<Id> get() const {
    constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
    if (folly::to_underlying(fid) == folly::to_underlying(type_)) {
      constexpr size_t pos =
          static_cast<size_t>(apache::thrift::op::get_ordinal_v<TType, Id>) - 1;
      return std::get<pos>(storage_);
    }
    throw std::bad_variant_access();
  }

  template <typename Id>
  TypeFor<Id>& ref() {
    constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
    if (folly::to_underlying(fid) == folly::to_underlying(type_)) {
      constexpr size_t pos =
          static_cast<size_t>(apache::thrift::op::get_ordinal_v<TType, Id>) - 1;
      return std::get<pos>(storage_);
    }
    throw std::bad_variant_access();
  }

  template <typename Id>
  const TypeFor<Id>& ref() const {
    constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
    if (folly::to_underlying(fid) == folly::to_underlying(type_)) {
      constexpr size_t pos =
          static_cast<size_t>(apache::thrift::op::get_ordinal_v<TType, Id>) - 1;
      return std::get<pos>(storage_);
    }
    throw std::bad_variant_access();
  }

  template <typename Id>
  const TypeFor<Id>& cref() const {
    constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
    if (folly::to_underlying(fid) == folly::to_underlying(type_)) {
      constexpr size_t pos =
          static_cast<size_t>(apache::thrift::op::get_ordinal_v<TType, Id>) - 1;
      return std::get<pos>(storage_);
    }
    throw std::bad_variant_access();
  }

  template <typename Id, typename... Args>
  void set(Args&&... args) {
    clear();
    constexpr size_t pos =
        static_cast<size_t>(apache::thrift::op::get_ordinal_v<TType, Id>) - 1;
    if constexpr (Fields::template IsChildNode<Id>) {
      using MemberType = typename Fields::template TypeFor<Id>::element_type;
      static_assert(
          std::is_constructible_v<MemberType, Args...>,
          "Unexpected args for union set()");
      std::get<pos>(storage_) =
          std::make_shared<MemberType>(std::forward<Args>(args)...);
    } else {
      using MemberType = typename Fields::template TypeFor<Id>::value_type;
      static_assert(
          std::is_constructible_v<MemberType, Args...>,
          "Unexpected args for union set()");
      std::get<pos>(storage_) =
          std::make_optional<MemberType>(std::forward<Args>(args)...);
    }

    constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
    type_ = static_cast<TypeEnum>(folly::to_underlying(fid));
  }

  void clear() {
    apache::thrift::op::for_each_field_id<TType>(
        [&]<class Id>(Id) { this->template resetMember<Id>(); });
  }

  template <typename Id>
  bool resetMember() {
    constexpr size_t pos =
        static_cast<size_t>(apache::thrift::op::get_ordinal_v<TType, Id>) - 1;
    auto& value = std::get<pos>(storage_);
    if (value) {
      value.reset();
      type_ = TypeEnum::__EMPTY__;
      return true;
    }
    return false;
  }

  TypeEnum type() const {
    return type_;
  }

 private:
  TypeEnum type_{TypeEnum::__EMPTY__};
  StorageType storage_;
};

template <typename TType>
struct ThriftUnionFields : public FieldBaseType {
  using Self = ThriftUnionFields<TType>;

  using CowType = FieldsType;
  using ThriftType = TType;
  using TC = apache::thrift::type_class::variant;
  using TypeEnum = typename TType::Type;

  // Storage is a std::tuple indexed by ordinal, wrapped in ThriftUnionStorage
  using Storage = CowStorage<TType, ThriftUnionFields<TType>, false>;

  template <typename Id>
  using FieldTraits =
      CowFieldTraits<TType, Id, ThriftUnionFields<TType>, false>;

  template <typename Id>
  using TypeFor = typename FieldTraits<Id>::type;

  template <typename Id>
  using ThriftTypeFor = apache::thrift::op::get_native_type<TType, Id>;

  template <typename Id>
  static constexpr bool IsChildNode = FieldTraits<Id>::isChild::value;

  // constructors:
  // One takes a thrift type directly, one default constructs everything

  ThriftUnionFields() {}

  template <typename T>
  explicit ThriftUnionFields(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    fromThrift(std::forward<T>(thrift));
  }

  TType toThrift() const {
    TType thrift;
    apache::thrift::op::for_each_field_id<TType>([&]<class Id>(Id) {
      using Traits = FieldTraits<Id>;
      constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
      if (folly::to_underlying(fid) == folly::to_underlying(storage_.type())) {
        auto& stored = storage_.template cref<Id>();
        if (stored) {
          if constexpr (Traits::isChild::value) {
            apache::thrift::op::get<Id>(thrift) = stored->toThrift();
          } else {
            apache::thrift::op::get<Id>(thrift) = stored->cref();
          }
        }
      }
    });
    return thrift;
  }

  template <typename T>
  void fromThrift(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    apache::thrift::op::for_each_field_id<TType>([&]<class Id>(Id) {
      constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
      if (folly::to_underlying(fid) == folly::to_underlying(thrift.getType())) {
        auto& val = *apache::thrift::op::get<Id>(thrift);
        storage_.template set<Id>(val);
      }
    });
  }

#ifdef ENABLE_DYNAMIC_APIS

  folly::dynamic toDynamic() const {
    folly::dynamic out;
    facebook::thrift::to_dynamic(
        out, toThrift(), facebook::thrift::dynamic_format::JSON_1);
    return out;
  }

  void fromDynamic(const folly::dynamic& value) {
    TType thrift;
    facebook::thrift::from_dynamic(
        thrift, value, facebook::thrift::dynamic_format::JSON_1);
    fromThrift(thrift);
  }
#endif

  folly::fbstring encode(fsdb::OperProtocol proto) const {
    return serialize<TC>(proto, toThrift());
  }

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const {
    return serializeBuf<TC>(proto, toThrift());
  }

  void fromEncoded(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
    fromThrift(deserialize<TC, TType>(proto, encoded));
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded) {
    fromThrift(deserializeBuf<TC, TType>(proto, std::move(encoded)));
  }

  TypeEnum type() const {
    return storage_.type();
  }

  template <typename Id>
  bool isSet() const {
    constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
    return folly::to_underlying(fid) == folly::to_underlying(type());
  }

  template <typename Id>
  TypeFor<Id> get() const {
    return storage_.template get<Id>();
  }

  /*
   * References to underlying field.
   *
   * TODO: create ref() wrapper class that respects if the node is
   * published.
   */
  template <typename Id>
  TypeFor<Id>& ref() {
    return storage_.template ref<Id>();
  }

  template <typename Id>
  const TypeFor<Id>& ref() const {
    return storage_.template ref<Id>();
  }

  template <typename Id>
  const TypeFor<Id>& cref() const {
    return storage_.template cref<Id>();
  }

  /*
   * Setters.
   */

  template <typename Id, typename... Args>
  void set(Args&&... args) {
    storage_.template set<Id>(std::forward<Args>(args)...);
  }

  template <typename Id>
  bool remove() {
    return storage_.template resetMember<Id>();
  }

  bool remove(const std::string& token) {
    bool found{false}, ret{false};
    visitMember<TType>(token, [&]<class Id>(Id) {
      found = true;
      ret = this->template remove<Id>();
    });

    if (!found) {
      throw std::runtime_error(
          folly::to<std::string>("Invalid union child name: ", token));
    }

    return ret;
  }

  template <typename Fn>
  void forEachChild(Fn fn) {
    apache::thrift::op::for_each_field_id<TType>([&]<class Id>(Id) {
      if constexpr (FieldTraits<Id>::isChild::value) {
        constexpr auto fid = apache::thrift::op::get_field_id_v<TType, Id>;
        if (folly::to_underlying(fid) !=
            folly::to_underlying(storage_.type())) {
          return;
        }
        auto child = storage_.template get<Id>();
        if (child) {
          fn(child.get());
        }
      }
    });
  }

 private:
  ThriftUnionStorage<Self> storage_;
};

template <typename TType>
class ThriftUnionNode
    : public NodeBaseT<ThriftUnionNode<TType>, ThriftUnionFields<TType>>,
      public thrift_cow::Serializable {
 public:
  using Self = ThriftUnionNode<TType>;
  using Fields = ThriftUnionFields<TType>;
  using TC = typename Fields::TC;
  using ThriftType = typename Fields::ThriftType;
  using BaseT = NodeBaseT<ThriftUnionNode<TType>, Fields>;
  using CowType = NodeType;
  using TypeEnum = typename Fields::TypeEnum;
  using PathIter = typename std::vector<std::string>::const_iterator;

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

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const override {
    return this->getFields()->encodeBuf(proto);
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded)
      override {
    return this->writableFields()->fromEncodedBuf(proto, std::move(encoded));
  }

  TypeEnum type() const {
    return this->getFields()->type();
  }

  template <typename Id>
  auto get() const {
    return this->getFields()->template get<Id>();
  }

  template <typename Id, typename... Args>
  void set(Args&&... args) {
    this->writableFields()->template set<Id>(std::forward<Args>(args)...);
  }

  template <typename Id>
  typename Fields::template TypeFor<Id>& ref() {
    return this->writableFields()->template ref<Id>();
  }

  template <typename Id>
  const typename Fields::template TypeFor<Id>& ref() const {
    return this->writableFields()->template ref<Id>();
  }

  template <typename Id>
  const typename Fields::template TypeFor<Id>& cref() const {
    return this->getFields()->template cref<Id>();
  }

  // prefer safe_ref/safe_cref for safe access
  template <typename Id>
  auto safe_ref() {
    return detail::ref(this->writableFields()->template ref<Id>());
  }

  template <typename Id>
  auto safe_cref() const {
    return detail::cref(this->getFields()->template cref<Id>());
  }

  template <typename Id>
  bool remove() {
    // TODO: what to do when removing a member that isn't set? Should
    // we error?
    return this->writableFields()->template remove<Id>();
  }

  virtual bool remove(const std::string& token) override {
    return this->writableFields()->remove(token);
  }

  template <typename Id>
  bool isSet() const {
    return this->getFields()->template isSet<Id>();
  }

  template <typename Id>
  void modify(bool construct = true) {
    DCHECK(!this->isPublished());

    if (this->template isSet<Id>()) {
      if constexpr (Fields::template IsChildNode<Id>) {
        auto& child = this->template ref<Id>();
        if (child->isPublished()) {
          auto clonedChild = child->clone();
          child.swap(clonedChild);
        }
      }
    } else if (construct) {
      // default construct target member
      this->template set<Id>();
    }
  }

  virtual void modify(const std::string& token, bool construct = true)
      override {
    visitMember<typename Fields::ThriftType>(
        token, [&]<class Id>(Id) { this->template modify<Id>(construct); });
  }

  static void modify(std::shared_ptr<Self>* node, std::string token) {
    auto newNode = ((*node)->isPublished()) ? (*node)->clone() : *node;
    newNode->modify(token);
    node->swap(newNode);
  }

 private:
  friend class CloneAllocator;
};

} // namespace facebook::fboss::thrift_cow
