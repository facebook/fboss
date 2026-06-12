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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace facebook::fboss {

struct CmdShowBgpInitializationEventsTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = ::facebook::neteng::fboss::bgp::thrift::
      map_bgp_thriftBgpInitializationEvent_i64_cpptemplate_stdunordered_map_895;
};

class CmdShowBgpInitializationEvents
    : public CmdHandler<
          CmdShowBgpInitializationEvents,
          CmdShowBgpInitializationEventsTraits> {
 public:
  using RetType = CmdShowBgpInitializationEventsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& events, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
