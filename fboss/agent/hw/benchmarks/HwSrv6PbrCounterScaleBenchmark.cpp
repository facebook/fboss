//
// HwSrv6PbrCounterScaleBenchmark.cpp
//
// SRv6 PBR per-ACE packet+byte counter scale benchmarks on Cisco Silicon One.
// Programs 448 copies of one ACE shape (RedirectToSrv6BumpsPacketAndByteCounter
// gold tier): TC + ROUTE_DST match, shared gold routeDst per bucket, gold SRv6
// redirect, per-ACE counter. TC cycles 0-7; routeDst bucket every 8 ACEs (56
// NHGs for 448 unique keys). GR2 ingress RTF TCAM is split IPv4/IPv6; IPv6 half
// is 448 lines (896 total).
//
//   HwSrv6PbrCounterScaleBenchmark_Configure -- times applyNewConfig() for 448
//   ACEs. HwSrv6PbrCounterScaleBenchmark_Read      -- times updateStats() (448
//   SAI byte
//                                              counter reads) + reading byte
//                                              counters from stats cache
//                                              (install is untimed).

#include <algorithm>

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

namespace {

constexpr int kAcePrefixLen = 128;
// Ingress IPv6 ACL table capacity on Yuba / GR2 (448 entries; 896 total TCAM
// split evenly between IPv4 and IPv6 RTF tables).
constexpr int kMaxIngressIpv6AclEntries = 448;
// SAI FIELD_TC is 4-bit (values 0-7); see sai_acl_key.cpp.
constexpr int kMaxPbrTc = 8;
constexpr int kMaxRouteDstNhGroups =
    (kMaxIngressIpv6AclEntries + kMaxPbrTc - 1) / kMaxPbrTc;

constexpr char kTunnelId[] = "srv6PbrScaleTunnel0";
constexpr char kEncapSrcIp[] = "2401:db00::1";
constexpr char kQosPolicyName[] = "srv6_pbr_scale_qos";
constexpr char kPbrAclTableName[] = "Srv6PbrAclTable";
constexpr char kScaleRouteDstNhgName[] = "scaleRouteDstNhg";
constexpr char kScaleRedirectNhgName[] = "scaleRedirectNhg";
// AgentSrv6PbrCounterTest gold tier (RedirectToSrv6BumpsPacketAndByteCounter).
constexpr char kGoldRouteSid[] = "bbbb:b1b1:51:52:53:54:55:56";
constexpr uint8_t kGoldTierTc = 1;
constexpr uint8_t kGoldTierDscp = 10;
constexpr char kScaleRouteV6Dst[] = "2001:db8:ca5e::1";
constexpr uint8_t kScaleRouteV6PrefixLen = 128;

#if defined(TAJO_SDK)
constexpr bool kUseSrv6PbrTcRouteDstPath = true;
#else
constexpr bool kUseSrv6PbrTcRouteDstPath = false;
#endif

folly::IPAddressV6 makePbrAceMatchDip(int aceIdx) {
  return folly::IPAddressV6(fmt::format("2001:db8:{:x}::1", aceIdx + 1));
}

// Bucket 0 uses agent gold uSID; higher buckets uniquify routeDst for scale.
std::string makeScaleRouteDstSid(int routeDstBucket) {
  if (routeDstBucket == 0) {
    return kGoldRouteSid;
  }
  return fmt::format(
      "bbbb:b1b1:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
      0x50 + (routeDstBucket & 0xff),
      (routeDstBucket >> 8) & 0xff,
      (routeDstBucket + 1) & 0xff,
      (routeDstBucket + 2) & 0xff,
      (routeDstBucket + 3) & 0xff,
      (routeDstBucket + 4) & 0xff);
}

std::string scaleRouteDstNhgName(int routeDstBucket) {
  if (routeDstBucket == 0) {
    return kScaleRouteDstNhgName;
  }
  return fmt::format("scaleRouteDstNhg_{}", routeDstBucket);
}

void enableSrv6PbrBenchmarkFlags() {
  FLAGS_enable_acl_table_group = true;
#if defined(TAJO_SDK)
  FLAGS_enable_nexthop_id_manager = true;
  FLAGS_resolve_nexthops_from_id = true;
#endif
}

void addSrv6EncapTunnelConfig(cfg::SwitchConfig& cfg) {
  cfg::Srv6Tunnel tunnel;
  tunnel.srv6TunnelId() = kTunnelId;
  tunnel.underlayIntfID() = *cfg.interfaces()[0].intfID();
  tunnel.tunnelType() = TunnelType::SRV6_ENCAP;
  tunnel.srcIp() = kEncapSrcIp;
  tunnel.ttlMode() = cfg::TunnelMode::UNIFORM;
  tunnel.dscpMode() = cfg::TunnelMode::UNIFORM;
  tunnel.ecnMode() = cfg::TunnelMode::UNIFORM;
  cfg.srv6Tunnels() = {tunnel};
}

void addScaleDscpToTcQosMap(cfg::SwitchConfig& cfg) {
  cfg::QosMap qosMap;
  qosMap.dscpMaps()->resize(1);
  qosMap.dscpMaps()[0].internalTrafficClass() = kGoldTierTc;
  qosMap.dscpMaps()[0].fromDscpToTrafficClass()->push_back(kGoldTierDscp);
  qosMap.trafficClassToQueueId()->emplace(kGoldTierTc, 0);

  cfg.qosPolicies()->resize(1);
  cfg.qosPolicies()[0].name() = kQosPolicyName;
  cfg.qosPolicies()[0].qosMap() = qosMap;

  if (!cfg.dataPlaneTrafficPolicy().has_value()) {
    cfg.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  }
  cfg.dataPlaneTrafficPolicy()->defaultQosPolicy() = kQosPolicyName;
}

void addSrv6PbrAclTable(cfg::SwitchConfig& cfg) {
  if (!FLAGS_enable_acl_table_group) {
    return;
  }
  if (utility::getAclTable(cfg, cfg::AclStage::INGRESS, kPbrAclTableName)) {
    return;
  }
  utility::addAclTable(
      &cfg,
      cfg::AclStage::INGRESS,
      kPbrAclTableName,
      1 /* priority */,
      {
          cfg::AclTableActionType::COUNTER,
          cfg::AclTableActionType::REDIRECT,
      },
      {
          cfg::AclTableQualifier::TC,
          cfg::AclTableQualifier::ROUTE_DST,
      },
      {});
}

cfg::RedirectNextHop makeSrv6RedirectNh(
    const cfg::SwitchConfig& cfg,
    const folly::IPAddress& underlayNhIp,
    const std::string& sid) {
  cfg::RedirectNextHop nh;
  nh.ip() = underlayNhIp.str();
  nh.intfID() = *cfg.interfaces()[0].intfID();
  nh.tunnelType() = TunnelType::SRV6_ENCAP;
  nh.tunnelId() = kTunnelId;
  nh.srv6SegmentList() = {sid};
  return nh;
}

AgentEnsembleSwitchConfigFn makePbrConfigFn() {
  return [](const AgentEnsemble& ensemble) {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    if (kUseSrv6PbrTcRouteDstPath) {
      addSrv6EncapTunnelConfig(cfg);
      addScaleDscpToTcQosMap(cfg);
    }
    return cfg;
  };
}

struct PbrAceNames {
  std::string ace;
  std::string counter;
};

PbrAceNames makeAceNames(int aceIdx) {
  return {
      fmt::format("srv6_pbr_ace_{}", aceIdx),
      fmt::format("srv6_pbr_ctr_{}", aceIdx),
  };
}

void appendPbrAceDscpDstIpFallback(
    cfg::SwitchConfig& cfg,
    int aceIdx,
    const folly::IPAddressV6& redirectNh,
    const std::vector<cfg::CounterType>& counterTypes) {
  const auto names = makeAceNames(aceIdx);

  cfg::AclEntry ace;
  ace.name() = names.ace;
  ace.dscp() = static_cast<int8_t>(aceIdx % 64);
  ace.dstIp() =
      fmt::format("{}/{}", makePbrAceMatchDip(aceIdx).str(), kAcePrefixLen);
  utility::addAcl(&cfg, ace, cfg::AclStage::INGRESS);

  utility::addTrafficCounter(&cfg, names.counter, counterTypes);

  cfg::RedirectToNextHopAction redirect;
  cfg::RedirectNextHop rnh;
  rnh.ip() = redirectNh.str();

  cfg::MatchAction matchAction;
  matchAction.redirectToNextHop() = redirect;
  matchAction.counter() = names.counter;

  utility::addMatcher(&cfg, names.ace, matchAction);
}

// One ACE template (agent gold tier): TC + routeDst + gold SRv6 redirect +
// counter. aceIdx only varies TC (0-7) and routeDst bucket for unique keys.
void appendPbrAceTcRouteDstSrv6(
    cfg::SwitchConfig& cfg,
    int aceIdx,
    const folly::IPAddress& underlayNhIp,
    const cfg::RedirectNextHop& goldRedirectNh,
    const std::vector<cfg::CounterType>& counterTypes) {
  const auto names = makeAceNames(aceIdx);
  const int routeDstBucket = aceIdx / kMaxPbrTc;

  cfg::AclEntry ace;
  ace.name() = names.ace;
  ace.tc() = static_cast<int8_t>(aceIdx % kMaxPbrTc);
  ace.routeDst() = makeSrv6RedirectNh(
      cfg, underlayNhIp, makeScaleRouteDstSid(routeDstBucket));
  utility::addAclEntry(&cfg, ace, kPbrAclTableName, cfg::AclStage::INGRESS);

  utility::addTrafficCounter(&cfg, names.counter, counterTypes);

  cfg::RedirectToNextHopAction redirect;
  redirect.redirectNextHops() = {goldRedirectNh};

  cfg::MatchAction matchAction;
  matchAction.redirectToNextHop() = redirect;
  matchAction.counter() = names.counter;

  cfg::MatchToAction mta;
  mta.matcher() = names.ace;
  mta.action() = matchAction;
  if (!cfg.dataPlaneTrafficPolicy().has_value()) {
    cfg.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  }
  cfg.dataPlaneTrafficPolicy()->matchToAction()->push_back(mta);
}

struct PbrScaleEnsemble {
  std::unique_ptr<AgentEnsemble> ensemble;
  SwSwitch* sw{nullptr};
  folly::IPAddress underlayNhIp;
  cfg::RedirectNextHop goldRedirectNh;
};

RouteNextHopSet makeSrv6NhopSet(
    const utility::EcmpSetupAnyNPorts6& ecmpHelper,
    const folly::IPAddress& underlayNhIp,
    const std::string& sid) {
  const auto nhop = ecmpHelper.nhop(0);
  CHECK(nhop.linkLocalNhopIp.has_value());
  const folly::IPAddressV6 srv6Sid(sid);

  RouteNextHopSet nhops;
  nhops.insert(ResolvedNextHop(
      underlayNhIp,
      nhop.intf,
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::vector<folly::IPAddressV6>{srv6Sid},
      TunnelType::SRV6_ENCAP,
      kTunnelId));
  return nhops;
}

void programScaleSrv6NhgsAndRoute(
    PbrScaleEnsemble& ctx,
    const utility::EcmpSetupAnyNPorts6& ecmpHelper) {
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.reserve(kMaxRouteDstNhGroups + 1);
  for (int i = 0; i < kMaxRouteDstNhGroups; ++i) {
    groups.emplace_back(
        scaleRouteDstNhgName(i),
        makeSrv6NhopSet(ecmpHelper, ctx.underlayNhIp, makeScaleRouteDstSid(i)));
  }
  groups.emplace_back(
      kScaleRedirectNhgName,
      makeSrv6NhopSet(ecmpHelper, ctx.underlayNhIp, kGoldRouteSid));

  auto rib = ctx.sw->getRib();
  rib->addOrUpdateNamedNextHopGroups(
      ctx.sw->getScopeResolver(),
      groups,
      createRibToSwitchStateFunction(),
      ctx.sw);

  UnicastRoute route;
  route.dest()->ip() =
      facebook::network::toBinaryAddress(folly::IPAddress(kScaleRouteV6Dst));
  route.dest()->prefixLength() = kScaleRouteV6PrefixLen;
  NamedRouteDestination namedDest;
  namedDest.nextHopGroup_ref() = kScaleRouteDstNhgName;
  route.namedRouteDestination() = namedDest;
  route.counterID() = kScaleRouteDstNhgName;

  auto routeUpdater = ctx.sw->getRouteUpdater();
  routeUpdater.addRoute(RouterID(0), ClientID::TE_AGENT, route);
  routeUpdater.program();
  waitForStateUpdates(ctx.sw);
}

PbrScaleEnsemble setupPbrScaleEnsemble() {
  enableSrv6PbrBenchmarkFlags();

  PbrScaleEnsemble ctx;
  ctx.ensemble =
      createAgentEnsemble(makePbrConfigFn(), false /*disableLinkStateToggler*/);
  ctx.sw = ctx.ensemble->getSw();

  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      ctx.sw->getState(), ctx.sw->needL2EntryForNeighbor());

