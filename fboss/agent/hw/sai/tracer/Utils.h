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
std::string
oidAttr(const sai_attribute_t* attr_list, int i, sai_object_type_t object_type);

void oidListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listCount,
    std::vector<std::string>& attrLines,
    sai_object_type_t object_type);

void aclEntryActionU8Attr(
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

std::string boolAttr(const sai_attribute_t* attr_list, int i);

std::string u8Attr(const sai_attribute_t* attr_list, int i);

std::string s32Attr(const sai_attribute_t* attr_list, int i);

std::string u32Attr(const sai_attribute_t* attr_list, int i);

void s32ListAttr(
    const sai_attribute_t* attr_list,
    int i,
    uint32_t listCount,
    std::vector<std::string>& attrLines);

} // namespace facebook::fboss
