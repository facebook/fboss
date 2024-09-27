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
#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/thrift_cow/nodes/Types.h"

#include <variant>

namespace facebook::fboss::thrift_cow {

template <typename Fields>
struct ThriftUnionStorage;

namespace union_helpers {

FATAL_GET_MEMBER_TYPE(ttype);

struct IsChild {
  template <typename Traits>
  using apply = typename Traits::isChild;
};

// This is templated on the ThriftUnionFields type and is used in
// fatal::foreach to copy from a thrift struct to our underlying
// storage type (ThriftUnionStorage)
template <typename FieldsT>
struct CopyToMember {
  using Storage = ThriftUnionStorage<FieldsT>;
  using TType = typename FieldsT::ThriftType;

  template <typename T>
  void operator()(fatal::tag<T>, Storage& storage, const TType& rhs) {
    using member = typename T::member;
    using name = typename T::name;

    if (rhs.getType() == member::metadata::id::value) {
      storage.template set<name>(typename member::getter{}(rhs));
    }
  }
};

// This is templated on the ThriftUnionFields type and is used in
// fatal::foreach to copy from our underlying storage type
// (ThriftUnionStorage) to a thrift struct
template <typename FieldsT>
struct CopyFromMember {
  using Storage = ThriftUnionStorage<FieldsT>;
  using TType = typename FieldsT::ThriftType;

  template <typename T>
  void operator()(fatal::tag<T>, TType& thrift, const Storage& storage) {
    using member = typename T::member;
    using name = typename T::name;

    if (storage.type() == member::metadata::id::value) {
      member::set(thrift, storage.template cref<name>()->toThrift());
    }
  }
};

// Invoke a function on each child. Expects functions that take a raw
// pointer to a node.
template <typename FieldsT>
struct ChildInvoke {
  using Storage = ThriftUnionStorage<FieldsT>;

  template <typename T, typename Fn>
  void operator()(fatal::tag<T>, Storage& storage, Fn fn) {
    using member = typename T::member;
    using name = typename T::name;

    if (storage.type() != member::metadata::id::value) {
      return;
    }

    using ChildType = typename FieldsT::template TypeFor<name>;
    ChildType value = storage.template get<name>();
    if (value) {
      fn(value.get());
    }
  }
};

template <typename FieldsT>
struct MemberReset {
  using Storage = ThriftUnionStorage<FieldsT>;

  template <typename T>
  void operator()(fatal::tag<T>, Storage& storage) {
    using name = typename T::name;
    storage.template resetMember<name>();
  }
};

template <typename Name>
std::string nameString() {
  return std::string(fatal::z_data<Name>(), fatal::size<Name>::value);
}

} // namespace union_helpers

template <typename Fields>
struct ThriftUnionStorage {
  using TypeEnum = typename Fields::TypeEnum;
  using StorageType = typename Fields::NamedMemberTypes;

  template <typename Name>
  using MetadataFor = typename Fields::template MetadataFor<Name>;

  template <typename T>
  using TypeFor = typename Fields::template TypeFor<T>;
  template <typename T>
  using ThriftTypeFor = typename Fields::template ThriftTypeFor<T>;

  template <typename Name>
  TypeFor<Name> get() const {
    using Metadata = MetadataFor<Name>;
    if (Metadata::id::value == type_) {
      return storage_.template get<Name>();
    }

    throw std::bad_variant_access();
  }

  template <typename Name>
  TypeFor<Name>& ref() {
    using Metadata = MetadataFor<Name>;
    if (Metadata::id::value == type_) {
      return storage_.template get<Name>();
    }

    throw std::bad_variant_access();
  }

  template <typename Name>
  const TypeFor<Name>& ref() const {
    using Metadata = MetadataFor<Name>;
    if (Metadata::id::value == type_) {
      return storage_.template get<Name>();
    }

    throw std::bad_variant_access();
  }

  template <typename Name>
  const TypeFor<Name>& cref() const {
    using Metadata = MetadataFor<Name>;
    if (Metadata::id::value == type_) {
      return storage_.template get<Name>();
    }

    throw std::bad_variant_access();
  }

  template <typename Name, typename... Args>
  void set(Args&&... args) {
    clear();
    if constexpr (Fields::template IsChildNode<Name>::value) {
      using MemberType = typename Fields::template TypeFor<Name>::element_type;
      static_assert(
          std::is_constructible_v<MemberType, Args...>,
          "Unexpected args for union set()");

      storage_.template get<Name>() =
          std::make_shared<MemberType>(std::forward<Args>(args)...);
    } else {
      using MemberType = typename Fields::template TypeFor<Name>::value_type;
      static_assert(
          std::is_constructible_v<MemberType, Args...>,
          "Unexpected args for union set()");

      storage_.template get<Name>() =
          std::make_optional<MemberType>(std::forward<Args>(args)...);
    }

    using Metadata = MetadataFor<Name>;
    type_ = static_cast<TypeEnum>(Metadata::id::value);
  }

  void clear() {
    fatal::foreach<typename Fields::MemberTypes>(
        union_helpers::MemberReset<Fields>(), *this);
  }

