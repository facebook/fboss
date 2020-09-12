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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"
#include "fboss/agent/hw/sai/api/Traits.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

#include <boost/functional/hash.hpp>
#include <fmt/ranges.h>

#include <type_traits>
#include <utility>
#include <vector>

extern "C" {
#include <sai.h>
}

/*
 * The SaiAttribute template class facilitates pleasant and safe interaction
 * with the ubiquitous sai_attribute_t.
 *
 * Consider, as an example, interacting with port admin state and port speed.
 * Suppose we would like to read, then set the admin state, and just read the
 * speed.
 * The C-style code will look like:
 *
 * sai_attribute_t state_attr;
 * state_attr.id = SAI_PORT_ATTR_ADMIN_STATE;
 * port_api->get_port_attribute(port_handle, 1, &state_attr);
 * bool admin_state = state_attr.value.booldata;
 * state_attr.value.booldata = true;
 * port_api->set_port_attribute(port_handle, &state_attr);
 * sai_attribute_t speed_attr;
 * speed_attr.id = SAI_PORT_ATTR_SPEED;
 * port_api->get_port_attribute(port_handle, 1, &speed_attr);
 * uint32_t speed = speed_attr.value.u32;
 *
 * this has a few flaws:
 * 1) The author must keep track of the return types associated with various
 * attributes. e.g., does admin state come back as a bool or enum, what
 * is the size of speed, etc..
 * 2) It is possible to simply type the wrong union member and accidentally
 * extract an invalid member.
 * 3) There's a bit of boiler plate to "type"
 *
 * With this library, the interaction looks like this:
 * // Defined once, ahead of time:
 * using PortAttrAdminState = SaiAttribute<sai_port_attr_t,
 *                                         SAI_PORT_ATTR_ADMIN_STATE, bool>;
 * using PortAttrSpeed = SaiAttribute<sai_port_attr_t,
 *                                    SAI_PORT_ATTR_SPEED, uint32_t>;
 * // the actual code:
 * PortAttrAdminState state_attr;
 * auto admin_state = port_api.get_attribute(state_attr, port_handle);
 * PortAttrAdminState state_attr2(true);
 * port_api.set_attribute(state_attr2, port_handle);
 * PortAttrSpeed speed_attr;
 * auto speed = port_api.get_attribute(speed_attr, port_handle);
 *
 * This no longer requires knowing anything about the internals of the
 * sai_attribute_t.value union, nor matching the types in the SAI spec
 * against its fields.
 *
 * The above example is the "easy" case where the data is a primitive.
 * Some of the union data types are structs, lists, or arrays. To model those
 * with a similar style class, we keep track of separate ValueType and DataType.
 * DataType represents the type of the underlying data inside the union, and
 * ValueType represents the "nicer" wrapped type a user would interact with.
 * For example, DataType = sai_mac_t, ValueType = folly::MacAddress.
 * This simply makes the ctor and value() a bit more complicated:
 * they need to fill out the ValueType from the DataType or vice-versa.
 * For now, the ValueType may be copied into the SaiAttribute, but we could
 * easily adopt a different ownership model.
 *
 * SaiAttribute depends on two categories of helper template functions: _extract
 * and _fill.
 *
 * The _extract functions return a reference into the SAI union. They
 * use SFINAE to compile only the proper function for a given DataType and
 * ValueType. e.g., if DataType is uint16_t, the only one that survives
 * instantiation is the specialization that accessess value.u16.
 *
 * The _fill functions are used when ValueType != DataType to fill out the
 * contents of the union from the user-friendly type or vice-versa.
 */

