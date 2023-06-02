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

#include "fboss/agent/EnumUtils.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/mka_service/if/gen-cpp2/mka_structs_types.h"

#include <boost/container/flat_map.hpp>
#include <map>
#include <string>
#include <vector>

namespace facebook::fboss {

class SwitchState;

struct PortFields {
  struct VlanInfo {
    explicit VlanInfo(bool emitTags) : tagged(emitTags) {}
    bool operator==(const VlanInfo& other) const {
      return tagged == other.tagged;
    }
    bool operator!=(const VlanInfo& other) const {
      return !(*this == other);
    }
    state::VlanInfo toThrift() const;
    static VlanInfo fromThrift(state::VlanInfo const&);
    bool tagged;
  };

  using VlanMembership = boost::container::flat_map<VlanID, VlanInfo>;
  using LLDPValidations = std::map<cfg::LLDPTag, std::string>;
  using NeighborReachability = std::vector<cfg::PortNeighbor>;

  enum class OperState {
    DOWN = 0,
    UP = 1,
  };

  struct MKASakKey {
    bool operator<(const MKASakKey& r) const {
      return std::tie(*sci.macAddress(), *sci.port(), associationNum) <
          std::tie(*r.sci.macAddress(), *r.sci.port(), r.associationNum);
    }
    bool operator==(const MKASakKey& r) const {
      return std::tie(*sci.macAddress(), *sci.port(), associationNum) ==
          std::tie(*r.sci.macAddress(), *r.sci.port(), r.associationNum);
    }
    mka::MKASci sci;
    int associationNum;
  };
  using RxSaks = std::map<MKASakKey, mka::MKASak>;

  static state::RxSak rxSakToThrift(
      const MKASakKey& sakKey,
      const mka::MKASak& sak);
  static std::pair<MKASakKey, mka::MKASak> rxSakFromThrift(state::RxSak rxSak);
};

USE_THRIFT_COW(Port);

/*
 * Port stores state about one of the physical ports on the switch.
 */
class Port : public ThriftStructNode<Port, state::PortFields> {
 public:
  using BaseT = ThriftStructNode<Port, state::PortFields>;
  using BaseT::modify;
  using LegacyFields = PortFields;
  using VlanInfo = PortFields::VlanInfo;
  using VlanMembership = PortFields::VlanMembership;
  using OperState = PortFields::OperState;
  using LLDPValidations = PortFields::LLDPValidations;
  using NeighborReachability = PortFields::NeighborReachability;
  using MKASakKey = PortFields::MKASakKey;
  using RxSaks = PortFields::RxSaks;
  using PfcPriorityList = thrift_cow::detail::ReferenceWrapper<
      const std::shared_ptr<thrift_cow::ThriftListNode<
          apache::thrift::type_class::list<
              apache::thrift::type_class::integral>,
          std::vector<short>>>>;

  Port(PortID id, const std::string& name);

  PortID getID() const {
    return PortID(cref<switch_state_tags::portId>()->cref());
  }

  const std::string& getName() const {
    return cref<switch_state_tags::portName>()->cref();
  }

  void setName(const std::string& name) {
    set<switch_state_tags::portName>(name);
  }

  const std::string& getDescription() const {
    return cref<switch_state_tags::portDescription>()->cref();
  }

  void setDescription(const std::string& description) {
    set<switch_state_tags::portDescription>(description);
  }

  phy::PortPrbsState getAsicPrbs() const {
    // THRIFT_COPY
    return cref<switch_state_tags::asicPrbs>()->toThrift();
  }

  void setAsicPrbs(phy::PortPrbsState prbsState) {
    set<switch_state_tags::asicPrbs>(prbsState);
  }

  phy::PortPrbsState getGbSystemPrbs() const {
    // THRIFT_COPY
    return cref<switch_state_tags::gbSystemPrbs>()->toThrift();
  }

  void setGbSystemPrbs(phy::PortPrbsState prbsState) {
    set<switch_state_tags::gbSystemPrbs>(prbsState);
  }

  phy::PortPrbsState getGbLinePrbs() const {
    // THRIFT_COPY
    return cref<switch_state_tags::gbLinePrbs>()->toThrift();
  }

  void setGbLinePrbs(phy::PortPrbsState prbsState) {
    set<switch_state_tags::gbLinePrbs>(prbsState);
  }

