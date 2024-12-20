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

#include <folly/DynamicConverter.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>
#include <utility>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::thrift_cow {

struct FieldBaseType {};

template <typename T>
using is_field_type = std::is_base_of<FieldBaseType, std::remove_cvref_t<T>>;
template <typename T>
constexpr bool is_field_type_v = is_field_type<T>::value;

namespace detail {

template <typename TC>
constexpr bool tc_is_struct_or_union =
    std::is_same_v<TC, apache::thrift::type_class::structure> ||
    std::is_same_v<TC, apache::thrift::type_class::variant>;

}

template <fsdb::OperProtocol Protocol>
struct ProtocolSerializers {
  static_assert(Protocol != Protocol, "Unsupported Protocol");
};

template <>
struct ProtocolSerializers<fsdb::OperProtocol::BINARY> {
  using Reader = apache::thrift::BinaryProtocolReader;
  using Writer = apache::thrift::BinaryProtocolWriter;
};

template <>
struct ProtocolSerializers<fsdb::OperProtocol::SIMPLE_JSON> {
  using Reader = apache::thrift::SimpleJSONProtocolReader;
  using Writer = apache::thrift::SimpleJSONProtocolWriter;
};

template <>
struct ProtocolSerializers<fsdb::OperProtocol::COMPACT> {
  using Reader = apache::thrift::CompactProtocolReader;
  using Writer = apache::thrift::CompactProtocolWriter;
};

template <fsdb::OperProtocol Protocol>
struct Serializer {
  using Serializers = ProtocolSerializers<Protocol>;
  using Reader = typename Serializers::Reader;
  using Writer = typename Serializers::Writer;
  using TSerializer = apache::thrift::Serializer<Reader, Writer>;

  template <typename TC, typename TType>
  static folly::fbstring serialize(const TType& ttype)
    requires(detail::tc_is_struct_or_union<TC>)
  {
    folly::fbstring encoded;
    TSerializer::serialize(ttype, &encoded);
    encoded.shrink_to_fit();
    return encoded;
  }

  template <typename TC, typename TType>
  static folly::fbstring serialize(const TType& ttype)
    requires(!detail::tc_is_struct_or_union<TC>)
  {
    auto str = serializeBuf<TC>(ttype).moveToFbString();
    str.shrink_to_fit();
    return str;
  }

  template <typename TC, typename TType>
  static folly::IOBuf serializeBuf(const TType& ttype)
    requires(detail::tc_is_struct_or_union<TC>)
  {
    folly::IOBufQueue queue;
    TSerializer::serialize(ttype, &queue);
    return queue.moveAsValue();
  }

  template <typename TC, typename TType>
  static folly::IOBuf serializeBuf(const TType& ttype)
    requires(!detail::tc_is_struct_or_union<TC>)
  {
    folly::IOBufQueue queue;
    Writer writer;
    writer.setOutput(&queue);
    apache::thrift::detail::pm::protocol_methods<TC, TType>::write(
        writer, ttype);
    return queue.moveAsValue();
  }

  template <typename TC, typename TType>
  static TType deserialize(const folly::fbstring& encoded)
    requires(detail::tc_is_struct_or_union<TC>)
  {
    return TSerializer::template deserialize<TType>(encoded.toStdString());
  }

  template <typename TC, typename TType>
  static TType deserialize(const folly::fbstring& encoded)
    requires(!detail::tc_is_struct_or_union<TC>)
  {
    Reader reader;
    auto buf = folly::IOBuf::copyBuffer(encoded.data(), encoded.length());
    reader.setInput(buf.get());
    TType recovered;
    apache::thrift::detail::pm::protocol_methods<TC, TType>::read(
        reader, recovered);
    return recovered;
  }

  template <typename TC, typename TType>
  static TType deserializeBuf(folly::IOBuf&& buf)
    requires(detail::tc_is_struct_or_union<TC>)
  {
    return TSerializer::template deserialize<TType>(&buf);
  }

  template <typename TC, typename TType>
  static TType deserializeBuf(folly::IOBuf&& buf)
    requires(!detail::tc_is_struct_or_union<TC>)
  {
    Reader reader;
    reader.setInput(&buf);
    TType recovered;
    apache::thrift::detail::pm::protocol_methods<TC, TType>::read(
        reader, recovered);
    return recovered;
  }
};