namespace {

/*
 * There are a few corner cases that make _extract a bit tricky:
 * 1) some of the union data types are identical. For example, sai_ip4_t and
 * sai_uint32_t are uint32_t. This makes specialization based just on the
 * SAI data type for the attribute impossible in those cases. We handle
 * this by using a different ValueType and a slightly more complicated set
 * of conditions for enable_if.
 * 2) sai_pointer_t is void *. This is currently only used by switch api
 * for setting up callback functions, so we plan to provide a special direct
 * interface for that, for now, rather than using this generic mechanism.
 */
#define DEFINE_extract(_type, _field)                                   \
  template <typename AttrT>                                             \
  typename std::enable_if<                                              \
      std::is_same<typename AttrT::ExtractSelectionType, _type>::value, \
      typename AttrT::DataType>::type&                                  \
  _extract(sai_attribute_t& sai_attribute) {                            \
    return sai_attribute.value._field;                                  \
  }                                                                     \
  template <typename AttrT>                                             \
  const typename std::enable_if<                                        \
      std::is_same<typename AttrT::ExtractSelectionType, _type>::value, \
      typename AttrT::DataType>::type&                                  \
  _extract(const sai_attribute_t& sai_attribute) {                      \
    return sai_attribute.value._field;                                  \
  }

using facebook::fboss::SaiObjectIdT;

DEFINE_extract(bool, booldata);
DEFINE_extract(char[32], chardata);
DEFINE_extract(sai_uint8_t, u8);
DEFINE_extract(sai_uint16_t, u16);
DEFINE_extract(sai_uint32_t, u32);
DEFINE_extract(sai_uint64_t, u64);
DEFINE_extract(sai_int8_t, s8);
DEFINE_extract(sai_int16_t, s16);
DEFINE_extract(sai_int32_t, s32);
DEFINE_extract(sai_int64_t, s64);
DEFINE_extract(sai_u32_range_t, u32range);
DEFINE_extract(sai_s32_range_t, s32range);
DEFINE_extract(folly::MacAddress, mac);
DEFINE_extract(folly::IPAddressV4, ip4);
DEFINE_extract(folly::IPAddressV6, ip6);
DEFINE_extract(folly::IPAddress, ipaddr);
DEFINE_extract(folly::CIDRNetwork, ipprefix);
DEFINE_extract(SaiObjectIdT, oid);
DEFINE_extract(std::vector<sai_object_id_t>, objlist);
DEFINE_extract(std::vector<sai_uint8_t>, u8list);
DEFINE_extract(std::vector<sai_uint16_t>, u16list);
DEFINE_extract(std::vector<sai_uint32_t>, u32list);
DEFINE_extract(std::vector<sai_int8_t>, s8list);
DEFINE_extract(std::vector<sai_int16_t>, s16list);
DEFINE_extract(std::vector<sai_int32_t>, s32list);
DEFINE_extract(std::vector<sai_qos_map_t>, qosmap);
DEFINE_extract(facebook::fboss::AclEntryFieldU8, aclfield);
DEFINE_extract(facebook::fboss::AclEntryFieldU16, aclfield);
DEFINE_extract(facebook::fboss::AclEntryFieldU32, aclfield);
DEFINE_extract(facebook::fboss::AclEntryFieldIpV6, aclfield);
DEFINE_extract(facebook::fboss::AclEntryFieldIpV4, aclfield);
DEFINE_extract(facebook::fboss::AclEntryFieldMac, aclfield);
DEFINE_extract(facebook::fboss::AclEntryFieldSaiObjectIdT, aclfield);
DEFINE_extract(facebook::fboss::AclEntryActionU8, aclaction);
DEFINE_extract(facebook::fboss::AclEntryActionU32, aclaction);
DEFINE_extract(facebook::fboss::AclEntryActionSaiObjectIdT, aclaction);
DEFINE_extract(facebook::fboss::AclEntryActionSaiObjectIdList, aclaction);

// TODO:
DEFINE_extract(sai_vlan_list_t, vlanlist);
DEFINE_extract(sai_map_list_t, maplist);
DEFINE_extract(sai_acl_capability_t, aclcapability);
DEFINE_extract(sai_acl_resource_list_t, aclresource);
DEFINE_extract(sai_tlv_list_t, tlvlist);
DEFINE_extract(sai_segment_list_t, segmentlist);
DEFINE_extract(sai_ip_address_list_t, ipaddrlist);

template <typename SrcT, typename DstT>
typename std::enable_if<std::is_same<SrcT, DstT>::value>::type _fill(
    const SrcT& src,
    DstT& dst) {}

template <typename SrcT, typename DstT>
typename std::enable_if<
    !std::is_same<SrcT, DstT>::value &&
    std::is_convertible<SrcT, DstT>::value>::type
_fill(const SrcT& src, DstT& dst) {
  dst = src;
}

inline void _fill(const folly::IPAddress& src, sai_ip_address_t& dst) {
  dst = facebook::fboss::toSaiIpAddress(src);
}

inline void _fill(const sai_ip_address_t& src, folly::IPAddress& dst) {
  dst = facebook::fboss::fromSaiIpAddress(src);
}

inline void _fill(const folly::IPAddressV4& src, sai_ip4_t& dst) {
  dst = facebook::fboss::toSaiIpAddress(src).addr.ip4;
}

inline void _fill(const sai_ip4_t& src, folly::IPAddressV4& dst) {
  dst = facebook::fboss::fromSaiIpAddress(src);
}

inline void _fill(const folly::MacAddress& src, sai_mac_t& dst) {
  facebook::fboss::toSaiMacAddress(src, dst);
}

inline void _fill(const sai_mac_t& src, folly::MacAddress& dst) {
  dst = facebook::fboss::fromSaiMacAddress(src);
}

template <typename SaiListT, typename T>
void _fill(std::vector<T>& src, SaiListT& dst) {
  static_assert(
      std::is_same<decltype(dst.list), T*>::value,
      "pointer in sai list doesn't match vector type");
  dst.count = src.size();
  dst.list = src.data();
}

template <typename SaiListT, typename T>
void _fill(const SaiListT& src, std::vector<T>& dst) {
  static_assert(
      std::is_same<decltype(src.list), T*>::value,
      "pointer in sai list doesn't match vector type");
  dst.resize(src.count);
  std::copy(src.list, src.list + src.count, std::begin(dst));
}

inline void _fill(
    const sai_acl_field_data_t& src,
    facebook::fboss::AclEntryFieldU8& dst) {
  dst.setDataAndMask(std::make_pair(src.data.u8, src.mask.u8));
}

inline void _fill(
    const facebook::fboss::AclEntryFieldU8& src,
    sai_acl_field_data_t& dst) {
  dst.enable = true;
  std::tie(dst.data.u8, dst.mask.u8) = src.getDataAndMask();
}

inline void _fill(
    const sai_acl_field_data_t& src,
    facebook::fboss::AclEntryFieldU16& dst) {
  dst.setDataAndMask(std::make_pair(src.data.u16, src.mask.u16));
}

inline void _fill(
    const facebook::fboss::AclEntryFieldU16& src,
    sai_acl_field_data_t& dst) {
  dst.enable = true;
  std::tie(dst.data.u16, dst.mask.u16) = src.getDataAndMask();
}

inline void _fill(
    const sai_acl_field_data_t& src,
    facebook::fboss::AclEntryFieldU32& dst) {
  dst.setDataAndMask(std::make_pair(src.data.u32, src.mask.u32));
}

inline void _fill(
    const facebook::fboss::AclEntryFieldU32& src,
    sai_acl_field_data_t& dst) {
  dst.enable = true;
  std::tie(dst.data.u32, dst.mask.u32) = src.getDataAndMask();
}

inline void _fill(
    const sai_acl_field_data_t& src,
    facebook::fboss::AclEntryFieldIpV6& dst) {
  dst.setDataAndMask(std::make_pair(
      facebook::fboss::fromSaiIpAddress(src.data.ip6),
      facebook::fboss::fromSaiIpAddress(src.mask.ip6)));
}

inline void _fill(
    const facebook::fboss::AclEntryFieldIpV6& src,
    sai_acl_field_data_t& dst) {
  dst.enable = true;
  facebook::fboss::toSaiIpAddressV6(src.getDataAndMask().first, &dst.data.ip6);
  facebook::fboss::toSaiIpAddressV6(src.getDataAndMask().second, &dst.mask.ip6);
}

inline void _fill(
    const sai_acl_field_data_t& src,
    facebook::fboss::AclEntryFieldIpV4& dst) {
  dst.setDataAndMask(std::make_pair(
      facebook::fboss::fromSaiIpAddress(src.data.ip4),
      facebook::fboss::fromSaiIpAddress(src.mask.ip4)));
}

inline void _fill(
    const facebook::fboss::AclEntryFieldIpV4& src,
    sai_acl_field_data_t& dst) {
  dst.enable = true;
  dst.data.ip4 =
      facebook::fboss::toSaiIpAddress(src.getDataAndMask().first).addr.ip4;
  dst.mask.ip4 =
      facebook::fboss::toSaiIpAddress(src.getDataAndMask().second).addr.ip4;
}

inline void _fill(
    const sai_acl_field_data_t& src,
    facebook::fboss::AclEntryFieldMac& dst) {
  dst.setDataAndMask(std::make_pair(
      facebook::fboss::fromSaiMacAddress(src.data.mac),
      facebook::fboss::fromSaiMacAddress(src.mask.mac)));
}

inline void _fill(
    const facebook::fboss::AclEntryFieldMac& src,
    sai_acl_field_data_t& dst) {
  dst.enable = true;
  facebook::fboss::toSaiMacAddress(src.getDataAndMask().first, dst.data.mac);
  facebook::fboss::toSaiMacAddress(src.getDataAndMask().second, dst.mask.mac);
}

inline void _fill(
    const sai_acl_field_data_t& src,
    facebook::fboss::AclEntryFieldSaiObjectIdT& dst) {
  /*
   * Mask is not needed for sai_object_id_t Acl Entry field.
   * Thus, there is no oid field in sai_acl_field_data_mask_t.
   * oid is u64, but unfortunately, u64 was not added to
   * sai_acl_field_data_mask_t till SAI 1.6, so use u32.
   * This will be ignored by the implementation anyway.
   */
  dst.setDataAndMask(std::make_pair(src.data.oid, src.mask.u32));
}

inline void _fill(
    const facebook::fboss::AclEntryFieldSaiObjectIdT& src,
    sai_acl_field_data_t& dst) {
  /*
   * Mask is not needed for sai_object_id_t Acl Entry field.
   * Thus, there is no oid field in sai_acl_field_data_mask_t.
   * oid is u64, but unfortunately, u64 was not added to
   * sai_acl_field_data_mask_t till SAI 1.6, so use u32.
   * This will be ignored by the implementation anyway.
   */
  dst.enable = true;
  std::tie(dst.data.oid, dst.mask.u32) = src.getDataAndMask();
}

inline void _fill(
    const sai_acl_action_data_t& src,
    facebook::fboss::AclEntryActionU8& dst) {
  dst.setData(src.parameter.u8);
}

inline void _fill(
    const facebook::fboss::AclEntryActionU8& src,
    sai_acl_action_data_t& dst) {
  dst.enable = true;
  dst.parameter.u8 = src.getData();
}

inline void _fill(
    const sai_acl_action_data_t& src,
    facebook::fboss::AclEntryActionU32& dst) {
  dst.setData(src.parameter.u32);
}

inline void _fill(
    const facebook::fboss::AclEntryActionU32& src,
    sai_acl_action_data_t& dst) {
  dst.enable = true;
  dst.parameter.u32 = src.getData();
}

inline void _fill(
    const sai_acl_action_data_t& src,
    facebook::fboss::AclEntryActionSaiObjectIdT& dst) {
  dst.setData(src.parameter.oid);
}

inline void _fill(
    const facebook::fboss::AclEntryActionSaiObjectIdT& src,
    sai_acl_action_data_t& dst) {
  dst.enable = true;
  dst.parameter.oid = src.getData();
}

inline void _fill(
    const sai_acl_action_data_t& src,
    facebook::fboss::AclEntryActionSaiObjectIdList& dst) {
  std::vector<sai_object_id_t> dstData(src.parameter.objlist.count);
  dstData.resize(src.parameter.objlist.count);
  std::copy(
      src.parameter.objlist.list,
      src.parameter.objlist.list + src.parameter.objlist.count,
      std::begin(dstData));

  dst.setData(dstData);
}

inline void _fill(
    const facebook::fboss::AclEntryActionSaiObjectIdList& src,
    sai_acl_action_data_t& dst) {
  dst.enable = true;
  dst.parameter.objlist.count = src.getData().size();
  dst.parameter.objlist.list = const_cast<sai_object_id_t*>(
      reinterpret_cast<const sai_object_id_t*>(src.getData().data()));
}

template <typename SrcT, typename DstT>
void _realloc(const SrcT& src, DstT& dst) {
  XLOG(FATAL) << "Unexpected call to fully generic _realloc";
}

template <typename SaiListT, typename T>
void _realloc(const SaiListT& src, std::vector<T>& dst) {
  static_assert(
      std::is_same<decltype(src.list), T*>::value,
      "pointer in sai list doesn't match vector type");
  dst.resize(src.count);
}

} // namespace

