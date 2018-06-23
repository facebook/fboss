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

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <boost/serialization/strong_typedef.hpp>

extern "C" {
#include <opennsl/switch.h>
}

namespace facebook {
namespace fboss {

#define DECLARE_MODULE_CONTROL_STRONG_TYPE(new_type, primitive) \
  BOOST_STRONG_TYPEDEF(primitive, new_type);

DECLARE_MODULE_CONTROL_STRONG_TYPE(
    PreprocessingControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(SeedControl, opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    IPv4NonTcpUdpFieldSelectionControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    IPv6NonTcpUdpFieldSelectionControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    IPv4TcpUdpFieldSelectionControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    IPv6TcpUdpFieldSelectionControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    IPv4TcpUdpPortsEqualFieldSelectionControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    IPv6TcpUdpPortsEqualFieldSelectionControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    FirstOutputFunctionControl,
    opennsl_switch_control_t)
DECLARE_MODULE_CONTROL_STRONG_TYPE(
    SecondOutputFunctionControl,
    opennsl_switch_control_t)

class BcmSwitch;
class LoadBalancer;

class BcmRtag7Module {
 public:
  /* ModuleControl is an attempt to make up for a deficiency in Broadcom's RTAG7
   * API.
   *
   * Consider a natural API for programming the RTAG7 engine:
   *
   *   int opennsl_rtag7_control_set(int unit,
   *                                 char module,
   *                                 bcm_rtag7_feature_t feature,
   *                                 int setting);
   *
   * So, for example, to enable pre-processing on module 'A', we would have
   *
   *   int err = opennsl_rtag7_control_set(0,
   *                                      'A',
   *                                      opennslPreprocessing,
   *                                      TRUE);
   *
   * Unfortunately, the Broadcom API joins the second and third arguments into a
   * single compile-time constant. Taking the above example, it would actually
   * have to be expressed as
   *
   *   int err = opennsl_rtag7_control_set(0,
   *                                       enablePreprocessingOnModuleA,
   *                                       TRUE);
   *
   * To avoid littering the implementation of BcmRtag7Module implementation with
   * the following pattern:
   *
   *   if (module == 'A') {
   *     // use constant corresponding to module A
   *   } else if (module == 'B') {
   *     // use constant corresponding to module B
   *   else {
   *     // error
   *   }
   *
   * ModuleControl holds the constants (ie. the combination of the second and
   * third argument) needed to program a specific module.
   *
   * As an added benefit, Ive renamed the constants so they are easier to
   * understand.
   */
  struct ModuleControl {
    char module;

    PreprocessingControl enablePreprocessing;

    SeedControl setSeed;

    // The values for these constants are determined by those used in
    // opennsl_esw_switch_control_port_set().
    int selectFirstOutput;
    int selectSecondOutput;

    // Field selection for IPv4/IPv6 packets which are neither TCP nor UDP
    IPv4NonTcpUdpFieldSelectionControl ipv4NonTcpUdpFieldSelection;
    IPv6NonTcpUdpFieldSelectionControl ipv6NonTcpUdpFieldSelection;

    // Field selection for IPv4/IPv6 packets which are either TCP or UDP, but
    // whose transport ports are _unequal_ (ie. source port != destination port)
    IPv4TcpUdpFieldSelectionControl ipv4TcpUdpPortsUnequalFieldSelection;
    IPv6TcpUdpFieldSelectionControl ipv6TcpUdpPortsUnequalFieldSelection;

    // Field selection for IPv4/IPv6 packets which are either TCP or UDP, but
    // whose transport ports are equal (ie. source port == destination port).
    IPv4TcpUdpPortsEqualFieldSelectionControl
        ipv4TcpUdpPortsEqualFieldSelection;
    IPv6TcpUdpPortsEqualFieldSelectionControl
        ipv6TcpUdpPortsEqualFieldSelection;

    FirstOutputFunctionControl hashFunction1;
    SecondOutputFunctionControl hashFunction2;
  };
  static const ModuleControl kModuleAControl();
  static const ModuleControl kModuleBControl();

  BcmRtag7Module(
      ModuleControl moduleControl,
      opennsl_switch_control_t offset,
      const BcmSwitch* hw);
  ~BcmRtag7Module();

  void init(const std::shared_ptr<LoadBalancer>& /* loadBalancer */) {}
  void program(
      const std::shared_ptr<LoadBalancer>& /* oldLoadBalancer */,
      const std::shared_ptr<LoadBalancer>& /* newLoadBalancer */) {}

 private:
  ModuleControl moduleControl_;
  opennsl_switch_control_t offset_;
  const BcmSwitch* const hw_;
};

} // namespace fboss
} // namespace facebook
