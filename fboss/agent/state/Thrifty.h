#pragma once

#include <folly/IPAddress.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <type_traits>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/NodeMap.h"

#include "common/network/if/gen-cpp2/Address_types.h"

namespace facebook::fboss {

// All thrift state maps need to have an items field
inline constexpr folly::StringPiece kItems{"items"};

template <typename ThriftKeyT, typename NodeT>
struct ThriftyNodeMapTraits : public NodeMapTraits<
                                  ThriftKeyT,
                                  NodeT,
                                  NodeMapNoExtraFields,
                                  std::map<ThriftKeyT, NodeT>> {
  static const std::string& getThriftKeyName() {
    static const std::string _key = "id";
    return _key;
  }

  template <typename NodeKeyT>
  static const std::string getKeyFromLegacyNode(
      const folly::dynamic& dyn,
      const std::string& keyName) {
    auto& legacyKey = dyn[keyName];
    if (legacyKey.isNull() or legacyKey.isArray()) {
      throw FbossError("key of map is null or array");
    }
    std::string key{};
    if constexpr (!is_fboss_key_object_type<NodeKeyT>::value) {
      key = legacyKey.asString();
    } else {
      // in cases where key is actually an object we need to convert this to
      // proper string representation
      key = NodeKeyT::fromFollyDynamicLegacy(legacyKey).str();
    }
    return key;
  }

  // convert key from cpp version to one thrift will like
  template <typename NodeKey>
  static const ThriftKeyT convertKey(const NodeKey& key) {
    return static_cast<ThriftKeyT>(key);
  }

  // parse dynamic key into thrift acceptable type
  static const ThriftKeyT parseKey(const folly::dynamic& key) {
    return key.asInt();
  }
};

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

  template <typename T>
  static bool listEq(
      std::vector<std::shared_ptr<T>> a,
      std::vector<std::shared_ptr<T>> b) {
    return a.size() == b.size() &&
        std::equal(
               std::begin(a),
               std::end(a),
               std::begin(b),
               std::end(b),
               [](const auto& lhs, const auto& rhs) { return *lhs == *rhs; });
  }

  template <typename T>
  static bool listEq(
      std::optional<std::vector<std::shared_ptr<T>>> a,
      std::optional<std::vector<std::shared_ptr<T>>> b) {
    return (!a.has_value() && !b.has_value()) ||
        (a.has_value() && b.has_value() && listEq(*a, *b));
  }

  static bool nodeNeedsMigration(const folly::dynamic& dyn) {
    return !dyn.isObject() ||
        !dyn.getDefault(kThriftySchemaUpToDate, false).asBool();
  }

  // given folly dynamic of ip, return binary address
  static network::thrift::BinaryAddress toThriftBinaryAddress(
      const folly::dynamic& ip) {
    return network::toBinaryAddress(folly::IPAddress(ip.asString()));
  }

  // given folly dynamic of binary address, return folly ip address
  static folly::IPAddress toFollyIPAddress(const folly::dynamic& addr) {
    auto jsonStr = folly::toJson(addr);
    auto inBuf =
        folly::IOBuf::wrapBufferAsValue(jsonStr.data(), jsonStr.size());
    auto thriftBinaryAddr = apache::thrift::SimpleJSONSerializer::deserialize<
        network::thrift::BinaryAddress>(folly::io::Cursor{&inBuf});
    return network::toIPAddress(thriftBinaryAddr);
  }

  // return folly dynamic of binary address, given binary address
  static folly::dynamic toFollyDynamic(
      const network::thrift::BinaryAddress& addr) {
    std::string jsonStr;
    apache::thrift::SimpleJSONSerializer::serialize(addr, &jsonStr);
    return folly::parseJson(jsonStr);
  }

  // return folly dynamic of ip address, given ip address
  static folly::dynamic toFollyDynamic(const folly::IPAddress& addr) {
    return addr.str();
  }

  // ip address represented dyn is converted to type AddrT
  template <typename AddrT>
  static AddrT convertAddress(const folly::dynamic& dyn) {
    static_assert(
        std::is_same_v<AddrT, folly::IPAddress> ||
            std::is_same_v<AddrT, network::thrift::BinaryAddress>,
        "invalid address type");
    if constexpr (std::is_same_v<AddrT, folly::IPAddress>) {
      return toFollyIPAddress(dyn);
    } else {
      return toThriftBinaryAddress(dyn);
    }
  }