  ctx.ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, 1, true /*useLinkLocal*/);
  });

  auto routeUpdater = ctx.sw->getRouteUpdater();
  ecmpHelper.programRoutes(&routeUpdater, 1);

  const auto& nhop = ecmpHelper.nhop(0);
  CHECK(nhop.linkLocalNhopIp.has_value());
  ctx.underlayNhIp = folly::IPAddress(*nhop.linkLocalNhopIp);

  if (kUseSrv6PbrTcRouteDstPath) {
    programScaleSrv6NhgsAndRoute(ctx, ecmpHelper);
    auto cfg = ctx.sw->getConfig();
    ctx.goldRedirectNh =
        makeSrv6RedirectNh(cfg, ctx.underlayNhIp, kGoldRouteSid);
  }

  return ctx;
}

const std::vector<cfg::CounterType> kPacketAndByteCounterTypes{
    cfg::CounterType::PACKETS,
    cfg::CounterType::BYTES};
const std::vector<cfg::CounterType> kBytesOnlyCounterTypes{
    cfg::CounterType::BYTES};

cfg::SwitchConfig buildPbrAceConfig(
    const PbrScaleEnsemble& ctx,
    int numEntries,
    const std::vector<cfg::CounterType>& counterTypes =
        kPacketAndByteCounterTypes) {
  auto cfg = ctx.sw->getConfig();
  if (kUseSrv6PbrTcRouteDstPath) {
    addSrv6PbrAclTable(cfg);
    for (int i = 0; i < numEntries; ++i) {
      appendPbrAceTcRouteDstSrv6(
          cfg, i, ctx.underlayNhIp, ctx.goldRedirectNh, counterTypes);
    }
  } else {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        ctx.sw->getState(), ctx.sw->needL2EntryForNeighbor());
    for (int i = 0; i < numEntries; ++i) {
      appendPbrAceDscpDstIpFallback(
          cfg, i, ecmpHelper.nhop(0).ip, counterTypes);
    }
  }
  return cfg;
}

