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

#include <folly/IPAddress.h>
#include "common/network/if/gen-cpp2/Address_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/protocol/static/CmdDeleteProtocolStatic.h"

namespace facebook::fboss {

/**
 * Template base class for static route prefix arguments (for delete command).
 * Handles both IPv4 and IPv6 routes with compile-time IP version checking.
 *
 * Parses command line arguments in the format:
 *   <prefix>
 *
 * For example (IPv4):
 *   10.0.0.0/8
 *
 * For example (IPv6):
 *   2001:db8::/32
 */
template <network::thrift::AddressType IPVersion>
class StaticRouteDeletePrefixArgBase
    : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */
  StaticRouteDeletePrefixArgBase( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v) {
    if (v.size() != 1) {
      throw std::invalid_argument(
          "Expected <prefix>, got " + std::to_string(v.size()) + " arguments");
    }

    // Parse and validate the prefix (CIDR network)
    try {
      auto [ip, prefixLen] = folly::IPAddress::createNetwork(v[0]);
      bool isCorrectVersion = (IPVersion == network::thrift::AddressType::V4)
          ? ip.isV4()
          : ip.isV6();
      if (!isCorrectVersion) {
        std::string expected =
            (IPVersion == network::thrift::AddressType::V4) ? "IPv4" : "IPv6";
        std::string got =
            (IPVersion == network::thrift::AddressType::V4) ? "IPv6" : "IPv4";
        std::string cmd = (IPVersion == network::thrift::AddressType::V4)
            ? "delete protocol static ipv6 route"
            : "delete protocol static ip route";
        throw std::invalid_argument(
            "Expected " + expected + " prefix, got " + got + ": " + v[0] +
            ". Use '" + cmd + "' for " + got + " routes");
      }
      prefix_ = v[0];
    } catch (const folly::IPAddressFormatException& e) {
      throw std::invalid_argument(e.what());
    }
  }

  const std::string& getPrefix() const {
    return prefix_;
  }

 private:
  std::string prefix_;
};

class StaticRouteDeleteIpPrefixArg
    : public StaticRouteDeletePrefixArgBase<network::thrift::AddressType::V4> {
 public:
  using StaticRouteDeletePrefixArgBase<
      network::thrift::AddressType::V4>::StaticRouteDeletePrefixArgBase;

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_STATIC_ROUTE_PREFIX;
};

class StaticRouteDeleteIpv6PrefixArg
    : public StaticRouteDeletePrefixArgBase<network::thrift::AddressType::V6> {
 public:
  using StaticRouteDeletePrefixArgBase<
      network::thrift::AddressType::V6>::StaticRouteDeletePrefixArgBase;

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_STATIC_ROUTE_PREFIX_V6;
};

// Template function to delete static route (shared between IPv4 and IPv6)
template <typename PrefixArgType>
std::string deleteStaticRouteImpl(const PrefixArgType& prefixArg);

struct CmdDeleteProtocolStaticIpRouteTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteProtocolStatic;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_STATIC_ROUTE_PREFIX;
  static constexpr network::thrift::AddressType ipVersion =
      network::thrift::AddressType::V4;
  using ObjectArgType = StaticRouteDeleteIpPrefixArg;
  using RetType = std::string;
};

class CmdDeleteProtocolStaticIpRoute
    : public CmdHandler<
          CmdDeleteProtocolStaticIpRoute,
          CmdDeleteProtocolStaticIpRouteTraits> {
 public:
  using ObjectArgType = CmdDeleteProtocolStaticIpRouteTraits::ObjectArgType;
  using RetType = CmdDeleteProtocolStaticIpRouteTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& prefixArg);

  void printOutput(const RetType& logMsg);
};

struct CmdDeleteProtocolStaticIpv6RouteTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteProtocolStatic;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_STATIC_ROUTE_PREFIX_V6;
  static constexpr network::thrift::AddressType ipVersion =
      network::thrift::AddressType::V6;
  using ObjectArgType = StaticRouteDeleteIpv6PrefixArg;
  using RetType = std::string;
};

class CmdDeleteProtocolStaticIpv6Route
    : public CmdHandler<
          CmdDeleteProtocolStaticIpv6Route,
          CmdDeleteProtocolStaticIpv6RouteTraits> {
 public:
  using ObjectArgType = CmdDeleteProtocolStaticIpv6RouteTraits::ObjectArgType;
  using RetType = CmdDeleteProtocolStaticIpv6RouteTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& prefixArg);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
