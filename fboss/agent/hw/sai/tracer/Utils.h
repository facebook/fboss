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

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

// Helper methods to setup attributes

// OidAttr not only serializes oid, but also look into the variable mappings
// to check if the oid matches any previously declared variables
std::string oidAttr(const sai_attribute_t* attr_list, int i);

void oidListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    std::vector<std::string>& attrLines);

void aclEntryActionSaiObjectIdAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryActionSaiObjectIdListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    std::vector<std::string>& attrLines);

void aclEntryActionU8Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryActionU32Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryFieldSaiObjectIdAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryFieldIpV4Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryFieldIpV6Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryFieldU8Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryFieldU16Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryFieldU32Attr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void aclEntryFieldMacAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

std::string boolAttr(const sai_attribute_t* attr_list, int i);

std::string s8Attr(const sai_attribute_t* attr_list, int i);

std::string u8Attr(const sai_attribute_t* attr_list, int i);

std::string u16Attr(const sai_attribute_t* attr_list, int i);

std::string s32Attr(const sai_attribute_t* attr_list, int i);

std::string u32Attr(const sai_attribute_t* attr_list, int i);

std::string u64Attr(const sai_attribute_t* attr_list, int i);

void s8ListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    std::vector<std::string>& attrLines,
    bool nullable = false);

void s32ListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    std::vector<std::string>& attrLines);

void u32ListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    std::vector<std::string>& attrLines);

void s32RangeAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void u32RangeAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void qosMapListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listIndex,
    std::vector<std::string>& attrLines);

void charDataAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void macAddressAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void ipAttr(
    const sai_attribute_t* attr_list,
    int i,
    std::vector<std::string>& attrLines);

void u8ArrGenericAttr(
    const uint8_t* array,
    uint arrLength,
    int i,
    std::vector<std::string>& attrLines,
    const std::string& attrName);

std::string attrNameToEnum(
    const std::string& objType,
    const std::string& attrName);

} // namespace facebook::fboss
