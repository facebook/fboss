// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/MirrorOnDropReport.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

using MirrorOnDropReportMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using MirrorOnDropReportMapThriftType =
    std::map<std::string, state::MirrorOnDropReportFields>;

class MirrorOnDropReportMap;
using MirrorOnDropReportMapTraits = ThriftMapNodeTraits<
    MirrorOnDropReportMap,
    MirrorOnDropReportMapTypeClass,
    MirrorOnDropReportMapThriftType,
    MirrorOnDropReport>;

class MirrorOnDropReportMap
    : public ThriftMapNode<MirrorOnDropReportMap, MirrorOnDropReportMapTraits> {
 public:
  using Traits = MirrorOnDropReportMapTraits;
  using BaseT =
      ThriftMapNode<MirrorOnDropReportMap, MirrorOnDropReportMapTraits>;
  MirrorOnDropReportMap() = default;
  virtual ~MirrorOnDropReportMap() = default;

  static std::shared_ptr<MirrorOnDropReportMap> fromThrift(
      const std::map<std::string, state::MirrorOnDropReportFields>&
          mirrorOnDropReports);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using MultiSwitchMirrorOnDropReportMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, MirrorOnDropReportMapTypeClass>;
using MultiSwitchMirrorOnDropReportMapThriftType =
    std::map<std::string, MirrorOnDropReportMapThriftType>;

class MultiSwitchMirrorOnDropReportMap;

using MultiSwitchMirrorOnDropReportMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchMirrorOnDropReportMap,
    MultiSwitchMirrorOnDropReportMapTypeClass,
    MultiSwitchMirrorOnDropReportMapThriftType,
    MirrorOnDropReportMap>;

class HwSwitchMatcher;

class MultiSwitchMirrorOnDropReportMap
    : public ThriftMultiSwitchMapNode<
          MultiSwitchMirrorOnDropReportMap,
          MultiSwitchMirrorOnDropReportMapTraits> {
 public:
  using Traits = MultiSwitchMirrorOnDropReportMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchMirrorOnDropReportMap,
      MultiSwitchMirrorOnDropReportMapTraits>;
  using BaseT::modify;

  MultiSwitchMirrorOnDropReportMap* modify(std::shared_ptr<SwitchState>* state);
  static std::shared_ptr<MultiSwitchMirrorOnDropReportMap> fromThrift(
      const std::map<
          std::string,
          std::map<std::string, state::MirrorOnDropReportFields>>&
          mnpuMirrorOnDropReports);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
