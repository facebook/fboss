// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/api/AclApi.h"

namespace facebook {
namespace fboss {

template <>
folly::dynamic
SaiObject<SaiNextHopGroupTraits>::adapterHostKeyToFollyDynamic() {
  folly::dynamic json = folly::dynamic::object;
  folly::dynamic memberList = folly::dynamic::array;
  for (auto& ahk : adapterHostKey_.nhopMemberSet) {
    folly::dynamic object = folly::dynamic::object;

    if (auto ipAhk =
            std::get_if<SaiIpNextHopTraits::AdapterHostKey>(&ahk.first)) {
      object[AttributeName<SaiIpNextHopTraits::Attributes::Type>::value] =
          folly::to<std::string>(SAI_NEXT_HOP_TYPE_IP);
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
      object[AttributeName<SaiIpNextHopTraits::Attributes::Type>::value] =
          folly::to<std::string>(SAI_NEXT_HOP_TYPE_MPLS);
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
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    else if (
        auto srv6Ahk = std::get_if<SaiSrv6SidlistNextHopTraits::AdapterHostKey>(
            &ahk.first)) {
      object[AttributeName<SaiIpNextHopTraits::Attributes::Type>::value] =
          folly::to<std::string>(SAI_NEXT_HOP_TYPE_SRV6_SIDLIST);
      object[AttributeName<
          SaiSrv6SidlistNextHopTraits::Attributes::RouterInterfaceId>::value] =
          folly::to<std::string>(
              std::get<
                  SaiSrv6SidlistNextHopTraits::Attributes::RouterInterfaceId>(
                  *srv6Ahk)
                  .value());
      object
          [AttributeName<SaiSrv6SidlistNextHopTraits::Attributes::Ip>::value] =
              std::get<SaiSrv6SidlistNextHopTraits::Attributes::Ip>(*srv6Ahk)
                  .value()
                  .str();
      object[AttributeName<
          SaiSrv6SidlistNextHopTraits::Attributes::TunnelId>::value] =
          folly::to<std::string>(
              std::get<SaiSrv6SidlistNextHopTraits::Attributes::TunnelId>(
                  *srv6Ahk)
                  .value());
      object[AttributeName<
          SaiSrv6SidlistNextHopTraits::Attributes::Srv6SidlistId>::value] =
          folly::to<std::string>(
              std::get<SaiSrv6SidlistNextHopTraits::Attributes::Srv6SidlistId>(
                  *srv6Ahk)
                  .value());
    }
#endif

    object[AttributeName<
        SaiNextHopGroupMemberTraits::Attributes::Weight>::value] = ahk.second;

    memberList.push_back(object);
  }
  json[AttributeName<
      SaiNextHopGroupTraits::Attributes::NextHopMemberList>::value] =
      memberList;
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  json[AttributeName<SaiArsTraits::Attributes::Mode>::value] =
      adapterHostKey_.mode;
#endif
  return json;
}

void follyDynamicToNhopSet(
    const folly::dynamic& json,
    SaiNextHopGroupTraits::AdapterHostKey& key) {
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
    if (object.find(
            AttributeName<
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
        key.nhopMemberSet.insert(std::make_pair(ipAhk, weight));
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
        key.nhopMemberSet.insert(std::make_pair(mplsAhk, weight));
      } break;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      case SAI_NEXT_HOP_TYPE_SRV6_SIDLIST: {
        SaiSrv6SidlistNextHopTraits::AdapterHostKey srv6Ahk;

        std::get<SaiSrv6SidlistNextHopTraits::Attributes::RouterInterfaceId>(
            srv6Ahk) =
            folly::to<sai_object_id_t>(
                object[AttributeName<SaiSrv6SidlistNextHopTraits::Attributes::
                                         RouterInterfaceId>::value]
                    .asString());
        std::get<SaiSrv6SidlistNextHopTraits::Attributes::Ip>(srv6Ahk) =
            folly::IPAddress(
                object[AttributeName<
                           SaiSrv6SidlistNextHopTraits::Attributes::Ip>::value]
                    .asString());
        std::get<SaiSrv6SidlistNextHopTraits::Attributes::TunnelId>(srv6Ahk) =
            folly::to<sai_object_id_t>(
                object[AttributeName<SaiSrv6SidlistNextHopTraits::Attributes::
                                         TunnelId>::value]
                    .asString());
        std::get<SaiSrv6SidlistNextHopTraits::Attributes::Srv6SidlistId>(
            srv6Ahk) =
            folly::to<sai_object_id_t>(
                object[AttributeName<SaiSrv6SidlistNextHopTraits::Attributes::
                                         Srv6SidlistId>::value]
                    .asString());
        key.nhopMemberSet.insert(std::make_pair(srv6Ahk, weight));
      } break;
#endif

      default:
        XLOG(FATAL) << "unsupported next hop type " << type;
    }
  }
}

template <>
typename SaiNextHopGroupTraits::AdapterHostKey
SaiObject<SaiNextHopGroupTraits>::follyDynamicToAdapterHostKey(
    const folly::dynamic& json) {
  SaiNextHopGroupTraits::AdapterHostKey key;
  // Pre D75845886 used an array as adapterHostKey value
  if (json.isObject()) {
    const auto& memberJson = json[AttributeName<
        SaiNextHopGroupTraits::Attributes::NextHopMemberList>::value];
    follyDynamicToNhopSet(memberJson, key);
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    key.mode =
        json[AttributeName<SaiArsTraits::Attributes::Mode>::value].asInt();
#endif
  } else if (json.isArray()) {
    follyDynamicToNhopSet(json, key);
  } else {
    throw FbossError("Unsupported value type for nhop-group AdapterHostKey");
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
SaiObject<SaiLagTraits>::follyDynamicToAdapterHostKey(
    const folly::dynamic& json) {
  std::string label = json.asString();
  SaiLagTraits::AdapterHostKey::ValueType key{};
  std::copy(std::begin(label), std::end(label), std::begin(key));
  return key;
}

#if defined(BRCM_SAI_SDK_XGS_AND_DNX)

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
  addOptionalAttrToArray<SaiWredTraits::Attributes::EcnGreenMarkProbability>(
      array, adapterHostKey_);
  return array;
}

template <typename attrT>
void pupulateOptionalAttrtToKey(
    const folly::dynamic& array,
    SaiWredTraits::AdapterHostKey& adapterHostKey,
    int index) {
  if (!array[index].isString()) {
    std::get<std::optional<attrT>>(adapterHostKey) = array[index].asInt();
  }
}

template <>
typename SaiWredTraits::AdapterHostKey
SaiObject<SaiWredTraits>::follyDynamicToAdapterHostKey(
    const folly::dynamic& json) {
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
  // Handle upgrade: if 8th element exists, populate EcnGreenMarkProbability.
  // Otherwise, default to 100 for warmboot from old format (7 elements) to new
  // format (8 elements)
  if (json.size() > 7) {
    pupulateOptionalAttrtToKey<
        SaiWredTraits::Attributes::EcnGreenMarkProbability>(json, key, 7);
  } else {
    // Default to 100 to be same as the SAI attribute default.
    std::get<std::optional<SaiWredTraits::Attributes::EcnGreenMarkProbability>>(
        key) = 100;
  }
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
    const folly::dynamic& json) {
  return json.asString();
}

template <>
folly::dynamic SaiObject<SaiUdfGroupTraits>::adapterHostKeyToFollyDynamic() {
  return adapterHostKey_;
}

template <>
typename SaiUdfGroupTraits::AdapterHostKey
SaiObject<SaiUdfGroupTraits>::follyDynamicToAdapterHostKey(
    const folly::dynamic& json) {
  return json.asString();
}

} // namespace fboss
} // namespace facebook
