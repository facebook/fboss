/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;
using std::string;
using std::vector;

namespace facebook::fboss {

string oidAttr(
    const sai_attribute_t* attr_list,
    int i,
    sai_object_type_t object_type) {
  return to<string>(
      "sai_attributes[",
      i,
      "].value.oid = ",
      SaiTracer::getInstance()->getVariable(
          attr_list[i].value.oid, object_type));
}

void oidListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    std::vector<std::string>& attrLines,
    sai_object_type_t object_type) {
  // First make sure we have enough lists for use
  uint32_t listLimit = SaiTracer::getInstance()->checkListCount(
      listIndex + 1, sizeof(sai_object_id_t), attr_list[i].value.objlist.count);

  string prefix = to<string>("sai_attributes", "[", i, "].value.objlist.");
  attrLines.push_back(
      to<string>(prefix, "count = ", attr_list[i].value.objlist.count));
  attrLines.push_back(
      to<string>(prefix, "list = (sai_object_id_t*)(list_", listIndex, ")"));
  for (int j = 0; j < std::min(attr_list[i].value.objlist.count, listLimit);
       ++j) {
    attrLines.push_back(to<string>(
        prefix,
        "list[",
        j,
        "] = ",
        SaiTracer::getInstance()->getVariable(
            attr_list[i].value.objlist.list[j], object_type)));
  }
}

void aclEntryActionU8Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines) {
  string prefix = to<string>("sai_attributes", "[", i, "].value.aclaction.");
  attrLines.push_back(
      to<string>(prefix, "enable = ", attr_list[i].value.aclaction.enable));
  attrLines.push_back(to<string>(
      prefix, "parameter.u8 = ", attr_list[i].value.aclaction.parameter.u8));
}

void aclEntryFieldIpV6Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines) {
  // The underlying implementation of sai_ip6_t is uint8_t[16]
  for (int j = 0; j < 16; ++j) {
    attrLines.push_back(to<string>(
        "sai_attributes",
        "[",
        i,
        "].value.aclfield.data.ip6[",
        j,
        "] = ",
        attr_list[i].value.aclfield.data.ip6[j]));
  }

  for (int j = 0; j < 16; ++j) {
    attrLines.push_back(to<string>(
        "sai_attributes",
        "[",
        i,
        "].value.aclfield.mask.ip6[",
        j,
        "] = ",
        attr_list[i].value.aclfield.mask.ip6[j]));
  }
}

void aclEntryFieldU8Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines) {
  string prefix = to<string>("sai_attributes", "[", i, "].value.aclfield.");
  attrLines.push_back(
      to<string>(prefix, "enable = ", attr_list[i].value.aclfield.enable));
  attrLines.push_back(
      to<string>(prefix, "data.u8 = ", attr_list[i].value.aclfield.data.u8));
  attrLines.push_back(
      to<string>(prefix, "mask.u8 = ", attr_list[i].value.aclfield.mask.u8));
}

void aclEntryFieldU16Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines) {
  string prefix = to<string>("sai_attributes", "[", i, "].value.aclfield.");
  attrLines.push_back(
      to<string>(prefix, "enable = ", attr_list[i].value.aclfield.enable));
  attrLines.push_back(
      to<string>(prefix, "data.u16 = ", attr_list[i].value.aclfield.data.u16));
  attrLines.push_back(
      to<string>(prefix, "mask.u16 = ", attr_list[i].value.aclfield.mask.u16));
}

void aclEntryFieldU32Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines) {
  string prefix = to<string>("sai_attributes", "[", i, "].value.aclfield.");
  attrLines.push_back(
      to<string>(prefix, "enable = ", attr_list[i].value.aclfield.enable));
  attrLines.push_back(
      to<string>(prefix, "data.u32 = ", attr_list[i].value.aclfield.data.u32));
  attrLines.push_back(
      to<string>(prefix, "mask.u32 = ", attr_list[i].value.aclfield.mask.u32));
}

std::string boolAttr(const sai_attribute_t* attr_list, int i) {
  return to<string>(
      "sai_attributes[",
      i,
      "].value.booldata = ",
      attr_list[i].value.booldata ? "true" : "false");
}

string u8Attr(const sai_attribute_t* attr_list, int i) {
  return to<string>(
      "sai_attributes[", i, "].value.u8 = ", attr_list[i].value.u8);
}

string u16Attr(const sai_attribute_t* attr_list, int i) {
  return to<string>(
      "sai_attributes[", i, "].value.u16 = ", attr_list[i].value.u16);
}

string s32Attr(const sai_attribute_t* attr_list, int i) {
  return to<string>(
      "sai_attributes[", i, "].value.s32 = ", attr_list[i].value.s32);
}

string u32Attr(const sai_attribute_t* attr_list, int i) {
  return to<string>(
      "sai_attributes[", i, "].value.u32 = ", attr_list[i].value.u32);
}

void s32ListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    vector<string>& attrLines) {
  // First make sure we have enough lists for use
  uint32_t listLimit = SaiTracer::getInstance()->checkListCount(
      listIndex + 1, sizeof(sai_int32_t), attr_list[i].value.s32list.count);

  string prefix = to<string>("sai_attributes", "[", i, "].value.s32list.");
  attrLines.push_back(
      to<string>(prefix, "count = ", attr_list[i].value.s32list.count));
  attrLines.push_back(
      to<string>(prefix, "list = (sai_int32_t*)(list_", listIndex, ")"));
  for (int j = 0; j < std::min(attr_list[i].value.s32list.count, listLimit);
       ++j) {
    attrLines.push_back(to<string>(
        prefix, "list[", j, "] = ", attr_list[i].value.s32list.list[j]));
  }
}

} // namespace facebook::fboss