void cleanupPbrAces(PbrScaleEnsemble& ctx, int numEntries) {
  auto cleanCfg = ctx.sw->getConfig();
  const std::optional<std::string> tableName = kUseSrv6PbrTcRouteDstPath
      ? std::optional<std::string>(kPbrAclTableName)
      : std::nullopt;

  for (int i = 0; i < numEntries; ++i) {
    const auto names = makeAceNames(i);
    utility::delAclStat(&cleanCfg, names.ace, names.counter);
    if (kUseSrv6PbrTcRouteDstPath) {
      auto& mta = *cleanCfg.dataPlaneTrafficPolicy()->matchToAction();
      mta.erase(
          std::remove_if(
              mta.begin(),
              mta.end(),
              [&](const cfg::MatchToAction& entry) {
                return *entry.matcher() == names.ace;
              }),
          mta.end());
    } else {
      utility::delMatcher(&cleanCfg, names.ace);
    }
    utility::delAcl(&cleanCfg, names.ace, tableName);
  }
  ctx.ensemble->applyNewConfig(cleanCfg);
  waitForStateUpdates(ctx.sw);
}

struct PbrScalePreparedRun {
  PbrScaleEnsemble ctx;
  cfg::SwitchConfig aceCfg;
  std::vector<std::string> counterNames;
};