  // change dynamic representation to that of AddrT
  template <typename AddrT>
  static void translateTo(folly::dynamic& dyn) {
    AddrT addr = convertAddress<AddrT>(dyn);
    dyn = ThriftyUtils::toFollyDynamic(addr);
  }

  static void stringToInt(folly::dynamic& dyn, const std::string& field) {
    if (auto it = dyn.find(field); it != dyn.items().end()) {
      dyn[field] = dyn[field].asInt();
    }
  }

  static void thriftyMapToFollyArray(
      folly::dynamic& dyn,
      const std::string& map,
      const std::string& key,
      const std::string& value) {
    if (dyn.find(map) != dyn.items().end()) {
      folly::dynamic follyArray = folly::dynamic::array;
      for (const auto& follyMapKey : dyn[map].keys()) {
        folly::dynamic jsonEntry = folly::dynamic::object;
        jsonEntry[key] = follyMapKey;
        jsonEntry[value] = dyn[map][follyMapKey];
        follyArray.push_back(jsonEntry);
      }
      dyn[map] = follyArray;
    }
  }

  static void follyArraytoThriftyMap(
      folly::dynamic& newDyn,
      const std::string& map,
      const std::string& key,
      const std::string& value) {
    if (newDyn.find(map) != newDyn.items().end()) {
      folly::dynamic follyMap = folly::dynamic::object;
      for (const auto& entry : newDyn[map]) {
        follyMap[entry[key].asString()] = entry[value];
      }
      newDyn[map] = follyMap;
    }
  }

  static auto constexpr kThriftySchemaUpToDate = "__thrifty_schema_uptodate";
};

template <typename Derived, typename ThriftT>
class ThriftyFields {
 public:
  ThriftyFields() {}
  explicit ThriftyFields(const ThriftT& data) : data_(data) {}

  virtual ~ThriftyFields() = default;

  //  migrateTo does not modify dyn so we don't have to change the call sites of
  //  fromFollyDynamic, migrateFrom does not have this limitation
  static folly::dynamic migrateToThrifty(const folly::dynamic& dyn) {
    return dyn;
  }
  static void migrateFromThrifty(folly::dynamic& dyn) {
    dyn[ThriftyUtils::kThriftySchemaUpToDate] = true;
  }

  using FieldsT = Derived;

  static FieldsT fromFollyDynamic(folly::dynamic const& dyn) {
    if (ThriftyUtils::nodeNeedsMigration(dyn)) {
      return fromJson(folly::toJson(FieldsT::migrateToThrifty(dyn)));
    } else {
      // Schema is up to date meaning there is no migration required
      return fromJson(folly::toJson(dyn));
    }
  }

  folly::dynamic toFollyDynamic() const {
    auto dyn = folly::parseJson(this->str());
    FieldsT::migrateFromThrifty(dyn);
    return dyn;
  }

  static FieldsT fromJson(const folly::fbstring& jsonStr) {
    auto inBuf =
        folly::IOBuf::wrapBufferAsValue(jsonStr.data(), jsonStr.size());
    auto obj = apache::thrift::SimpleJSONSerializer::deserialize<ThriftT>(
        folly::io::Cursor{&inBuf});
    return FieldsT::fromThrift(obj);
  }

  std::string str() const {
    auto obj = toThrift();
    std::string jsonStr;
    apache::thrift::SimpleJSONSerializer::serialize(obj, &jsonStr);
    return jsonStr;
  }

  virtual ThriftT toThrift() const = 0;

  const ThriftT& data() const {
    return data_;
  }

  ThriftT& writableData() {
    return data_;
  }

 protected:
  ThriftT data_;
};

// Base class to convert NodeMaps to thrift
template <typename NodeMap, typename TraitsT, typename ThriftyTraitsT>
class ThriftyNodeMapT : public NodeMapT<NodeMap, TraitsT> {
 public:
  using NodeMapT<NodeMap, TraitsT>::NodeMapT;

  using ThriftType = typename ThriftyTraitsT::NodeContainer;
  using KeyType = typename TraitsT::KeyType;