namespace facebook::fboss {

template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename DefaultGetterT = void,
    typename Enable = void>
class SaiAttribute;

template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename DefaultGetterT>
class SaiAttribute<
    AttrEnumT,
    AttrEnum,
    DataT,
    DefaultGetterT,
    typename std::enable_if<!IsSaiTypeWrapper<DataT>::value>::type> {
 public:
  using DataType = typename DuplicateTypeFixer<DataT>::value;
  using ValueType = DataType;
  using ExtractSelectionType = DataT;
  static constexpr AttrEnumT Id = AttrEnum;
  static constexpr bool HasDefaultGetter =
      !std::is_same_v<DefaultGetterT, void>;

  SaiAttribute() {
    saiAttr_.id = Id;
  }

  /* implicit */ SaiAttribute(const ValueType& value) : SaiAttribute() {
    data() = value;
  }

  /* implicit */ SaiAttribute(ValueType&& value) : SaiAttribute() {
    data() = std::move(value);
  }

  const ValueType& value() {
    return data();
  }

  static ValueType defaultValue() {
    static_assert(HasDefaultGetter, "No default getter provided for attribute");
    return DefaultGetterT{}();
  }

  ValueType value() const {
    return data();
  }

  sai_attribute_t* saiAttr() {
    return &saiAttr_;
  }

  const sai_attribute_t* saiAttr() const {
    return &saiAttr_;
  }

  // TODO(borisb): once we deprecate the old create() from SaiApi, we
  // can delete this
  std::vector<sai_attribute_t> saiAttrs() const {
    return {saiAttr_};
  }

  void realloc() {
    XLOG(FATAL) << "Unexpected realloc on SaiAttribute with primitive DataT";
  }

  bool operator==(const SaiAttribute& other) const {
    return other.value() == value();
  }

  bool operator!=(const SaiAttribute& other) const {
    return !(*this == other);
  }

  bool operator<(const SaiAttribute& other) const {
    return value() < other.value();
  }

 private:
  DataType& data() {
    return _extract<SaiAttribute>(saiAttr_);
  }
  const DataType& data() const {
    return _extract<SaiAttribute>(saiAttr_);
  }
  sai_attribute_t saiAttr_{};
};

template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename DefaultGetterT>
class SaiAttribute<
    AttrEnumT,
    AttrEnum,
    DataT,
    DefaultGetterT,
    typename std::enable_if<IsSaiTypeWrapper<DataT>::value>::type> {
 public:
  using DataType = typename WrappedSaiType<DataT>::value;
  using ValueType = DataT;
  using ExtractSelectionType = DataT;
  static constexpr AttrEnumT Id = AttrEnum;
  static constexpr bool HasDefaultGetter =
      !std::is_same_v<DefaultGetterT, void>;

  SaiAttribute() {
    saiAttr_.id = Id;
  }

  /* value_ may be (and in fact almost certainly is) some complicated
   * heap stored type like std::vector, std::string, etc... For this
   * reason, we need to implement a deep copying copy ctor and copy
   * assignment operator, so that destruction of the implicit copy
   * source doesn't invalidate the copy target.
   *
   * e.g.:
   * std::vector<uint32_t> v;
   * v.resize(10);
   * FooAttribute a2;
   * {
   * FooAttribute a1(v);
   * a2 = a1;
   * }
   * a2.value() // uh-oh
   */
  SaiAttribute(const SaiAttribute& other) {
    // NOTE: use copy assignment to implement copy ctor
    *this = other;
  }
  SaiAttribute(SaiAttribute&& other) {
    *this = std::move(other);
  }
  SaiAttribute& operator=(const SaiAttribute& other) {
    saiAttr_.id = other.saiAttr_.id;
    setValue(other.value());
    return *this;
  }
  SaiAttribute& operator=(SaiAttribute&& other) {
    saiAttr_.id = other.saiAttr_.id;
    setValue(std::move(other).value());
    return *this;
  }
  /*
   * Constructors that take a value type. Important for cases where
   * the value type must be initialized before a call to api.getAttribute()
   * e.g.
   * std::vector<uint32_t> v;
   * v.resize(10);
   * VecAttr a(v);
   * api.getAttribute(a); // uses v as storage for sai get_attribute call
   */
  /* implicit */ SaiAttribute(const ValueType& value) : SaiAttribute() {
    setValue(value);
  }
  /* implicit */ SaiAttribute(ValueType&& value) : SaiAttribute() {
    setValue(std::move(value));
  }

  const ValueType& value() {
    _fill(data(), value_);
    return value_;
  }

  ValueType value() const {
    ValueType v;
    _fill(data(), v);
    return v;
  }

  static ValueType defaultValue() {
    static_assert(HasDefaultGetter, "No default getter provided for attribute");
    static ValueType v;
    _fill(DefaultGetterT{}(), v);
    return v;
  }
  /*
   * Value based Attributes will often require caller allocation. For
   * example, we need to allocate an array for "list" attributes.
   * However, in some get cases, we need to make an initial get call
   * with an unallocated input, then do the actual allocation based on
   * what the Adapter puts in the sai_attribute_t. In those cases, the Api
   * class will call realloc() to trigger that reallocation.
   */
  void realloc() {
    _realloc(data(), value_);
    _fill(value_, data());
  }

  sai_attribute_t* saiAttr() {
    return &saiAttr_;
  }

  const sai_attribute_t* saiAttr() const {
    return &saiAttr_;
  }

  // TODO(borisb): once we deprecate the old create() from SaiApi, we
  // can delete this
  std::vector<sai_attribute_t> saiAttrs() const {
    return {saiAttr_};
  }

  bool operator==(const SaiAttribute& other) const {
    return other.value() == value();
  }

  bool operator!=(const SaiAttribute& other) const {
    return !(*this == other);
  }

  bool operator<(const SaiAttribute& other) const {
    return value() < other.value();
  }

 private:
  void setValue(const ValueType& value) {
    value_ = value;
    _fill(value_, data());
  }
  void setValue(ValueType&& value) {
    value_ = std::move(value);
    _fill(value_, data());
  }
  DataType& data() {
    return _extract<SaiAttribute>(saiAttr_);
  }
  const DataType& data() const {
    return _extract<SaiAttribute>(saiAttr_);
  }
  sai_attribute_t saiAttr_{};
  ValueType value_{};
};

template <typename T>
struct SaiExtensionAttributeId {
  std::optional<sai_attr_id_t> operator()() {
    static_assert(
        sizeof(T) == -1,
        "In order to define extension attribute, you must also define an attribute id policy");
    return std::nullopt;
  }
};

template <
    typename T,
    typename SaiExtensionAttributeId = SaiExtensionAttributeId<T>>
class SaiExtensionAttribute {
 public:
  using DataType = std::conditional_t<
      IsSaiTypeWrapper<T>::value,
      typename WrappedSaiType<T>::value,
      T>;
  using ValueType = T;
  static constexpr bool HasDefaultGetter = false;
  using AttributeId = SaiExtensionAttributeId;
  using ExtractSelectionType = T;

