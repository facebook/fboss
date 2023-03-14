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

#include "fboss/cli/fboss2/CmdArgsLists.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/utils/AggregateUtils.h"
#include "fboss/cli/fboss2/utils/CmdCommonUtils.h"
#include "fboss/cli/fboss2/utils/FilterUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

#include <fmt/color.h>
#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <variant>

namespace facebook::fboss {

template <typename Cmd>
class resolve_arg_types {
  template <typename Front, typename Tuple>
  struct tuple_push_back {};

  // Unwrap tuple and create a new type with our new type at the end
  template <typename Front, typename... Ts>
  struct tuple_push_back<Front, std::tuple<Ts...>> {
    using type = std::tuple<Ts..., Front>;
  };

  // recurse up the linked list of classes
  using parent = resolve_arg_types<typename Cmd::Traits::ParentCmd>;
  using appended_type = typename tuple_push_back<
      typename Cmd::Traits::ObjectArgType,
      typename parent::filtered_type>::type;

  // For backward compatibility with old format
  template <typename T>
  using is_monostate = std::is_same<T, std::monostate>;
  // Implementation for newly added argument type class
  template <typename T>
  using is_nonetype = std::is_same<T, utils::NoneArgType>;

 public:
  // filter out std::monostate and utils::NoneArgType
  using filtered_type = std::conditional_t<
      !is_monostate<typename Cmd::Traits::ObjectArgType>::value &&
          !is_nonetype<typename Cmd::Traits::ObjectArgType>::value,
      appended_type,
      typename parent::filtered_type>;
  // Used at run time to determine which index args we need and which to get rid
  // of (i.e. which positions are std::monostate)
  using unfiltered_type = typename tuple_push_back<
      typename Cmd::Traits::ObjectArgType,
      typename parent::unfiltered_type>::type;
};

// Base case for recursion (when ParentCmd is void)
template <>
struct resolve_arg_types<void> {
  using filtered_type = std::tuple<>;
  using unfiltered_type = std::tuple<>;
};

struct BaseCommandTraits {
  // Only for top level commands, nested subcommands will override this
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  static constexpr bool ALLOW_FILTERING = false;
  static constexpr bool ALLOW_AGGREGATION = false;
  std::vector<utils::LocalOption> LocalOptions = {};
};

template <typename CmdTypeT, typename CmdTypeTraits>
class CmdHandler {
  static_assert(
      std::is_base_of_v<BaseCommandTraits, CmdTypeTraits>,
      "CmdTypeTraits needs to subclass BaseCommandTraits");

 public:
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      CmdTypeTraits::ObjectArgTypeId;
  using Traits = CmdTypeTraits;
  using ObjectArgType = typename CmdTypeTraits::ObjectArgType;
  using RetType = typename CmdTypeTraits::RetType;
  using ThriftPrimitiveType = apache::thrift::metadata::ThriftPrimitiveType;

  void run();
  void runHelper();

  bool isFilterable();
  bool isAggregatable();

  std::unordered_map<std::string, std::vector<std::string>>
  getAcceptedFilterValues() {
    return {};
  }

  const ValidFilterMapType getValidFilters();
  const ValidAggMapType getValidAggs();

 protected:
  CmdTypeT& impl() {
    return static_cast<CmdTypeT&>(*this);
  }
  const CmdTypeT& impl() const {
    return static_cast<const CmdTypeT&>(*this);
  }
  std::vector<std::string> getHosts() {
    if (!CmdGlobalOptions::getInstance()->getHosts().empty()) {
      return CmdGlobalOptions::getInstance()->getHosts();
    }
    if (!CmdGlobalOptions::getInstance()->getSmc().empty()) {
      return utils::getHostsInSmcTier(
          CmdGlobalOptions::getInstance()->getSmc());
    }
    if (!CmdGlobalOptions::getInstance()->getFile().empty()) {
      return utils::getHostsFromFile(
          CmdGlobalOptions::getInstance()->getFile());
    }
    // if host is not specified, default to localhost
    return {"localhost"};
  }

 private:
  RetType queryClientHelper(const HostInfo& hostInfo) {
    using ArgTypes = resolve_arg_types<CmdTypeT>;
    auto tupleArgs = CmdArgsLists::getInstance()
                         ->getTypedArgs<
                             typename ArgTypes::unfiltered_type,
                             typename ArgTypes::filtered_type>();
    return std::apply(
        [&](auto const&... t) { return impl().queryClient(hostInfo, t...); },
        tupleArgs);
  }

  std::tuple<std::string, RetType, std::string> asyncHandler(
      const std::string& host,
      const CmdGlobalOptions::UnionList& parsedFilters,
      const ValidFilterMapType& validFilterMap) {
    auto hostInfo = HostInfo(host);
    XLOG(DBG2) << "host: " << host << " ip: " << hostInfo.getIpStr();

    std::string errStr;
    RetType result;
    try {
      result = queryClientHelper(hostInfo);
    } catch (std::exception const& err) {
      errStr = folly::to<std::string>("Thrift call failed: '", err.what(), "'");
    }
    if (!parsedFilters.empty()) {
      result = filterOutput<CmdTypeT>(result, parsedFilters, validFilterMap);
    }

    return std::make_tuple(host, result, errStr);
  }
};

} // namespace facebook::fboss