  static std::shared_ptr<NodeMap> fromThrift(
      const typename ThriftyTraitsT::NodeContainer& map) {
    auto mapObj = std::make_shared<NodeMap>();

    for (auto& node : map) {
      auto fieldsObj = TraitsT::Node::Fields::fromThrift(node.second);
      mapObj->addNode(std::make_shared<typename TraitsT::Node>(fieldsObj));
    }

    return mapObj;
  }

  static std::shared_ptr<NodeMap> fromFollyDynamic(folly::dynamic const& dyn) {
    if (ThriftyUtils::nodeNeedsMigration(dyn)) {
      return fromFollyDynamicImpl(NodeMap::migrateToThrifty(dyn));
    } else {
      // Schema is up to date meaning there is not migration required
      return fromFollyDynamicImpl(dyn);
    }
  }

  static std::shared_ptr<NodeMap> fromFollyDynamicImpl(
      folly::dynamic const& dyn) {
    typename ThriftyTraitsT::NodeContainer mapTh;
    for (auto& [key, val] : dyn.items()) {
      if (key == kEntries || key == kExtraFields ||
          key == ThriftyUtils::kThriftySchemaUpToDate) {
        continue;
      }
      auto jsonStr = folly::toJson(val);
      auto inBuf =
          folly::IOBuf::wrapBufferAsValue(jsonStr.data(), jsonStr.size());
      auto node = apache::thrift::SimpleJSONSerializer::deserialize<
          typename ThriftyTraitsT::Node>(folly::io::Cursor{&inBuf});
      mapTh[ThriftyTraitsT::parseKey(key)] = node;
    }
    return fromThrift(mapTh);
  }

  typename ThriftyTraitsT::NodeContainer toThrift() const {
    typename ThriftyTraitsT::NodeContainer items;
    for (auto& node : *this) {
      items[getNodeThriftKey(node)] = node->getFields()->toThrift();
    }

    return items;
  }

  folly::dynamic toFollyDynamic() const override final {
    auto obj = this->toThrift();
    folly::dynamic dyn = folly::dynamic::object();
    for (auto& [key, val] : obj) {
      std::string jsonStr;
      apache::thrift::SimpleJSONSerializer::serialize(val, &jsonStr);
      dyn[folly::to<std::string>(key)] = folly::parseJson(jsonStr);
    }

    NodeMap::migrateFromThrifty(dyn);
    return dyn;
  }

  /*
   * Old style NodeMapT serlization has the nodes in a list but with thrift we
   * can probably just encode a map directly. So in thrift we'll use the name
   * "items" instead of "entries and to migrate we'll duplicate the data as a
   * list under "entries" and a map under "items"
   */
  static folly::dynamic migrateToThrifty(const folly::dynamic& dyn) {
    folly::dynamic newItems = folly::dynamic::object;
    auto* entries = getEntries(dyn);
    for (auto& item : *entries) {
      if (ThriftyUtils::nodeNeedsMigration(item)) {
        auto key = ThriftyTraitsT::template getKeyFromLegacyNode<KeyType>(
            item, ThriftyTraitsT::getThriftKeyName());
        newItems[key] = TraitsT::Node::Fields::migrateToThrifty(item);
      } else {
        newItems[item[ThriftyTraitsT::getThriftKeyName()].asString()] = item;
      }
    }
    return newItems;
  }

  static void migrateFromThrifty(folly::dynamic& dyn) {
    auto schemaUpToDate = true;
    folly::dynamic entries = folly::dynamic::array;
    std::set<std::string> keys{};

    for (auto& item : dyn.items()) {
      // dyn.items() is an uordered map and the order in which items are
      // returned is underterministic. enforce the order, so entries are always
      // in the same order.
      keys.insert(item.first.asString());
    }

    for (auto key : keys) {
      auto& item = dyn[key];
      TraitsT::Node::Fields::migrateFromThrifty(item);
      if (!item.getDefault(ThriftyUtils::kThriftySchemaUpToDate, false)
               .asBool()) {
        schemaUpToDate = false;
      }
      entries.push_back(item);
    }

    dyn[kEntries] = entries;
    // TODO: fill out extra fields as needed
    dyn[kExtraFields] = folly::dynamic::object;

    dyn[ThriftyUtils::kThriftySchemaUpToDate] = schemaUpToDate;
  }

