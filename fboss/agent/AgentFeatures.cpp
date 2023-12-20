// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AgentFeatures.h"

DEFINE_bool(dsf_4k, false, "Enable DSF Scale Test config");

DEFINE_bool(
    sai_user_defined_trap,
    false,
    "Flag to use user defined trap when programming ACL action to punt packets to cpu queue.");

DEFINE_bool(enable_acl_table_chain_group, false, "Allow ACL table chaining");
