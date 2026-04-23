/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package "facebook.com/neteng/fboss/bgp/public_tld/configerator/structs/neteng/config/vip_service_config"

namespace cpp2 vipconfig.config
namespace py vipconfig.config.vip_service_config
namespace go vipconfig.config.vip_service_config
namespace py3 vipconfig.config

struct VipServiceConfig {
  /**
  * tcp port where vip_service will be accepting requests
  */
  1: i32 port = 3333;
  /**
  * enable acl checking on vip injection/withdrawal
  */
  2: bool enforce_acl = false;
  /**
  * killswitch file path
  * if file is present, acl checking is bypassed even if enabled
  */
  3: string acl_killswitch_file = "";
  /**
  * An injector must renew vips within ttl to prevent vips from expiring.
  * Expired vips will be withdrawn.
  * ttl is set by each injector
  * min_ttl_s indicates the lowerbound of a valid ttl
  */
  4: i32 min_ttl_s = 1;
  /**
  * upperbound of a valid ttl in vip_service
  */
  5: i32 max_ttl_s = 600;
  /**
  * upperbound of allowed thrift connections
  **/
  6: i32 max_thrift_connections = 200;
  /**
  * upperbound of allowed vips per injector
  **/
  7: i32 max_vips_per_injector = 1024;
  /**
  * upperbound of allowed vips per vipServer
  **/
  8: i32 max_vips_per_vipserver = 4000;

  /**
  * policy related fields
  */
  21: optional string ingress_policy;
}