template <typename TC, typename TType>
folly::fbstring serialize(fsdb::OperProtocol proto, const TType& ttype) {
  switch (proto) {
    case fsdb::OperProtocol::BINARY:
      return Serializer<fsdb::OperProtocol::BINARY>::template serialize<TC>(
          ttype);
    case fsdb::OperProtocol::SIMPLE_JSON:
      return Serializer<fsdb::OperProtocol::SIMPLE_JSON>::template serialize<
          TC>(ttype);
    case fsdb::OperProtocol::COMPACT:
      return Serializer<fsdb::OperProtocol::COMPACT>::template serialize<TC>(
          ttype);
    default:
      throw std::logic_error("Unexpected protocol");
  }
}

template <typename TC, typename TType>
folly::IOBuf serializeBuf(fsdb::OperProtocol proto, const TType& ttype) {
  switch (proto) {
    case fsdb::OperProtocol::BINARY:
      return Serializer<fsdb::OperProtocol::BINARY>::template serializeBuf<TC>(
          ttype);
    case fsdb::OperProtocol::SIMPLE_JSON:
      return Serializer<fsdb::OperProtocol::SIMPLE_JSON>::template serializeBuf<
          TC>(ttype);
    case fsdb::OperProtocol::COMPACT:
      return Serializer<fsdb::OperProtocol::COMPACT>::template serializeBuf<TC>(
          ttype);
    default:
      throw std::logic_error("Unexpected protocol");
  }
}

template <typename TC, typename TType>
TType deserialize(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
  switch (proto) {
    case fsdb::OperProtocol::BINARY:
      return Serializer<
          fsdb::OperProtocol::BINARY>::template deserialize<TC, TType>(encoded);
    case fsdb::OperProtocol::SIMPLE_JSON:
      return Serializer<fsdb::OperProtocol::SIMPLE_JSON>::
          template deserialize<TC, TType>(encoded);
    case fsdb::OperProtocol::COMPACT:
      return Serializer<fsdb::OperProtocol::COMPACT>::
          template deserialize<TC, TType>(encoded);
    default:
      throw std::logic_error("Unexpected protocol");
  }
}

template <typename TC, typename TType>
TType deserializeBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded) {
  switch (proto) {
    case fsdb::OperProtocol::BINARY:
      return Serializer<fsdb::OperProtocol::BINARY>::
          template deserializeBuf<TC, TType>(std::move(encoded));
    case fsdb::OperProtocol::SIMPLE_JSON:
      return Serializer<fsdb::OperProtocol::SIMPLE_JSON>::
          template deserializeBuf<TC, TType>(std::move(encoded));
    case fsdb::OperProtocol::COMPACT:
      return Serializer<fsdb::OperProtocol::COMPACT>::
          template deserializeBuf<TC, TType>(std::move(encoded));
    default:
      throw std::logic_error("Unexpected protocol");
  }
}

struct Serializable {
  virtual ~Serializable() = default;

  folly::fbstring encode(fsdb::OperProtocol proto) const {
    return encodeBuf(proto).moveToFbString();
  }

  virtual folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const = 0;

  void fromEncoded(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
    auto buf =
        folly::IOBuf::wrapBufferAsValue(encoded.data(), encoded.length());
    fromEncodedBuf(proto, std::move(buf));
  }

  virtual void fromEncodedBuf(
      fsdb::OperProtocol proto,
      folly::IOBuf&& encoded) = 0;

  virtual folly::dynamic toFollyDynamic() const = 0;
};

struct ThriftObject {};

template <typename TC, typename TType>
class SerializableWrapper : public Serializable {
 public:
  using CowType = ThriftObject;
  explicit SerializableWrapper(TType& node) : node_(node) {}

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const override {
    folly::IOBufQueue queue;
    switch (proto) {
      case fsdb::OperProtocol::BINARY:
        apache::thrift::BinarySerializer::serialize(node_, &queue);
        break;
      case fsdb::OperProtocol::COMPACT:
        apache::thrift::CompactSerializer::serialize(node_, &queue);
        break;
      case fsdb::OperProtocol::SIMPLE_JSON:
        apache::thrift::SimpleJSONSerializer::serialize(node_, &queue);
        break;
      default:
        throw std::runtime_error(folly::to<std::string>(
            "Unknown protocol: ", static_cast<int>(proto)));
    }
    return queue.moveAsValue();
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded)
      override {
    node_ = deserializeBuf<TC, TType>(proto, std::move(encoded));
  }

#ifdef ENABLE_DYNAMIC_APIS
  folly::dynamic toFollyDynamic() const override {
    folly::dynamic dyn;
    facebook::thrift::to_dynamic(
        dyn, node_, facebook::thrift::dynamic_format::JSON_1);
    return dyn;
  }
#else
  folly::dynamic toFollyDynamic() const override {
    return {};
  }
#endif

 private:
  TType& node_;
};

} // namespace facebook::fboss::thrift_cow