  cfg::PortState getAdminState() const {
    return nameToEnum<cfg::PortState>(
        cref<switch_state_tags::portState>()->cref());
  }

  cfg::PortType getPortType() const {
    return cref<switch_state_tags::portType>()->cref();
  }
  void setPortType(cfg::PortType portType) {
    set<switch_state_tags::portType>(portType);
  }

  cfg::PortDrainState getPortDrainState() const {
    return cref<switch_state_tags::drainState>()->cref();
  }
  void setPortDrainState(cfg::PortDrainState drainState) {
    set<switch_state_tags::drainState>(drainState);
  }

  void setAdminState(cfg::PortState adminState) {
    set<switch_state_tags::portState>(enumToName(adminState));
  }

  std::optional<phy::LinkFaultStatus> getIPhyLinkFaultStatus() const {
    if (auto iPhyLinkFaultStatus =
            cref<switch_state_tags::iPhyLinkFaultStatus>()) {
      // THRIFT_COPY
      return iPhyLinkFaultStatus->toThrift();
    }
    return std::nullopt;
  }

  void setIPhyLinkFaultStatus(std::optional<phy::LinkFaultStatus> faultStatus) {
    if (!faultStatus) {
      ref<switch_state_tags::iPhyLinkFaultStatus>().reset();
      return;
    }
    set<switch_state_tags::iPhyLinkFaultStatus>(faultStatus.value());
  }

  OperState getOperState() const {
    return cref<switch_state_tags::portOperState>()->cref() ? OperState::UP
                                                            : OperState::DOWN;
  }

  void setOperState(bool isUp) {
    set<switch_state_tags::portOperState>(isUp);
  }

  bool isEnabled() const {
    return getAdminState() == cfg::PortState::ENABLED;
  }

  bool isDrained() const {
    return getPortDrainState() == cfg::PortDrainState::DRAINED;
  }

  bool isUp() const {
    return cref<switch_state_tags::portOperState>()->cref();
  }

  std::optional<mka::MKASak> getTxSak() const {
    if (auto txSak = cref<switch_state_tags::txSecureAssociationKey>()) {
      // THRIFT_COPY
      return txSak->toThrift();
    }
    return std::nullopt;
  }
  void setTxSak(std::optional<mka::MKASak> txSak) {
    if (!txSak) {
      ref<switch_state_tags::txSecureAssociationKey>().reset();
      return;
    }
    set<switch_state_tags::txSecureAssociationKey>(txSak.value());
  }
  // THRIFT_COPY
  RxSaks getRxSaksMap() const {
    RxSaks rxSecureAssociationKeys;
    for (auto rxSak :
         *(safe_cref<switch_state_tags::rxSecureAssociationKeys>())) {
      rxSecureAssociationKeys.emplace(
          PortFields::rxSakFromThrift(rxSak->toThrift()));
    }
    return rxSecureAssociationKeys;
  }
  void setRxSaksMap(const RxSaks& rxSecureAssociationKeys) {
    std::vector<state::RxSak> rxSaks{};
    for (const auto& [mkaSakKey, mkaSak] : rxSecureAssociationKeys) {
      rxSaks.push_back(PortFields::rxSakToThrift(mkaSakKey, mkaSak));
    }
    setRxSaks(rxSaks);
  }

  bool getMacsecDesired() const {
    return cref<switch_state_tags::macsecDesired>()->cref();
  }
  void setMacsecDesired(bool macsecDesired) {
    set<switch_state_tags::macsecDesired>(macsecDesired);
  }

  bool getDropUnencrypted() const {
    return cref<switch_state_tags::dropUnencrypted>()->cref();
  }
  void setDropUnencrypted(bool dropUnencrypted) {
    set<switch_state_tags::dropUnencrypted>(dropUnencrypted);
  }

  /**
   * Tells you Oper+Admin state of port. Will be UP only if both admin and
   * oper state is UP.
   */
  bool isPortUp() const {
    return isEnabled() && isUp();
  }