  SaiExtensionAttribute() {
    saiAttr_.id = kExtensionAttributeId();
  }

  SaiExtensionAttribute(const SaiExtensionAttribute& other)
      : SaiExtensionAttribute() {
    *this = other;
  }

  SaiExtensionAttribute(SaiExtensionAttribute&& other)
      : SaiExtensionAttribute() {
    *this = std::move(other);
  }

  SaiExtensionAttribute& operator=(const SaiExtensionAttribute& other) {
    CHECK_EQ(other.id(), kExtensionAttributeId());
    saiAttr_.id = other.id();
    setValue(other.value());
    return *this;
  }

  SaiExtensionAttribute& operator=(SaiExtensionAttribute&& other) {
    CHECK_EQ(other.id(), kExtensionAttributeId());
    saiAttr_ = std::move(other.saiAttr_);
    value_ = std::move(other.value_);
    return *this;
  }

  bool operator==(const SaiExtensionAttribute& other) const {
    return id() == other.id() && other.value() == value();
  }

  bool operator!=(const SaiExtensionAttribute& other) const {
    return !(*this == other);
  }

  /* implicit */ SaiExtensionAttribute(const ValueType& value)
      : SaiExtensionAttribute() {
    saiAttr_.id = kExtensionAttributeId();
    setValue(value);
  }

