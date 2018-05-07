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
#pragma once

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

extern "C" {
#include <sx/sdk/sx_ip.h>
#include <sx/sdk/sx_mac.h>
}

namespace facebook { namespace fboss { namespace utils {

/**
 * This file contains some helper functions, e.g.
 * to convert IP, Mac from folly:: objects to SDK structures
 */

// folly to SDK structure functions

/**
 * Convert network prefix in @param fromPrefix to SDK sx_ip_prefix_t structure
 * Works both for IPv4 and IPv6
 *
 * @param[in] fromPrefix folly CIDRNetwork instance
 * @param[out] toPrefix Output SDK structure
 */
void follyIPPrefixToSdk(const folly::CIDRNetwork& fromPrefix,
  sx_ip_prefix_t* toPrefix);

/**
 * Convert network address in @param fromIp to SDK sx_ip_prefix_t structure
 * Mask is /32 for v4 address, /128 for v6 address
 * Works both for IPv4 and IPv6
 *
 * @param[in] fromIp folly ip address
 * @param[out] toPrefix Output SDK structure
 */
void follyIPPrefixToSdk(const folly::IPAddress& fromIp,
  sx_ip_prefix_t* toPrefix);

/**
 * Convert folly::IPAddress to SDK sx_ip_addr_t structure
 *
 * @param[in] fromIp Input IP address to convert
 * @param[out] toIp Output IP address SDK structure
 */
void follyIPAddressToSdk(const folly::IPAddress& fromIp,
  sx_ip_addr_t* toIp);

/**
 * Convert folly::MacAddress to SDK sx_mac_addr_t structure
 *
 * @param[in] fromMac Input mac address to convert
 * @param[out] toMac Output mac address SDK structure
 */
void follyMacAddressToSdk(const folly::MacAddress& fromMac,
  sx_mac_addr_t* toMac);

// SDK structures to folly objects

/**
 * Convert SDK prefix struct to folly::CIDRNetwork
 *
 * @param[in] fromPrefix Input sx_ip_prefix struct
 * @return Output CIDRNetwork object
 */
folly::CIDRNetwork sdkIpPrefixToFolly(const sx_ip_prefix_t& fromPrefix);

/**
 * Convert SDK ip address struct to folly::IPAddress
 *
 * @param[in] fromIp Input sx_ip_addr struct
 * @return Output IPAddress object
 */
folly::IPAddress sdkIpAddressToFolly(const sx_ip_addr_t& fromIp);

/**
 * Convert SDK mac address struct to folly::MacAddress
 *
 * @param[in] fromMac Input sx_mac_addr struct
 * @return Output MacAddress object
 */
folly::MacAddress sdkMacAddressToFolly(const sx_mac_addr_t& fromMac);

}}} // facebook::fboss::utils
