/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_acl_api_t* wrappedAclApi();

SET_ATTRIBUTE_FUNC_DECLARATION(AclCounter);
SET_ATTRIBUTE_FUNC_DECLARATION(AclEntry);
SET_ATTRIBUTE_FUNC_DECLARATION(AclTable);
SET_ATTRIBUTE_FUNC_DECLARATION(AclTableGroup);
SET_ATTRIBUTE_FUNC_DECLARATION(AclTableGroupMember);

} // namespace facebook::fboss