  VlanMembership getVlans() const {
    // TODO(zecheng): vlanMembership are stored as map<string, VlanInfo>, which
    // is not ideal since we should use vlanId.
    // Keeping the interface same for now, until vlanMembership is moved to
    // map<VlanId, VlanInfo>.
    VlanMembership vlanMembership;
    // THRIFT_COPY
    for (const auto& [vlanString, vlanInfo] :
         cref<switch_state_tags::vlanMemberShips>()->toThrift()) {
      vlanMembership.emplace(
          VlanID(folly::to<uint32_t>(vlanString)),
          VlanInfo::fromThrift(vlanInfo));
    }
    return vlanMembership;
  }
  void setVlans(const VlanMembership& vlans) {
    std::map<std::string, state::VlanInfo> vlanMembership;
    for (const auto& [vlanId, vlanInfo] : vlans) {
      vlanMembership.emplace(
          folly::to<std::string>(vlanId), vlanInfo.toThrift());
    }
    set<switch_state_tags::vlanMemberShips>(vlanMembership);
  }

  void addVlan(VlanID id, bool tagged) {
    auto vlanMembership = getVlans();
    vlanMembership.emplace(std::make_pair(id, VlanInfo(tagged)));
    setVlans(vlanMembership);
  }

  auto getPortQueues() const {
    return safe_cref<switch_state_tags::queues>();
  }
  void resetPortQueues(QueueConfig& queues) {
    // TODO(zecheng): change type to ThriftListNode
    std::vector<PortQueueFields> queuesThrift{};
    for (auto queue : queues) {
      queuesThrift.push_back(queue->toThrift());
    }
    set<switch_state_tags::queues>(std::move(queuesThrift));
  }

  bool hasValidPortQueues() const {
    constexpr auto kDefaultProbability = 100;
    for (const auto& portQueue : *getPortQueues()) {
      const auto& aqms = portQueue->get<ctrl_if_tags::aqms>();
      if (!aqms) {
        continue;
      }
      for (const auto& entry : std::as_const(*aqms)) {
        // THRIFT_COPY
        auto behavior = entry->cref<switch_config_tags::behavior>()->toThrift();
        auto detection =
            entry->cref<switch_config_tags::detection>()->toThrift();
        if (behavior == facebook::fboss::cfg::QueueCongestionBehavior::ECN &&
            detection.linear_ref()->probability() != kDefaultProbability) {
          return false;
        }
      }
    }
    return true;
  }

  auto getPortPgConfigs() const {
    return safe_cref<switch_state_tags::pgConfigs>();
  }

  void resetPgConfigs(std::optional<PortPgConfigs>& pgConfigs) {
    if (!pgConfigs) {
      safe_ref<switch_state_tags::pgConfigs>().reset();
      return;
    }
    std::vector<state::PortPgFields> pgConfigThrift{};
    for (auto pgConfig : pgConfigs.value()) {
      pgConfigThrift.push_back(pgConfig->toThrift());
    }
    set<switch_state_tags::pgConfigs>(std::move(pgConfigThrift));
  }

  void resetPgConfigs(std::vector<state::PortPgFields> pgConfigs) {
    set<switch_state_tags::pgConfigs>(pgConfigs);
  }

  VlanID getIngressVlan() const {
    return VlanID(cref<switch_config_tags::ingressVlan>()->cref());
  }
  void setIngressVlan(VlanID id) {
    set<switch_config_tags::ingressVlan>(id);
  }

  cfg::PortSpeed getSpeed() const {
    // TODO(zecheng): change storage type when ready
    return nameToEnum<cfg::PortSpeed>(
        cref<switch_state_tags::portSpeed>()->cref());
  }
  void setSpeed(cfg::PortSpeed speed) {
    // TODO(zecheng): change storage type when ready
    set<switch_state_tags::portSpeed>(enumToName(speed));
  }

  cfg::PortProfileID getProfileID() const {
    // TODO(zecheng): change storage type when ready
    return nameToEnum<cfg::PortProfileID>(
        cref<switch_state_tags::portProfileID>()->cref());
  }
  void setProfileId(cfg::PortProfileID profileID) {
    // TODO(zecheng): change storage type when ready
    set<switch_state_tags::portProfileID>(enumToName(profileID));
  }