  /* implicit */ SaiExtensionAttribute(ValueType&& value)
      : SaiExtensionAttribute() {
    saiAttr_.id = kExtensionAttributeId();
    setValue(std::move(value));
  }

  const ValueType& value() {
    _fill(data(), value_);
    return value_;
  }

  ValueType value() const {
    ValueType v;
    _fill(data(), v);
    return v;
  }

  void realloc() {
    _realloc(data(), value_);
    _fill(value_, data());
  }

  sai_attribute_t* saiAttr() {
    return &saiAttr_;
  }

  const sai_attribute_t* saiAttr() const {
    return &saiAttr_;
  }

  sai_attr_id_t id() const {
    return saiAttr_.id;
  }

 private:
  void setValue(const ValueType& value) {
    value_ = value;
    _fill(value_, data());
  }

  void setValue(ValueType&& value) {
    value_ = std::move(value);
    _fill(value_, data());
  }

  DataType& data() {
    return _extract<SaiExtensionAttribute>(saiAttr_);
  }

  const DataType& data() const {
    return _extract<SaiExtensionAttribute>(saiAttr_);
  }

  static sai_attr_id_t kExtensionAttributeId() {
    auto id = SaiExtensionAttributeId()();
    CHECK(id.has_value()) << " unknown id";
    return id.value();
  }

