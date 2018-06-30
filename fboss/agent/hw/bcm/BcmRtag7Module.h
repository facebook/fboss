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
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/types.h"

#include <folly/Range.h>
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

/* BcmRtag7Module owns a single module in the RTAG7 load-balancing engine.
 *
 * A BcmRtag7Module object is responsible for configuring the module it owns so
 * as to faithfully implement a given LoadBalancer in hardware.
 *
 * The RTAG7 module owned by an BcmRtag7Module object is dictated by
 * its ModuleControl member variable.
 */
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

  using ModuleState = boost::container::flat_map<opennsl_switch_control_t, int>;
  using ModuleStateRange = folly::Range<ModuleState::iterator>;
  using ModuleStateConstRange = folly::Range<ModuleState::const_iterator>;

  // getUnitControl is a wrapper around opennsl_switch_control_get(...). Its
  // only use is in BcmWarmBootCache.cpp, where it is used to retrieve settings
  // related to RTAG7. Because at that callsite there is no instance of
  // BcmRtag7Module, it is a static method on BcmRtag7Module.
  static int getUnitControl(int unit, opennsl_switch_control_t type);
  static ModuleState retrieveRtag7ModuleState(int unit, ModuleControl control);

  BcmRtag7Module(
      ModuleControl moduleControl,
      opennsl_switch_control_t offset,
      const BcmSwitch* hw);
  ~BcmRtag7Module();

  void init(const std::shared_ptr<LoadBalancer>& loadBalancer);
  void program(
      const std::shared_ptr<LoadBalancer>& oldLoadBalancer,
      const std::shared_ptr<LoadBalancer>& newLoadBalancer);

 private:
  void programPreprocessing(bool enable);
  void programAlgorithm(cfg::HashingAlgorithm algorithm);
  void programOutputSelection();
  void programFieldSelection(
      LoadBalancer::IPv4FieldsRange v4FieldsRange,
      LoadBalancer::IPv6FieldsRange v6FieldsRange,
      LoadBalancer::TransportFieldsRange transportFieldsRange);
  void programSeed(uint32_t seed);
  void programMacroFlowHashing(bool enable);
  void enableRtag7(LoadBalancerID);
  void programIPv4FieldSelection(
      LoadBalancer::IPv4FieldsRange v4FieldsRange,
      LoadBalancer::TransportFieldsRange transportFieldsRange);
  void programIPv6FieldSelection(
      LoadBalancer::IPv6FieldsRange v6FieldsRange,
      LoadBalancer::TransportFieldsRange transportFieldsRange);
  void programFieldControl();

  int computeIPv4Subfields(LoadBalancer::IPv4FieldsRange v4FieldsRange) const;
  int computeIPv6Subfields(LoadBalancer::IPv6FieldsRange v6FieldsRange) const;
  int computeTransportSubfields(
      LoadBalancer::TransportFieldsRange transportFieldsRange) const;

  void enableFlowLabelSelection();
  int getFlowLabelSubfields() const;

  // This is a small warpper around opennsl_switch_control_set(unit, ...) that
  // defaults the unit to HwSwitch::getUnit()
  template <typename ModuleControlType>
  int setUnitControl(ModuleControlType controlType, int arg);

  ModuleControl moduleControl_;
  opennsl_switch_control_t offset_;
  const BcmSwitch* const hw_;

  static bool fieldControlProgrammed_;
};

} // namespace fboss
} // namespace facebook