  cfg::PortPause getPause() const {
    cfg::PortPause portPause;
    portPause.tx() = cref<switch_state_tags::txPause>()->cref();
    portPause.rx() = cref<switch_state_tags::rxPause>()->cref();
    return portPause;
  }
  void setPause(cfg::PortPause pause) {
    set<switch_state_tags::txPause>(*pause.tx());
    set<switch_state_tags::rxPause>(*pause.rx());
  }
  std::optional<cfg::PortPfc> getPfc() const {
    if (auto pfc = cref<switch_state_tags::pfc>()) {
      // THRIFT_COPY
      return pfc->toThrift();
    }
    return std::nullopt;
  }
  void setPfc(std::optional<cfg::PortPfc>& pfc) {
    if (!pfc) {
      ref<switch_state_tags::pfc>().reset();
      return;
    }
    set<switch_state_tags::pfc>(pfc.value());
  }

  std::vector<PfcPriority> getPfcPriorities() const {
    std::vector<PfcPriority> pfcPriorities{};
    if (safe_cref<switch_state_tags::pfcPriorities>()) {
      for (auto pri : *safe_cref<switch_state_tags::pfcPriorities>()) {
        pfcPriorities.emplace_back(static_cast<PfcPriority>(pri->cref()));
      }
    }
    return pfcPriorities;
  }
  void setPfcPriorities(std::optional<std::vector<int16_t>>& pri) {
    if (!pri) {
      ref<switch_state_tags::pfcPriorities>().reset();
      return;
    }
    set<switch_state_tags::pfcPriorities>(pri.value());
  }

  int32_t getMaxFrameSize() const {
    return cref<switch_state_tags::maxFrameSize>()->cref();
  }
  void setMaxFrameSize(int32_t maxFrameSize) {
    set<switch_state_tags::maxFrameSize>(maxFrameSize);
  }

  cfg::PortLoopbackMode getLoopbackMode() const {
    // TODO(zecheng): change storage type when ready
    return nameToEnum<cfg::PortLoopbackMode>(
        cref<switch_state_tags::portLoopbackMode>()->cref());
  }
  void setLoopbackMode(cfg::PortLoopbackMode loopbackMode) {
    // TODO(zecheng): change storage type when ready
    set<switch_state_tags::portLoopbackMode>(enumToName(loopbackMode));
  }

  int64_t getSflowIngressRate() const {
    return cref<switch_state_tags::sFlowIngressRate>()->cref();
  }
  void setSflowIngressRate(int64_t ingressRate) {
    set<switch_state_tags::sFlowIngressRate>(ingressRate);
  }

  int64_t getSflowEgressRate() const {
    return cref<switch_state_tags::sFlowEgressRate>()->cref();
  }
  void setSflowEgressRate(int64_t egressRate) {
    set<switch_state_tags::sFlowEgressRate>(egressRate);
  }

  std::optional<cfg::SampleDestination> getSampleDestination() const {
    // TODO(zecheng): change storage type when ready
    if (auto sampleDestination = cref<switch_state_tags::sampleDest>()) {
      return nameToEnum<cfg::SampleDestination>(sampleDestination->cref());
    }
    return std::nullopt;
  }

  void setSampleDestination(
      const std::optional<cfg::SampleDestination>& sampleDest) {
    if (!sampleDest) {
      ref<switch_state_tags::sampleDest>().reset();
      return;
    }
    set<switch_state_tags::sampleDest>(enumToName(sampleDest.value()));
  }

  std::optional<std::string> getIngressMirror() const {
    if (auto ingressMirror = cref<switch_state_tags::ingressMirror>()) {
      return ingressMirror->cref();
    }
    return std::nullopt;
  }

  void setIngressMirror(const std::optional<std::string>& mirror) {
    if (!mirror) {
      ref<switch_state_tags::ingressMirror>().reset();
      return;
    }
    set<switch_state_tags::ingressMirror>(mirror.value());
  }

  std::optional<std::string> getEgressMirror() const {
    if (auto egressMirror = cref<switch_state_tags::egressMirror>()) {
      return egressMirror->cref();
    }
    return std::nullopt;
  }

  void setEgressMirror(const std::optional<std::string>& mirror) {
    if (!mirror) {
      ref<switch_state_tags::egressMirror>().reset();
      return;
    }
    set<switch_state_tags::egressMirror>(mirror.value());
  }

  std::optional<std::string> getQosPolicy() const {
    if (auto qosPolicy = cref<switch_state_tags::qosPolicy>()) {
      return qosPolicy->cref();
    }
    return std::nullopt;
  }

