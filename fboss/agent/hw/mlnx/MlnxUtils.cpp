/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "fboss/agent/hw/mlnx/MlnxUtils.h"
#include "fboss/agent/FbossError.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

namespace {

constexpr auto kV4Mask = 32;
constexpr auto kV6Mask = 128;

void follyIPV6ToHBOLongs(const folly::IPAddressV6& ip, uint32_t* dst) {
  uint32_t* bytes = (uint32_t*)ip.toByteArray().data();
  for (int i = 0; i < sizeof(uint32_t); ++i) {
    dst[i] = ntohl(bytes[i]);
  }
}

void ipv6FromSdkToInAddr(const sx_ip_v6_addr_t& fromIp, in6_addr* toIp) {
  for (int i = 0; i < sizeof(*toIp) / sizeof(uint32_t); ++i) {
    toIp->__in6_u.__u6_addr32[i] = htonl(fromIp.__in6_u.__u6_addr32[i]);
  }
}

}

namespace facebook { namespace fboss { namespace utils {

void follyIPPrefixToSdk(const folly::CIDRNetwork& fromPrefix,
  sx_ip_prefix_t* toPrefix) {
  const auto prefixAddress = fromPrefix.first;
  auto mask = fromPrefix.second;
  if (prefixAddress.isV4()) {
    toPrefix->version = SX_IP_VERSION_IPV4;
    auto maskIp = folly::IPAddressV4(folly::IPAddressV4::fetchMask(mask));
    // SDK requires host byte order
    auto addr = prefixAddress.asV4().toLongHBO();
    auto mask = maskIp.toLongHBO();
    toPrefix->prefix.ipv4.addr.s_addr = addr;
    toPrefix->prefix.ipv4.mask.s_addr = mask;
  } else if (prefixAddress.isV6()) {
    toPrefix->version = SX_IP_VERSION_IPV6;

    auto maskIp = folly::IPAddressV6(folly::IPAddressV6::fetchMask(mask));
    follyIPV6ToHBOLongs(prefixAddress.asV6(),
      toPrefix->prefix.ipv6.addr.__in6_u.__u6_addr32);
    follyIPV6ToHBOLongs(maskIp, toPrefix->prefix.ipv6.mask.__in6_u.__u6_addr32);
  } else {
    throw FbossError("Invalid IP version");
  }
}

void follyIPPrefixToSdk(const folly::IPAddress& fromIp,
  sx_ip_prefix_t* toPrefix) {
  folly::CIDRNetwork network {fromIp, fromIp.isV4() ? kV4Mask : kV6Mask};
  follyIPPrefixToSdk(network, toPrefix);
}

void follyIPAddressToSdk(const folly::IPAddress& fromIp,
  sx_ip_addr_t* toIp) {
  if (fromIp.isV4()) {
    toIp->version = SX_IP_VERSION_IPV4;
    // SDK requires host byte order
    auto addr = fromIp.asV4().toLongHBO();
    toIp->addr.ipv4.s_addr = addr;
  } else if (fromIp.isV6()) {
    toIp->version = SX_IP_VERSION_IPV6;
    follyIPV6ToHBOLongs(fromIp.asV6(), toIp->addr.ipv6.__in6_u.__u6_addr32);
  } else {
    throw FbossError("Invalid IP version");
  }
}

void follyMacAddressToSdk(const folly::MacAddress& fromMac,
  sx_mac_addr_t* toMac) {
  // convert MAC string to sx_mac_addr_t structure
  auto macAddressBytes = fromMac.bytes();
  std::memcpy(&toMac->ether_addr_octet, macAddressBytes, folly::MacAddress::SIZE);
}

folly::CIDRNetwork sdkIpPrefixToFolly(const sx_ip_prefix_t& fromPrefix) {
  folly::CIDRNetwork resultNetwork;
  if (fromPrefix.version == SX_IP_VERSION_IPV4) {
    in_addr v4Prefix, v4Mask;
    uint8_t cidrMask = 0;
    v4Prefix.s_addr = htonl(fromPrefix.prefix.ipv4.addr.s_addr);
    v4Mask.s_addr = htonl(fromPrefix.prefix.ipv4.mask.s_addr);
    while (v4Mask.s_addr) {
      v4Mask.s_addr <<= 1;
      ++cidrMask;
    }
    folly::IPAddress addr{v4Prefix};
    resultNetwork.first = addr;
    resultNetwork.second = cidrMask;
  } else if (fromPrefix.version == SX_IP_VERSION_IPV6) {
    in6_addr v6Prefix, v6Mask;
    uint8_t cidrMask = 0;
    ipv6FromSdkToInAddr(fromPrefix.prefix.ipv6.addr, &v6Prefix);
    ipv6FromSdkToInAddr(fromPrefix.prefix.ipv6.mask, &v6Mask);
    for (int i = 0; i < sizeof(v6Mask) / sizeof(uint32_t); ++i) {
      while (v6Mask.__in6_u.__u6_addr32[i]) {
        v6Mask.__in6_u.__u6_addr32[i] <<= 1;
        ++cidrMask;
      }
    }
    folly::IPAddress addr{v6Prefix};
    resultNetwork.first = addr;
    resultNetwork.second = cidrMask;
  } else {
    throw FbossError("Invalid IP version");
  }
  return resultNetwork;
}

folly::IPAddress sdkIpAddressToFolly(const sx_ip_addr_t& fromIp) {
  folly::IPAddress resultAddr;
  if (fromIp.version == SX_IP_VERSION_IPV4) {
    in_addr v4Addr;
    v4Addr.s_addr = htonl(fromIp.addr.ipv4.s_addr);
    resultAddr = v4Addr;
  } else if (fromIp.version == SX_IP_VERSION_IPV6) {
    in6_addr v6Addr;
    ipv6FromSdkToInAddr(fromIp.addr.ipv6, &v6Addr);
    resultAddr = v6Addr;
  } else {
    throw FbossError("Invalid IP version");
  }
  return resultAddr;
}

folly::MacAddress sdkMacAddressToFolly(const sx_mac_addr_t& fromMac) {
  const auto& resultMac = folly::MacAddress::fromHBO(SX_MAC_TO_U64(fromMac));
  return resultMac;
}

}}} // facebook::fboss:utils