PbrScalePreparedRun preparePbrScaleRun(int numEntries) {
  PbrScalePreparedRun run;
  run.ctx = setupPbrScaleEnsemble();
  run.aceCfg = buildPbrAceConfig(run.ctx, numEntries);
  run.counterNames.reserve(numEntries);
  for (int i = 0; i < numEntries; ++i) {
    run.counterNames.push_back(makeAceNames(i).counter);
  }
  return run;
}

void logPbrAceProgramming(int numEntries) {
  XLOG(INFO) << "HwSrv6PbrCounterScaleBenchmark: programming " << numEntries
             << " ACEs (path="
             << (kUseSrv6PbrTcRouteDstPath ? "TC+ROUTE_DST+SRV6" : "DSCP+dstIp")
             << ")";
}

void programPbrAcesUntimed(PbrScalePreparedRun& run) {
  logPbrAceProgramming(static_cast<int>(run.counterNames.size()));
  run.ctx.ensemble->applyNewConfig(run.aceCfg);
  waitForStateUpdates(run.ctx.sw);
}

void srv6PbrCounterConfigureScaleBenchmark(int numEntries) {
  folly::BenchmarkSuspender suspender;

  auto run = preparePbrScaleRun(numEntries);

  suspender.dismiss();
  programPbrAcesUntimed(run);
  suspender.rehire();

  cleanupPbrAces(run.ctx, numEntries);
}