  template <typename Name>
  bool resetMember() {
    auto& value = storage_.template get<Name>();
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
struct ThriftUnionFields {
  using Self = ThriftUnionFields<TType>;

  // reflected union metadata
  using Info = apache::thrift::reflect_variant<TType>;

  using CowType = FieldsType;
  using ThriftType = TType;
  using TC = apache::thrift::type_class::variant;
  using TypeEnum = typename Info::traits::id;

  // type list of reflected_struct_member
  using Members = typename Info::traits::descriptors;

  // Extracting useful common types out of each member via Traits.h
  using MemberTypes = fatal::transform<Members, ExtractUnionFields>;

  // This is our ultimate storage type, which is effectively a
  // std::tuple with syntactic sugar for accessing based on
  // these constexpr strings. We wrap it in a ThriftUnionStorage class
  // which ensures at most one tuple value is set at any given time
  using NamedMemberTypes = typename fatal::tuple_from<MemberTypes>::
      template list<fatal::get_member_type::type, fatal::get_member_type::name>;

  using ChildrenTypes = fatal::filter<MemberTypes, union_helpers::IsChild>;

  template <typename Name>
  using MetadataFor = typename Info::template by_name<Name>::metadata;

  template <typename Name>
  using TypeFor = typename fatal::find<
      MemberTypes,
      Name,
      std::false_type,
      fatal::get_type::name,
      fatal::get_type::type>;

  template <typename Name>
  using ThriftTypeFor = typename Info::template by_name<Name>::type;

  template <typename Name>
  using IsChildNode =
      typename fatal::contains<ChildrenTypes, Name, fatal::get_type::name>;

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
    fatal::foreach<MemberTypes>(
        union_helpers::CopyFromMember<Self>(), thrift, storage_);
    return thrift;
  }

  template <typename T>
  void fromThrift(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    fatal::foreach<MemberTypes>(
        union_helpers::CopyToMember<Self>(), storage_, std::forward<T>(thrift));
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

  template <typename Name>
  bool isSet() const {
    using Metadata = MetadataFor<Name>;
    return Metadata::id::value == type();
  }

  template <typename Name>
  TypeFor<Name> get() const {
    return storage_.template get<Name>();
  }

  /*
   * References to underlying field.
   *
   * TODO: create ref() wrapper class that respects if the node is
   * published.
   */
  template <typename Name>
  TypeFor<Name>& ref() {
    return storage_.template ref<Name>();
  }

  template <typename Name>
  const TypeFor<Name>& ref() const {
    return storage_.template ref<Name>();
  }

  template <typename Name>
  const TypeFor<Name>& cref() const {
    return storage_.template cref<Name>();
  }

  /*
   * Setters.
   */

  template <typename Name, typename... Args>
  void set(Args&&... args) {
    storage_.template set<Name>(std::forward<Args>(args)...);
  }

  template <typename Name>
  bool remove() {
    return storage_.template resetMember<Name>();
  }

  bool remove(const std::string& token) {
    bool found{false}, ret{false};
    visitMember<MemberTypes>(token, [&](auto tag) {
      using name = typename decltype(fatal::tag_type(tag))::name;
      found = true;
      ret = this->template remove<name>();
    });

    if (!found) {
      throw std::runtime_error(
          folly::to<std::string>("Invalid union child name: ", token));
    }

    return ret;
  }

  template <typename Fn>
  void forEachChild(Fn fn) {
    fatal::foreach<ChildrenTypes>(
        union_helpers::ChildInvoke<Self>(), storage_, std::forward<Fn>(fn));
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

  template <typename Name>
  auto get() const {
    return this->getFields()->template get<Name>();
  }

  template <typename Name, typename... Args>
  void set(Args&&... args) {
    this->writableFields()->template set<Name>(std::forward<Args>(args)...);
  }

  template <typename Name>
  typename Fields::template TypeFor<Name>& ref() {
    return this->writableFields()->template ref<Name>();
  }

  template <typename Name>
  const typename Fields::template TypeFor<Name>& ref() const {
    return this->writableFields()->template ref<Name>();
  }

  template <typename Name>
  const typename Fields::template TypeFor<Name>& cref() const {
    return this->getFields()->template cref<Name>();
  }

  // prefer safe_ref/safe_cref for safe access
  template <typename Name>
  auto safe_ref() {
    return detail::ref(this->writableFields()->template ref<Name>());
  }

  template <typename Name>
  auto safe_cref() const {
    return detail::cref(this->getFields()->template cref<Name>());
  }

  template <typename Name>
  bool remove() {
    // TODO: what to do when removing a member that isn't set? Should
    // we error?
    return this->writableFields()->template remove<Name>();
  }

  bool remove(const std::string& token) {
    return this->writableFields()->remove(token);
  }

  template <typename Name>
  bool isSet() const {
    return this->getFields()->template isSet<Name>();
  }

  template <typename Name>
  void modify(bool construct = true) {
    DCHECK(!this->isPublished());

    if (this->template isSet<Name>()) {
      if constexpr (Fields::template IsChildNode<Name>::value) {
        auto& child = this->template ref<Name>();
        if (child->isPublished()) {
          auto clonedChild = child->clone();
          child.swap(clonedChild);
        }
      }
    } else if (construct) {
      // default construct target member
      this->template set<Name>();
    }
  }

  virtual void modify(const std::string& token, bool construct = true) {
    visitMember<typename Fields::MemberTypes>(token, [&](auto tag) {
      using name = typename decltype(fatal::tag_type(tag))::name;
      this->template modify<name>(construct);
    });
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
