/*
 *  Copyright (c) 2024-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/DynamicConverter.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"

namespace facebook::fboss::thrift_cow {

// HybridNodeFields wraps a Thrift struct object
template <typename TType>
struct HybridNodeFields {
  HybridNodeFields() = default;

  explicit HybridNodeFields(const TType& obj) : obj_(obj) {}

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  TType& ref() {
    return obj_;
  }

  const TType& ref() const {
    return obj_;
  }

  const TType& cref() const {
    return obj_;
  }

  void set(const TType& obj) {
    obj_ = obj;
  }

  TType toThrift() const {
    return obj_;
  }

#ifdef ENABLE_DYNAMIC_APIS
  folly::dynamic toFollyDynamic() const {
    folly::dynamic dyn;
    facebook::thrift::to_dynamic(
        dyn, obj_, facebook::thrift::dynamic_format::JSON_1);
    return dyn;
  }

  void fromFollyDynamic(const folly::dynamic& obj) {
    facebook::thrift::from_dynamic(
        obj_, obj, facebook::thrift::dynamic_format::JSON_1);
  }
#else
  folly::dynamic toFollyDynamic() const {
    return {};
  }
#endif

 private:
  TType obj_{};
};

// ThriftHybridNode is a Node holding a wrapped Thrift object
template <typename TypeClass, typename TType>
struct ThriftHybridNode : public fboss::NodeBaseT<
                              ThriftHybridNode<TypeClass, TType>,
                              HybridNodeFields<TType>>,
                          public thrift_cow::Serializable {
 public:
  using ThriftType = TType;
  using TC = TypeClass;
  using BaseT =
      NodeBaseT<ThriftHybridNode<TypeClass, TType>, HybridNodeFields<TType>>;
  using CowType = HybridNodeType;
  using Self = ThriftHybridNode<TypeClass, TType>;
  using PathIter = typename std::vector<std::string>::const_iterator;

  ThriftHybridNode() = default;
  explicit ThriftHybridNode(TType obj) : BaseT(std::move(obj)) {}
  explicit ThriftHybridNode(const Self* other) : BaseT(other->cref()) {}

  template <typename T = Self>
  auto set(const TType& obj) {
    this->writableFields()->set(obj);
  }

  void operator=(const TType& obj) {
    set(obj);
  }

  void operator==(const Self& other) {
    return this->toThrift() == other.toThrift();
  }

  void operator!=(const Self& other) {
    return this->toThrift() != other.toThrift();
  }

  void operator<(const Self& other) {
    return this->toThrift() < other.toThrift();
  }

  TType operator*() const {
    return this->toThrift();
  }

  TType toThrift() const {
    return this->getFields()->toThrift();
  }

  template <typename T = Self>
  auto fromThrift(const TType& obj) {
    set(obj);
  }

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const override {
    return serializeBuf<TypeClass>(proto, cref());
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded)
      override {
    fromThrift(deserializeBuf<TypeClass, TType>(proto, std::move(encoded)));
  }

  virtual void modify(const std::string& token, bool construct = true)
      override {
    if (construct) {
      SerializableWrapper<TypeClass, TType> wrapper(ref());
      wrapper.modify(token, construct);
    }
  }

  virtual bool remove(const std::string& token) override {
    SerializableWrapper<TypeClass, TType> wrapper(ref());
    return wrapper.remove(token);
  }

  TType& ref() {
    return this->writableFields()->ref();
  }

  const TType& ref() const {
    return this->getFields()->ref();
  }

  const TType& cref() const {
    return this->getFields()->cref();
  }

  std::size_t hash() const {
    return std::hash<TType>()(cref());
  }

  folly::dynamic toFollyDynamic() const override {
    return this->getFields()->toFollyDynamic();
  }

  void fromFollyDynamic(const folly::dynamic& value) {
    return this->writableFields()->fromFollyDynamic(value);
  }
};

} // namespace facebook::fboss::thrift_cow