  void setQosPolicy(const std::optional<std::string>& qosPolicy) {
    if (!qosPolicy) {
      ref<switch_state_tags::qosPolicy>().reset();
      return;
    }
    set<switch_state_tags::qosPolicy>(qosPolicy.value());
  }

  LLDPValidations getLLDPValidations() const {
    // THRIFT_COPY
    return cref<switch_state_tags::expectedLLDPValues>()->toThrift();
  }

  auto getExpectedNeighborValues() const {
    return safe_cref<switch_state_tags::expectedNeighborReachability>();
  }

  void setExpectedLLDPValues(LLDPValidations vals) {
    set<switch_state_tags::expectedLLDPValues>(vals);
  }

  void setExpectedNeighborReachability(const NeighborReachability& vals) {
    set<switch_state_tags::expectedNeighborReachability>(vals);
  }

  std::vector<cfg::AclLookupClass> getLookupClassesToDistributeTrafficOn()
      const {
    // THRIFT_COPY
    return cref<switch_state_tags::lookupClassesToDistrubuteTrafficOn>()
        ->toThrift();
  }

  void setLookupClassesToDistributeTrafficOn(
      const std::vector<cfg::AclLookupClass>&
          lookupClassesToDistrubuteTrafficOn) {
    set<switch_state_tags::lookupClassesToDistrubuteTrafficOn>(
        lookupClassesToDistrubuteTrafficOn);
  }

  phy::ProfileSideConfig getProfileConfig() const {
    // THRIFT_COPY
    return cref<switch_state_tags::profileConfig>()->toThrift();
  }
  void setProfileConfig(const phy::ProfileSideConfig& profileCfg) {
    set<switch_state_tags::profileConfig>(profileCfg);
  }

  std::vector<phy::PinConfig> getPinConfigs() const {
    // THRIFT_COPY
    return cref<switch_state_tags::pinConfigs>()->toThrift();
  }
  void resetPinConfigs(std::vector<phy::PinConfig> pinCfgs) {
    set<switch_state_tags::pinConfigs>(pinCfgs);
  }

  std::optional<phy::ProfileSideConfig> getLineProfileConfig() const {
    if (auto lineProfileConfig = cref<switch_state_tags::lineProfileConfig>()) {
      // THRIFT_COPY
      return lineProfileConfig->toThrift();
    }
    return std::nullopt;
  }
  void setLineProfileConfig(const phy::ProfileSideConfig& profileCfg) {
    set<switch_state_tags::lineProfileConfig>(profileCfg);
  }

  std::optional<std::vector<phy::PinConfig>> getLinePinConfigs() const {
    if (auto linePinConfigs = cref<switch_state_tags::linePinConfigs>()) {
      // THRIFT_COPY
      return linePinConfigs->toThrift();
    }
    return std::nullopt;
  }
  void resetLinePinConfigs(std::vector<phy::PinConfig> pinCfgs) {
    set<switch_state_tags::linePinConfigs>(pinCfgs);
  }

  InterfaceID getInterfaceID() const;
  std::vector<int32_t> getInterfaceIDs() const {
    return safe_cref<switch_state_tags::interfaceIDs>()->toThrift();
  }

  void setInterfaceIDs(const std::vector<int32_t>& interfaceIDs) {
    set<switch_state_tags::interfaceIDs>(interfaceIDs);
  }

  std::optional<std::string> getFlowletConfigName() const {
    if (auto name = cref<switch_state_tags::flowletConfigName>()) {
      return name->cref();
    }
    return std::nullopt;
  }

  void setFlowletConfigName(const std::optional<std::string>& name) {
    if (!name) {
      ref<switch_state_tags::flowletConfigName>().reset();
      return;
    }
    set<switch_state_tags::flowletConfigName>(name.value());
  }

  Port* modify(std::shared_ptr<SwitchState>* state);

  void fillPhyInfo(phy::PhyInfo* phyInfo);

 private:
  auto getRxSaks() const {
    return safe_cref<switch_state_tags::rxSecureAssociationKeys>();
  }
  void setRxSaks(const std::vector<state::RxSak>& rxSaks) {
    set<switch_state_tags::rxSecureAssociationKeys>(rxSaks);
  }

  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
