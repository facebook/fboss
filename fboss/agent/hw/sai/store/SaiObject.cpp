// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/api/AclApi.h"

namespace facebook {
namespace fboss {

template <>
folly::dynamic
SaiObject<SaiNextHopGroupTraits>::adapterHostKeyToFollyDynamic() {
  folly::dynamic json = folly::dynamic::array;
  for (auto& ahk : adapterHostKey_) {
    folly::dynamic object = folly::dynamic::object;
    object[AttributeName<SaiIpNextHopTraits::Attributes::Type>::value] =
        folly::to<std::string>(ahk.first.index());

    if (auto ipAhk =
            std::get_if<SaiIpNextHopTraits::AdapterHostKey>(&ahk.first)) {
      object[AttributeName<
          SaiIpNextHopTraits::Attributes::RouterInterfaceId>::value] =
          folly::to<std::string>(
              std::get<SaiIpNextHopTraits::Attributes::RouterInterfaceId>(
                  *ipAhk)
                  .value());
      object[AttributeName<SaiIpNextHopTraits::Attributes::Ip>::value] =
          std::get<SaiIpNextHopTraits::Attributes::Ip>(*ipAhk).value().str();
    } else if (
        auto mplsAhk =
            std::get_if<SaiMplsNextHopTraits::AdapterHostKey>(&ahk.first)) {
      object[AttributeName<
          SaiMplsNextHopTraits::Attributes::RouterInterfaceId>::value] =
          folly::to<std::string>(
              std::get<SaiMplsNextHopTraits::Attributes::RouterInterfaceId>(
                  *mplsAhk)
                  .value());
      object[AttributeName<SaiMplsNextHopTraits::Attributes::Ip>::value] =
          std::get<SaiMplsNextHopTraits::Attributes::Ip>(*mplsAhk)
              .value()
              .str();
      object
          [AttributeName<SaiMplsNextHopTraits::Attributes::LabelStack>::value] =
              folly::dynamic::array;
      for (auto label :
           std::get<SaiMplsNextHopTraits::Attributes::LabelStack>(*mplsAhk)
               .value()) {
        object
            [AttributeName<SaiMplsNextHopTraits::Attributes::LabelStack>::value]
                .push_back(folly::to<std::string>(label));
      }
    }

    object[AttributeName<
        SaiNextHopGroupMemberTraits::Attributes::Weight>::value] = ahk.second;

    json.push_back(object);
  }
  return json;
}

template <>
typename SaiNextHopGroupTraits::AdapterHostKey
SaiObject<SaiNextHopGroupTraits>::follyDynamicToAdapterHostKey(
    folly::dynamic json) {
  SaiNextHopGroupTraits::AdapterHostKey key;
  for (auto object : json) {
    auto type =
        object[AttributeName<SaiIpNextHopTraits::Attributes::Type>::value]
            .asInt();

    // D32229488 adds logic to write weight to switch_state's nhop group.
    // While warmbooting from pre-D32229488 to post-D32229488, default the
    // weight to 1 (default for SAI_NEXT_HOP_GROUP_MEMBER_ATTR_WEIGHT).
    // UCMP would only be enabled after D32229488 is rolled out, so OK to
    // use default weight.
    sai_uint32_t weight = 1;
    if (object.find(AttributeName<
                    SaiNextHopGroupMemberTraits::Attributes::Weight>::value) !=
        object.items().end()) {
      weight =
          object[AttributeName<
                     SaiNextHopGroupMemberTraits::Attributes::Weight>::value]
              .asInt();
    }

    switch (type) {
      case SAI_NEXT_HOP_TYPE_IP: {
        SaiIpNextHopTraits::AdapterHostKey ipAhk;

        std::get<SaiIpNextHopTraits::Attributes::RouterInterfaceId>(ipAhk) =
            folly::to<sai_object_id_t>(
                object[AttributeName<SaiIpNextHopTraits::Attributes::
                                         RouterInterfaceId>::value]
                    .asString());
        std::get<SaiIpNextHopTraits::Attributes::Ip>(ipAhk) = folly::IPAddress(
            object[AttributeName<SaiIpNextHopTraits::Attributes::Ip>::value]
                .asString());
        key.insert(std::make_pair(ipAhk, weight));
      } break;

      case SAI_NEXT_HOP_TYPE_MPLS: {
        SaiMplsNextHopTraits::AdapterHostKey mplsAhk;

        std::get<SaiMplsNextHopTraits::Attributes::RouterInterfaceId>(mplsAhk) =
            folly::to<sai_object_id_t>(
                object[AttributeName<SaiMplsNextHopTraits::Attributes::
                                         RouterInterfaceId>::value]
                    .asString());
        std::get<SaiMplsNextHopTraits::Attributes::Ip>(mplsAhk) =
            folly::IPAddress(
                object
                    [AttributeName<SaiMplsNextHopTraits::Attributes::Ip>::value]
                        .asString());
        std::vector<sai_uint32_t> stack;
        for (auto label : object[AttributeName<
                 SaiMplsNextHopTraits::Attributes::LabelStack>::value]) {
          stack.push_back(

              label.asInt());
        }
        std::get<SaiMplsNextHopTraits::Attributes::LabelStack>(mplsAhk) = stack;
        key.insert(std::make_pair(mplsAhk, weight));
      } break;

      default:
        XLOG(FATAL) << "unsupported next hop type " << type;
    }
  }
  return key;
}

template <>
folly::dynamic SaiObject<SaiLagTraits>::adapterHostKeyToFollyDynamic() {
  const auto& value = adapterHostKey_.value();
  std::string label{std::begin(value), std::end(value)};
  return label;
}

template <>
typename SaiLagTraits::AdapterHostKey
SaiObject<SaiLagTraits>::follyDynamicToAdapterHostKey(folly::dynamic json) {
  std::string label = json.asString();
  SaiLagTraits::AdapterHostKey::ValueType key{};
  std::copy(std::begin(label), std::end(label), std::begin(key));
  return key;
}

#if defined(SAI_VERSION_8_2_0_0_ODP) ||         \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) ||     \
    defined(SAI_VERSION_9_2_0_0_ODP) ||         \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||     \
    defined(SAI_VERSION_9_0_EA_SIM_ODP) ||      \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) || \
    defined(SAI_VERSION_10_0_EA_DNX_ODP)

// Store Wred adapter keys into hw switch state starting 8.2
template <typename attrT>
void addOptionalAttrToArray(
    folly::dynamic& array,
    const SaiWredTraits::AdapterHostKey& adapterHostKey) {
  if (auto optionalAttr = std::get<std::optional<attrT>>(adapterHostKey)) {
    array.push_back(optionalAttr.value().value());
  } else {
    array.push_back("None");
  }
}

template <>
folly::dynamic SaiObject<SaiWredTraits>::adapterHostKeyToFollyDynamic() {
  folly::dynamic array = folly::dynamic::array;
  array.push_back(
      std::get<SaiWredTraits::Attributes::GreenEnable>(adapterHostKey_)
          .value());
  addOptionalAttrToArray<SaiWredTraits::Attributes::GreenMinThreshold>(
      array, adapterHostKey_);
  addOptionalAttrToArray<SaiWredTraits::Attributes::GreenMaxThreshold>(
      array, adapterHostKey_);
  addOptionalAttrToArray<SaiWredTraits::Attributes::GreenDropProbability>(
      array, adapterHostKey_);
  array.push_back(
      std::get<SaiWredTraits::Attributes::EcnMarkMode>(adapterHostKey_)
          .value());
  addOptionalAttrToArray<SaiWredTraits::Attributes::EcnGreenMinThreshold>(
      array, adapterHostKey_);
  addOptionalAttrToArray<SaiWredTraits::Attributes::EcnGreenMaxThreshold>(
      array, adapterHostKey_);
  return array;
}

template <typename attrT>
void pupulateOptionalAttrtToKey(
    folly::dynamic& array,
    SaiWredTraits::AdapterHostKey& adapterHostKey,
    int index) {
  if (!array[index].isString()) {
    std::get<std::optional<attrT>>(adapterHostKey) = array[index].asInt();
  }
}

template <>
typename SaiWredTraits::AdapterHostKey
SaiObject<SaiWredTraits>::follyDynamicToAdapterHostKey(folly::dynamic json) {
  SaiWredTraits::AdapterHostKey key;
  std::get<SaiWredTraits::Attributes::GreenEnable>(key) = json[0].asBool();
  pupulateOptionalAttrtToKey<SaiWredTraits::Attributes::GreenMinThreshold>(
      json, key, 1);
  pupulateOptionalAttrtToKey<SaiWredTraits::Attributes::GreenMaxThreshold>(
      json, key, 2);
  pupulateOptionalAttrtToKey<SaiWredTraits::Attributes::GreenDropProbability>(
      json, key, 3);
  std::get<SaiWredTraits::Attributes::EcnMarkMode>(key) = json[4].asInt();
  pupulateOptionalAttrtToKey<SaiWredTraits::Attributes::EcnGreenMinThreshold>(
      json, key, 5);
  pupulateOptionalAttrtToKey<SaiWredTraits::Attributes::EcnGreenMaxThreshold>(
      json, key, 6);
  return key;
}

#endif

template <>
folly::dynamic SaiObject<SaiAclTableTraits>::adapterHostKeyToFollyDynamic() {
  return adapterHostKey_;
}

template <>
typename SaiAclTableTraits::AdapterHostKey
SaiObject<SaiAclTableTraits>::follyDynamicToAdapterHostKey(
    folly::dynamic json) {
  return json.asString();
}
} // namespace fboss
} // namespace facebook
