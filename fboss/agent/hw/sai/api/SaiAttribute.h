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

#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/api/SaiApiError.h"
#include "fboss/agent/hw/sai/api/Traits.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

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
#define DEFINE_extract(_type, _field)                     \
template <typename AttrT>                                 \
typename std::enable_if<                                  \
    std::is_same<typename AttrT::DataType, _type>::value, \
    typename AttrT::DataType>::type&                      \
_extract(sai_attribute_t& sai_attribute) {                \
  return sai_attribute.value._field;                      \
}                                                         \
template <typename AttrT>                                 \
const typename std::enable_if<                            \
    std::is_same<typename AttrT::DataType, _type>::value, \
    typename AttrT::DataType>::type&                      \
_extract(const sai_attribute_t& sai_attribute) {          \
  return sai_attribute.value._field;                      \
}

#define DEFINE_extract_wrapper_v(_type, _valueType, _field)       \
template <typename AttrT>                                         \
typename std::enable_if<                                          \
    std::is_same<typename AttrT::DataType, _type>::value &&       \
      std::is_same<typename AttrT::ValueType, _valueType>::value, \
    typename AttrT::DataType>::type&                              \
_extract(sai_attribute_t& sai_attribute) {                        \
  return sai_attribute.value._field;                              \
}                                                                 \
template <typename AttrT>                                         \
const typename std::enable_if<                                    \
    std::is_same<typename AttrT::DataType, _type>::value &&       \
      std::is_same<typename AttrT::ValueType, _valueType>::value, \
    typename AttrT::DataType>::type&                              \
_extract(const sai_attribute_t& sai_attribute) {                  \
  return sai_attribute.value._field;                              \
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
DEFINE_extract(sai_mac_t, mac);
DEFINE_extract_wrapper_v(sai_ip4_t, folly::IPAddressV4, ip4);
DEFINE_extract(sai_ip6_t, ip6);
DEFINE_extract_wrapper_v(sai_ip_address_t, folly::IPAddress, ipaddr);
DEFINE_extract(sai_ip_prefix_t, ipprefix);
DEFINE_extract_wrapper_v(sai_object_id_t, SaiObjectIdT, oid);
DEFINE_extract(sai_object_list_t, objlist);
DEFINE_extract(sai_u8_list_t, u8list);
DEFINE_extract(sai_u16_list_t, u16list);
DEFINE_extract(sai_u32_list_t, u32list);
DEFINE_extract(sai_s8_list_t, s8list);
DEFINE_extract(sai_s16_list_t, s16list);
DEFINE_extract(sai_s32_list_t, s32list);
DEFINE_extract(sai_u32_range_t, u32range);
DEFINE_extract(sai_s32_range_t, s32range);
DEFINE_extract(sai_vlan_list_t, vlanlist);
DEFINE_extract(sai_qos_map_list_t, qosmap);
DEFINE_extract(sai_map_list_t, maplist);
DEFINE_extract(sai_acl_field_data_t, aclfield);
DEFINE_extract(sai_acl_action_data_t, aclaction);
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

void _fill(const folly::IPAddress& src, sai_ip_address_t& dst) {
  dst = facebook::fboss::toSaiIpAddress(src);
}

void _fill(const sai_ip_address_t& src, folly::IPAddress& dst) {
  dst = facebook::fboss::fromSaiIpAddress(src);
}

void _fill(const folly::MacAddress& src, sai_mac_t& dst) {
  facebook::fboss::toSaiMacAddress(src, dst);
}

void _fill(const sai_mac_t& src, folly::MacAddress& dst) {
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
} // namespace

namespace facebook {
namespace fboss {
template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename ValueT = DataT,
    typename Enable = void>
class SaiAttribute;

template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename ValueT>
class SaiAttribute<
    AttrEnumT,
    AttrEnum,
    DataT,
    ValueT,
    typename std::enable_if<
        std::is_same<DataT, ValueT>::value ||
        isDuplicateValueType<ValueT>::value>::type> {
 public:
  using DataType = DataT;
  using ValueType = DataT;

  SaiAttribute() {
    saiAttr_.id = AttrEnum;
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

  ValueType value() const {
    return data();
  }

  sai_attribute_t* saiAttr() {
    return &saiAttr_;
  }

  const sai_attribute_t* saiAttr() const {
    return &saiAttr_;
  }

  std::vector<sai_attribute_t> saiAttrs() const {
    return {saiAttr_};
  }

  bool operator==(const SaiAttribute& other) const {
    return other.value() == value();
  }

  bool operator!=(const SaiAttribute& other) const {
    return !(*this == other);
  }

 private:
  DataType& data() {
    return _extract<SaiAttribute>(saiAttr_);
  }
  const DataType& data() const {
    return _extract<SaiAttribute>(saiAttr_);
  }
  sai_attribute_t saiAttr_;
};

template <
    typename AttrEnumT,
    AttrEnumT AttrEnum,
    typename DataT,
    typename ValueT>
class SaiAttribute<
    AttrEnumT,
    AttrEnum,
    DataT,
    ValueT,
    typename std::enable_if<
        !std::is_same<DataT, ValueT>::value &&
        !isDuplicateValueType<ValueT>::value>::type> {
 public:
  using DataType = DataT;
  using ValueType = ValueT;

  SaiAttribute() {
    saiAttr_.id = AttrEnum;
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
    setValue(other.value());
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

  sai_attribute_t* saiAttr() {
    return &saiAttr_;
  }

  const sai_attribute_t* saiAttr() const {
    return &saiAttr_;
  }

  std::vector<sai_attribute_t> saiAttrs() const {
    return {saiAttr_};
  }

  bool operator==(const SaiAttribute& other) const {
    return other.value() == value();
  }

  bool operator!=(const SaiAttribute& other) const {
    return !(*this == other);
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
  sai_attribute_t saiAttr_;
  ValueType value_;
};
} // namespace fboss
} // namespace facebook