  // for testing purposes. These are mirrors of to/from FollyDynamic defined in
  // NodeMap, but calling the legacy conversions of the node as well
  folly::dynamic toFollyDynamicLegacy() const {
    folly::dynamic nodesJson = folly::dynamic::array;
    for (const auto& node : *this) {
      nodesJson.push_back(node->toFollyDynamicLegacy());
    }
    folly::dynamic json = folly::dynamic::object;
    json[kEntries] = std::move(nodesJson);
    // TODO: extra files if needed
    json[kExtraFields] = folly::dynamic::object();
    return json;
  }

  static std::shared_ptr<NodeMap> fromFollyDynamicLegacy(
      folly::dynamic const& dyn) {
    auto nodeMap = std::make_shared<NodeMap>();
    auto entries = dyn[kEntries];
    for (const auto& entry : entries) {
      nodeMap->addNode(TraitsT::Node::fromFollyDynamicLegacy(entry));
    }
    // TODO: extra files if needed
    return nodeMap;
  }

  // return node id in the node map as it would be represented in thrift
  static typename ThriftyTraitsT::KeyType getNodeThriftKey(
      const std::shared_ptr<typename TraitsT::Node>& node) {
    return ThriftyTraitsT::convertKey(TraitsT::getKey(node));
  }

  bool operator==(
      const ThriftyNodeMapT<NodeMap, TraitsT, ThriftyTraitsT>& rhs) const {
    if (this->size() != rhs.size()) {
      return false;
    }
    for (auto& node : *this) {
      if (auto other = rhs.getNodeIf(TraitsT::getKey(node));
          !other || *node != *other) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(
      const ThriftyNodeMapT<NodeMap, TraitsT, ThriftyTraitsT>& rhs) const {
    return !(*this == rhs);
  }

 private:
  static const folly::dynamic* getEntries(const folly::dynamic& dyn) {
    if (dyn.isArray()) {
      return &dyn;
    }
    CHECK(dyn.isObject());
    return &dyn[kEntries];
  }
};

//
// Special version of NodeBaseT that features methods to convert
// object to/from JSON using Thrift serializers. For this to work
// one must supply Thrift type (ThrifT) that stores FieldsT state.
//
// TODO: in future, FieldsT and ThrifT should be one type
//
template <typename ThriftT, typename NodeT, typename FieldsT>
class ThriftyBaseT : public NodeBaseT<NodeT, FieldsT> {
  static_assert(std::is_base_of_v<ThriftyFields<FieldsT, ThriftT>, FieldsT>);

 public:
  using NodeBaseT<NodeT, FieldsT>::NodeBaseT;
  using Fields = FieldsT;
  using ThriftType = ThriftT;

  static std::shared_ptr<NodeT> fromThrift(const ThriftT& obj) {
    auto fields = FieldsT::fromThrift(obj);
    return std::make_shared<NodeT>(fields);
  }

  static std::shared_ptr<NodeT> fromFollyDynamic(folly::dynamic const& dyn) {
    if (ThriftyUtils::nodeNeedsMigration(dyn)) {
      return fromJson(folly::toJson(FieldsT::migrateToThrifty(dyn)));
    } else {
      // Schema is up to date meaning there is no migration required
      return fromJson(folly::toJson(dyn));
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

  folly::dynamic toFollyDynamic() const override final {
    auto dyn = folly::parseJson(this->str());
    FieldsT::migrateFromThrifty(dyn);
    return dyn;
  }

  // for testing purposes
  folly::dynamic toFollyDynamicLegacy() const {
    return this->getFields()->toFollyDynamicLegacy();
  }

  static std::shared_ptr<NodeT> fromFollyDynamicLegacy(
      folly::dynamic const& dyn) {
    return std::make_shared<NodeT>(FieldsT::fromFollyDynamicLegacy(dyn));
  }

  bool operator==(const ThriftyBaseT<ThriftT, NodeT, FieldsT>& rhs) const {
    return *this->getFields() == *rhs.getFields();
  }

  bool operator!=(const ThriftyBaseT<ThriftT, NodeT, FieldsT>& rhs) const {
    return !(*this == rhs);
  }
};
} // namespace facebook::fboss