void srv6PbrCounterReadScaleBenchmark(int numEntries) {
  folly::BenchmarkSuspender suspender;

  // Untimed: ensemble, NHGs, route, ACE config build, and hardware programming.
  // Byte-only counters so updateStats() issues one SAI read per ACE.
  auto ctx = setupPbrScaleEnsemble();
  auto aceCfg = buildPbrAceConfig(ctx, numEntries, kBytesOnlyCounterTypes);
  logPbrAceProgramming(numEntries);
  ctx.ensemble->applyNewConfig(aceCfg);
  waitForStateUpdates(ctx.sw);

  std::vector<std::string> counterNames;
  counterNames.reserve(numEntries);
  for (int i = 0; i < numEntries; ++i) {
    counterNames.push_back(makeAceNames(i).counter);
  }

  // Timed: SAI/SDK poll (updateStats) + byte counter reads from stats cache.
  suspender.dismiss();
  ctx.sw->updateStats();
  uint64_t totalBytes = 0;
  for (const auto& counterName : counterNames) {
    totalBytes +=
        utility::getAclInOutPackets(ctx.sw, counterName, true /*bytes*/);
  }
  folly::doNotOptimizeAway(totalBytes);
  suspender.rehire();

  XLOG(INFO) << "HwSrv6PbrCounterScaleBenchmark Read: polled " << numEntries
             << " byte counters";

  cleanupPbrAces(ctx, numEntries);
}

BENCHMARK(HwSrv6PbrCounterScaleBenchmark_Configure) {
  srv6PbrCounterConfigureScaleBenchmark(kMaxIngressIpv6AclEntries);
}

BENCHMARK(HwSrv6PbrCounterScaleBenchmark_Read) {
  srv6PbrCounterReadScaleBenchmark(kMaxIngressIpv6AclEntries);
}

} // namespace

} // namespace facebook::fboss
