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
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/thrift_cow/nodes/NodeUtils.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"

namespace facebook::fboss::thrift_cow {

template <typename TypeClass, typename TType>
struct ThriftHybridNode : public thrift_cow::Serializable {
 public:
  using ThriftType = TType;
  using CowType = HybridNodeType;
  using Self = ThriftHybridNode<TypeClass, TType>;
  using PathIter = typename std::vector<std::string>::const_iterator;

  ThriftHybridNode() = default;
  explicit ThriftHybridNode(TType obj) : obj_(std::move(obj)) {}

  template <typename T = Self>
  auto set(const TType& obj) {
    obj_ = obj;
  }

  void operator=(const TType& obj) {
    set(obj);
  }

  void operator==(const Self& other) {
    return obj_ == other.obj_;
  }

  void operator!=(const Self& other) {
    return obj_ != other.obj_;
  }

  void operator<(const Self& other) {
    return obj_ < other.obj_;
  }

  TType operator*() const {
    return obj_;
  }

  TType toThrift() const {
    return obj_;
  }

  template <typename T = Self>
  auto fromThrift(const TType& obj) {
    set(obj);
  }

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const override {
    return serializeBuf<TypeClass>(proto, toThrift());
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded)
      override {
    fromThrift(deserializeBuf<TypeClass, TType>(proto, std::move(encoded)));
  }

  bool remove(const std::string& token) {
    throw std::runtime_error(folly::to<std::string>(
        "Cannot remove a child from a ThriftHybridNode: ", token));
  }

  bool remove(const std::string& token) const {
    throw std::runtime_error(folly::to<std::string>(
        "Cannot remove a child from a ThriftHybridNode: ", token));
  }

  void modify(const std::string&) {}

  TType& ref() {
    return obj_;
  }

  const TType& ref() const {
    return obj_;
  }

  const TType& cref() const {
    return obj_;
  }

  std::size_t hash() const {
    return std::hash<TType>()(obj_);
  }

#ifdef ENABLE_DYNAMIC_APIS
  folly::dynamic toFollyDynamic() const {
    return folly::toDynamic(this->ref());
  }

  // this would override the underlying thrift object
  void fromFollyDynamic(const folly::dynamic& obj) {
    facebook::thrift::from_dynamic(
        this->ref(), obj, facebook::thrift::dynamic_format::JSON_1);
  }
#else
  folly::dynamic toFollyDynamic() const {
    return {};
  }
#endif

 private:
  TType obj_{};
};

} // namespace facebook::fboss::thrift_cow
