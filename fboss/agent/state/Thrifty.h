#pragma once

#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <type_traits>

#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

class ThriftyUtils {
 public:
  static void renameField(
      folly::dynamic& dyn,
      const std::string& oldName,
      const std::string& newName) {
    if (auto it = dyn.find(oldName); it != dyn.items().end()) {
      // keep old value for backwards compat
      dyn.insert(newName, it->second);
    }
  }

  template <typename EnumT>
  static void changeEnumToInt(folly::dynamic& dyn, const std::string& key) {
    if (auto it = dyn.find(key); it != dyn.items().end()) {
      EnumT val;
      if (apache::thrift::util::tryParseEnum(it->second.asString(), &val)) {
        it->second = static_cast<int>(val);
      }
    }
  }

  template <typename EnumT>
  static void changeEnumToString(folly::dynamic& dyn, const std::string& key) {
    if (auto it = dyn.find(key); it != dyn.items().end()) {
      try {
        it->second = apache::thrift::util::enumName(
            static_cast<EnumT>(it->second.asInt()));
      } catch (const folly::TypeError&) {
        // no problem, it was already a string
      }
    }
  }

  static auto constexpr kThriftySchemaUpToDate = "__thrifty_schema_uptodate";
};

class ThriftyFields {
 public:
  //  migrateTo does not modify dyn so we don't have to change the call sites of
  //  fromFollyDynamic, migrateFrom does not have this limitation
  static folly::dynamic migrateToThrifty(const folly::dynamic& dyn) {
    return dyn;
  }
  static void migrateFromThrifty(folly::dynamic& dyn) {
    dyn[ThriftyUtils::kThriftySchemaUpToDate] = true;
  }
};

//
// Special version of NodeBaseT that features methods to convert
// object to/from JSON using Thrift serializers. For this to work
// one must supply Thrift type (ThrifT) that stores FieldsT state.
//
// TODO: in future, FieldsT and ThrifT should be one type
//
template <
    typename ThriftT,
    typename NodeT,
    typename FieldsT,
    typename =
        std::enable_if_t<std::is_base_of_v<ThriftyFields, FieldsT>, void>>
class ThriftyBaseT : public NodeBaseT<NodeT, FieldsT> {
 public:
  using NodeBaseT<NodeT, FieldsT>::NodeBaseT;

  static std::shared_ptr<NodeT> fromThrift(const ThriftT& obj) {
    auto fields = FieldsT::fromThrift(obj);
    return std::make_shared<NodeT>(fields);
  }

  static std::shared_ptr<NodeT> fromFollyDynamic(folly::dynamic const& dyn) {
    if (dyn.getDefault(ThriftyUtils::kThriftySchemaUpToDate, false).asBool()) {
      // Schema is up to date meaning there is not migration required
      return fromJson(folly::toJson(dyn));
    } else {
      return fromJson(folly::toJson(FieldsT::migrateToThrifty(dyn)));
    }
  }

  static std::shared_ptr<NodeT> fromJson(const folly::fbstring& jsonStr) {
    auto inBuf =
        folly::IOBuf::wrapBufferAsValue(jsonStr.data(), jsonStr.size());
    auto obj = apache::thrift::SimpleJSONSerializer::deserialize<ThriftT>(
        folly::io::Cursor{&inBuf});
    return ThriftyBaseT::fromThrift(obj);
  }

  ThriftT toThrift() const {
    return this->getFields()->toThrift();
  }

  std::string str() const {
    auto obj = this->toThrift();
    std::string jsonStr;
    apache::thrift::SimpleJSONSerializer::serialize(obj, &jsonStr);
    return jsonStr;
  }

  folly::dynamic toFollyDynamic() const override {
    auto dyn = folly::parseJson(this->str());
    FieldsT::migrateFromThrifty(dyn);
    return dyn;
  }
};
} // namespace facebook::fboss