  sai_attribute_t saiAttr_{};
  ValueType value_{};
};

template <typename T>
struct IsSaiExtensionAttribute<SaiExtensionAttribute<T>, void>
    : std::true_type {};

template <typename T>
struct IsSaiExtensionAttribute<
    T,
    std::enable_if_t<std::is_base_of_v<
        SaiExtensionAttribute<typename T::ValueType, typename T::AttributeId>,
        T>>> : std::true_type {};

// implement trait that detects SaiAttribute
template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename DefaultGetterT>
struct IsSaiAttribute<
    SaiAttribute<AttrEnumT, AttrEnum, DataT, DefaultGetterT, void>>
    : public std::true_type {};

template <typename T>
struct IsSaiAttribute<SaiExtensionAttribute<T>> : public std::true_type {};

template <typename T>
struct IsSaiAttribute<T, std::enable_if_t<IsSaiExtensionAttribute<T>::value>>
    : std::true_type {};

template <typename AttrT>
struct AttributeName {
  // N.B., we can't just use static_assert(false, msg) because the
  // compiler will trip the assert at declaration if it doesn't depend on the
  // type parameter
  static_assert(
      sizeof(AttrT) == -1,
      "In order to format a SaiAttribute, you must specialize "
      "AttributeName<AttrT>. Consider using the SAI_ATTRIBUTE_NAME "
      "macro below if you are adding an attribute to a SaiApi.");
  using value = void;
};

#define SAI_ATTRIBUTE_NAME(Obj, Attribute)                        \
  template <>                                                     \
  struct AttributeName<Sai##Obj##Traits::Attributes::Attribute> { \
    static const inline std::string value = #Attribute;           \
  };

} // namespace facebook::fboss

namespace std {
template <typename AttrEnumT, AttrEnumT AttrEnum, typename DataT>
struct hash<facebook::fboss::SaiAttribute<AttrEnumT, AttrEnum, DataT, void>> {
  size_t operator()(
      const facebook::fboss::SaiAttribute<AttrEnumT, AttrEnum, DataT, void>&
          attr) const {
    size_t seed = 0;
    boost::hash_combine(seed, attr.value());
    return seed;
  }
};

} // namespace std
