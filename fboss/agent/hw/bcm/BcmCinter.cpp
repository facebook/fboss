/**
 * Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmCinter.h"

#include <array>
#include <iterator>
#include <string>
#include <tuple>
#include <vector>

#include <gflags/gflags.h>

#include <folly/FileUtil.h>
#include <folly/MapUtil.h>
#include <folly/Singleton.h>
#include <folly/String.h>

#include "fboss/agent/SysError.h"

DEFINE_string(
    cint_file,
    "/var/facebook/logs/fboss/bcm_cinter.log",
    "File to dump equivalent cint commands for each wrapped SDK api");

DEFINE_bool(enable_bcm_cinter, false, "Whether to enable bcm cinter");

DEFINE_bool(gen_tx_cint, false, "Generate cint for packet TX calls");

DEFINE_int32(
    cinter_log_timeout,
    100,
    "Log timeout value in milliseconds. Logger will periodically"
    "flush logs even if the buffer is not full");

using folly::join;
using folly::to;
using std::array;
using std::make_move_iterator;
using std::pair;
using std::string;
using std::tie;
using std::vector;

namespace {

folly::Singleton<facebook::fboss::BcmCinter> _bcmCinter;

template <typename T>
vector<string> makeParamVec(T&& arg) {
  return {to<string>(std::forward<T>(arg))};
}

template <typename T, typename... Args>
vector<string> makeParamVec(T&& first, Args&&... args) {
  vector<string> params = makeParamVec(std::forward<T>(first));
  auto remParams = makeParamVec(std::forward<Args>(args)...);
  params.insert(params.end(), remParams.begin(), remParams.end());
  return params;
}

template <typename... Args>
string makeParamStr(Args&&... args) {
  return join(", ", makeParamVec(std::forward<Args>(args)...));
}

template <typename T>
string getCintVar(
    folly::Synchronized<std::map<T, std::string>>& varMap,
    T key,
    std::optional<std::string> default_val = std::nullopt) {
  return varMap.withRLock([&](auto& vars) {
    return folly::get_default(vars, key, default_val.value_or(to<string>(key)));
  });
}

} // namespace

namespace facebook::fboss {

BcmCinter::BcmCinter() {
  if (FLAGS_enable_bcm_cinter) {
    asyncLogger_ = std::make_unique<AsyncLogger>(
        FLAGS_cint_file, FLAGS_cinter_log_timeout);

    asyncLogger_->startFlushThread();
    setupGlobals();
  }
}

BcmCinter::~BcmCinter() {
  if (FLAGS_enable_bcm_cinter) {
    asyncLogger_->forceFlush();
    asyncLogger_->stopFlushThread();
  }
}

std::shared_ptr<BcmCinter> BcmCinter::getInstance() {
  return _bcmCinter.try_get();
}

void BcmCinter::setupGlobals() {
  array<string, 27> globals = {
      "_shr_pbmp_t pbmp",
      "_shr_rx_reasons_t reasons",
      "bcm_cosq_gport_discard_t discard",
      "bcm_field_qset_t qset",
      "bcm_if_t intf_array[64]",
      "bcm_l2_station_t station",
      "bcm_l3_egress_ecmp_t ecmp",
      "bcm_l3_egress_t l3_egress",
      "bcm_l3_host_t l3_host",
      "bcm_l3_intf_t l3_intf",
      "bcm_l3_route_t l3_route",
      "bcm_mirror_destination_t mirror_dest",
      "bcm_mpls_egress_label_t label_array[10]",
      "bcm_mpls_tunnel_switch_t switch_info",
      "bcm_pkt_t *pkt",
      "bcm_qos_map_t qos_map_entry",
      "bcm_rx_cfg_t rx_cfg",
      "bcm_trunk_info_t trunk_info",
      "bcm_trunk_member_t trunk_member",
      "bcm_trunk_t tid",
      "bcm_cosq_bst_profile_t cosq_bst_profile",
      "int num_labels",
      "int rv",
      "int station_id",
      "bcm_port_resource_t resource",
      "bcm_port_resource_t resources[4]",
      "bcm_port_phy_tx_t tx",
  };
  writeCintLines(std::move(globals));
}

template <typename C>
void BcmCinter::writeCintLines(C&& lines) {
  auto constexpr kSeparator = ";\n";
  auto cint = join(kSeparator, std::forward<C>(lines)) + kSeparator;
  if (FLAGS_enable_bcm_cinter) {
    asyncLogger_->appendLog(cint.c_str(), cint.size());
  }
  linesWritten_ += lines.size();
}

void BcmCinter::writeCintLine(const std::string& cint) {
  writeCintLines(array<string, 1>{cint});
}

vector<string> BcmCinter::wrapFunc(const string& funcCall) const {
  vector<string> cintLines = {
      to<string>("rv = ", funcCall),
      // NOTE this is approximate line number in particular it
      // will point to line number just before the lines written
      // in setting up args and making this function call.
      to<string>(
          "if (rv) { printf(\"\\nError around line ",
          linesWritten_.load(),
          " : ",
          funcCall.substr(0, funcCall.find("(")),
          ": %d -> %s\", rv, bcm_errmsg(rv));  }")};
  return cintLines;
}

vector<string> BcmCinter::cintForQset(const bcm_field_qset_t& qset) const {
  vector<string> cintLines = {"BCM_FIELD_QSET_INIT(qset)"};
  for (auto qual = 0; qual < bcmFieldQualifyCount; ++qual) {
    if (BCM_FIELD_QSET_TEST(qset, qual)) {
      cintLines.emplace_back(
          to<string>("BCM_FIELD_QSET_ADD(qset, ", qual, ")"));
    }
  }
  return cintLines;
}

vector<string> BcmCinter::cintForCosQDiscard(
    const bcm_cosq_gport_discard_t& discard) const {
  vector<string> cintLines = {
      "bcm_cosq_gport_discard_t_init(&discard)",
      to<string>("discard.flags=", discard.flags),
      to<string>("discard.min_thresh=", discard.min_thresh),
      to<string>("discard.max_thresh=", discard.max_thresh),
      to<string>("discard.drop_probability=", discard.drop_probability)};
  return cintLines;
}

string BcmCinter::getNextTmpV6Var() {
  return to<string>("tmpV6_", ++tmpV6Created_);
}

string BcmCinter::getNextTmpMacVar() {
  return to<string>("tmpMac_", ++tmpMacCreated_);
}

string BcmCinter::getNextTmpStatArray() {
  return to<string>("tmpStatArray_", ++tmpStatArrayCreated_);
}

string BcmCinter::getNextTmpTrunkMemberArray() {
  return to<string>("tmpTrunkMemberArray_", ++tmpTrunkMemberArrayCreated_);
}

string BcmCinter::getNextTmpPortBitmapVar() {
  return to<string>("tmpPbmp_", ++tmpPortBitmapCreated_);
}

string BcmCinter::getNextTmpReasonsVar() {
  return to<string>("tmpReasons_", ++tmpReasonsCreated_);
}

string BcmCinter::getNextFieldEntryVar() {
  return to<string>("fieldEntry_", ++fieldEntryCreated_);
}

string BcmCinter::getNextQosMapVar() {
  return to<string>("qosMap_", ++qosMapCreated_);
}

string BcmCinter::getNextL3IntfIdVar() {
  return to<string>("l3Intf_", ++l3IntfCreated_);
}

string BcmCinter::getNextStatVar() {
  return to<string>("stat_", ++statCreated_);
}

string BcmCinter::getNextMirrorDestIdVar() {
  return to<string>("mirrorDestId_", ++mirrorDestIdCreated_);
}

pair<string, string> BcmCinter::cintForIp6(const bcm_ip6_t in) {
  array<string, 16> bytes;
  for (auto i = 0; i < 16; ++i) {
    bytes[i] = to<string>(in[i]);
  }
  auto v6VarName = getNextTmpV6Var();
  return make_pair(
      to<string>("bcm_ip6_t ", v6VarName, " = ", " {", join(", ", bytes), "}"),
      v6VarName);
}

pair<string, string> BcmCinter::cintForMac(const bcm_mac_t in) {
  array<string, 6> bytes;
  for (auto i = 0; i < 6; ++i) {
    bytes[i] = to<string>(in[i]);
  }
  auto macVarName = getNextTmpMacVar();
  return make_pair(
      to<string>("bcm_mac_t ", macVarName, " = ", " {", join(", ", bytes), "}"),
      macVarName);
}

pair<string, string> BcmCinter::cintForStatArray(
    const bcm_field_stat_t* statArray,
    int statSize) {
  std::vector<string> stats;
  for (int i = 0; i < statSize; ++i) {
    stats.push_back(to<string>(statArray[i]));
  }
  auto statArrayName = getNextTmpStatArray();
  return make_pair(
      to<string>(
          "bcm_field_stat_t ",
          statArrayName,
          "[] =",
          " {",
          join(", ", stats),
          "}"),
      statArrayName);
}

vector<string> BcmCinter::cintForL3Ecmp(const bcm_l3_egress_ecmp_t& ecmp) {
  vector<string> cintLines = {
      "bcm_l3_egress_ecmp_t_init(&ecmp)",
      to<string>("ecmp.flags=", ecmp.flags),
      to<string>("ecmp.max_paths=", ecmp.max_paths),
      to<string>("ecmp.ecmp_intf=", getCintVar(l3IntfIdVars, ecmp.ecmp_intf))};
  return cintLines;
}

vector<string> BcmCinter::cintForL3Route(const bcm_l3_route_t& l3_route) {
  vector<string> cintLines;

  if (l3_route.l3a_flags & BCM_L3_IP6) {
    string cintV6, v6Var, cintMask, v6Mask;
    tie(cintV6, v6Var) = cintForIp6(l3_route.l3a_ip6_net);
    tie(cintMask, v6Mask) = cintForIp6(l3_route.l3a_ip6_mask);
    cintLines = {cintV6,
                 cintMask,
                 "bcm_l3_route_t_init(&l3_route)",
                 to<string>("l3_route.l3a_vrf = ", l3_route.l3a_vrf),
                 to<string>(
                     "l3_route.l3a_intf = ",
                     getCintVar(l3IntfIdVars, l3_route.l3a_intf)),
                 to<string>("l3_route.l3a_ip6_net = ", v6Var),
                 to<string>("l3_route.l3a_ip6_mask = ", v6Mask),
                 to<string>("l3_route.l3a_flags = ", l3_route.l3a_flags)};

  } else {
    cintLines = {"bcm_l3_route_t_init(&l3_route)",
                 to<string>("l3_route.l3a_vrf = ", l3_route.l3a_vrf),
                 to<string>(
                     "l3_route.l3a_intf = ",
                     getCintVar(l3IntfIdVars, l3_route.l3a_intf)),
                 to<string>("l3_route.l3a_subnet = ", l3_route.l3a_subnet),
                 to<string>("l3_route.l3a_ip_mask = ", l3_route.l3a_ip_mask),
                 to<string>("l3_route.l3a_flags = ", l3_route.l3a_flags)};
  }
  return cintLines;
}

vector<string> BcmCinter::cintForL3Intf(const bcm_l3_intf_t& l3_intf) {
  vector<string> cintLines;
  string cintMac, macVar;
  tie(cintMac, macVar) = cintForMac(l3_intf.l3a_mac_addr);
  cintLines = {cintMac,
               "bcm_l3_intf_t_init(&l3_intf)",
               to<string>("l3_intf.l3a_vrf = ", l3_intf.l3a_vrf),
               to<string>("l3_intf.l3a_vid = ", l3_intf.l3a_vid),
               to<string>("l3_intf.l3a_mtu = ", l3_intf.l3a_mtu),
               to<string>("l3_intf.l3a_mac_addr = ", macVar),
               to<string>("l3_intf.l3a_flags = ", l3_intf.l3a_flags)};
  if (l3_intf.l3a_flags & BCM_L3_WITH_ID) {
    cintLines.push_back(
        to<string>("l3_intf.l3a_intf_id = ", l3_intf.l3a_intf_id));
  }
  return cintLines;
}

vector<string> BcmCinter::cintForMirrorDestination(
    const bcm_mirror_destination_t* mirror_dest) {
  vector<string> cintLines{
      to<string>("bcm_mirror_destination_t_init(&mirror_dest)"),
      to<string>("mirror_dest.gport = ", mirror_dest->gport),
      to<string>("mirror_dest.flags = ", mirror_dest->flags),
      to<string>("mirror_dest.tos = ", mirror_dest->tos),
  };
  if (!(mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_IP_GRE) &&
      !(mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_SFLOW)) {
    return cintLines;
  }
  cintLines.push_back(
      to<string>("mirror_dest.version = ", mirror_dest->version));

  cintLines.push_back(
      to<string>("mirror_dest.gre_protocol = ", mirror_dest->gre_protocol));
  cintLines.push_back(to<string>("mirror_dest.ttl = ", mirror_dest->ttl));
  cintLines.push_back(
      to<string>("mirror_dest.udp_src_port = ", mirror_dest->udp_src_port));
  cintLines.push_back(
      to<string>("mirror_dest.udp_dst_port = ", mirror_dest->udp_dst_port));

  string srcMac, dstMac;
  string srcMacName, dstMacName;
  tie(srcMac, srcMacName) = cintForMac(mirror_dest->src_mac);
  tie(dstMac, dstMacName) = cintForMac(mirror_dest->dst_mac);
  cintLines.push_back(srcMac);
  cintLines.push_back(to<string>("mirror_dest.src_mac = ", srcMacName));
  cintLines.push_back(dstMac);
  cintLines.push_back(to<string>("mirror_dest.dst_mac = ", dstMacName));

  if (mirror_dest->version == 4) {
    cintLines.push_back(
        to<string>("mirror_dest.src_addr = ", mirror_dest->src_addr));
    cintLines.push_back(
        to<string>("mirror_dest.dst_addr = ", mirror_dest->dst_addr));
  } else {
    string cintSrcV6Addr, cintDstV6Addr;
    string cintSrcV6Name, cintDstV6Name;
    tie(cintSrcV6Addr, cintSrcV6Name) = cintForIp6(mirror_dest->src6_addr);
    tie(cintDstV6Addr, cintDstV6Name) = cintForIp6(mirror_dest->dst6_addr);
    cintLines.push_back(cintSrcV6Addr);
    cintLines.push_back(to<string>("mirror_dest.src6_addr = ", cintSrcV6Name));
    cintLines.push_back(cintDstV6Name);
    cintLines.push_back(to<string>("mirror_dest.dst6_addr = ", cintDstV6Name));
  }
  return cintLines;
}

vector<string> BcmCinter::cintForL3Host(const bcm_l3_host_t& l3_host) {
  vector<string> cintLines;
  if (l3_host.l3a_flags & BCM_L3_IP6) {
    string cintV6, v6Var;
    tie(cintV6, v6Var) = cintForIp6(l3_host.l3a_ip6_addr);
    cintLines = {
        cintV6,
        "bcm_l3_host_t_init(&l3_host)",
        to<string>("l3_host.l3a_vrf = ", l3_host.l3a_vrf),
        to<string>(
            "l3_host.l3a_intf = ", getCintVar(l3IntfIdVars, l3_host.l3a_intf)),
        to<string>("l3_host.l3a_flags = ", l3_host.l3a_flags),
        to<string>("l3_host.l3a_ip6_addr = ", v6Var)};

  } else {
    cintLines = {
        "bcm_l3_host_t_init(&l3_host)",
        to<string>("l3_host.l3a_vrf = ", l3_host.l3a_vrf),
        to<string>(
            "l3_host.l3a_intf = ", getCintVar(l3IntfIdVars, l3_host.l3a_intf)),
        to<string>("l3_host.l3a_flags = ", l3_host.l3a_flags),
        to<string>("l3_host.l3a_ip_addr = ", l3_host.l3a_ip_addr)};
  }
  return cintLines;
}

std::vector<std::string> BcmCinter::cintForLabelsArray(
    int num_labels,
    bcm_mpls_egress_label_t* label_array) {
  vector<string> cintLines;
  for (auto i = 0; i < num_labels; i++) {
    cintLines.push_back(
        to<string>("bcm_mpls_egress_label_t_init(&label_array[", i, "])"));
    cintLines.push_back(
        to<string>("label_array[", i, "].label = ", label_array[i].label));
    cintLines.push_back(
        to<string>("label_array[", i, "].flags = ", label_array[i].flags));
    cintLines.push_back(to<string>(
        "label_array[", i, "].qos_map_id = ", label_array[i].qos_map_id));
  }
  return cintLines;
}

std::vector<std::string> BcmCinter::cintForMplsTunnelSwitch(
    const bcm_mpls_tunnel_switch_t& switch_info) {
  vector<string> cintLines;
  cintLines.push_back(
      to<string>("bcm_mpls_tunnel_switch_t_init(&switch_info)"));
  cintLines.push_back(to<string>("switch_info.label = ", switch_info.label));
  cintLines.push_back(to<string>("switch_info.flags = ", switch_info.flags));
  cintLines.push_back(to<string>("switch_info.port = ", switch_info.port));
  cintLines.push_back(to<string>("switch_info.action = ", switch_info.action));
  cintLines.push_back(to<string>(
      "switch_info.egress_if = ",
      getCintVar(l3IntfIdVars, switch_info.egress_if)));
  return cintLines;
}

int BcmCinter::bcm_cosq_init(int unit) {
  writeCintLines(wrapFunc(to<string>("bcm_cosq_init(", unit, ")")));
  return 0;
}

int BcmCinter::bcm_cosq_gport_sched_set(
    int unit,
    bcm_gport_t gport,
    bcm_cos_queue_t cosq,
    int mode,
    int weight) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_cosq_gport_sched_set(",
      makeParamStr(unit, gport, cosq, mode, weight),
      ")")));
  return 0;
}

int BcmCinter::bcm_cosq_control_set(
    int unit,
    bcm_gport_t port,
    bcm_cos_queue_t cosq,
    bcm_cosq_control_t type,
    int arg) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_cosq_control_set(",
      makeParamStr(unit, port, cosq, type, arg),
      ")")));
  return 0;
}

int BcmCinter::bcm_cosq_gport_bandwidth_set(
    int unit,
    bcm_gport_t gport,
    bcm_cos_queue_t cosq,
    uint32 kbits_sec_min,
    uint32 kbits_sec_max,
    uint32 flags) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_cosq_gport_bandwidth_set(",
      makeParamStr(unit, gport, cosq, kbits_sec_min, kbits_sec_max, flags),
      ")")));
  return 0;
}

int BcmCinter::bcm_cosq_gport_discard_set(
    int unit,
    bcm_gport_t gport,
    bcm_cos_queue_t cosq,
    bcm_cosq_gport_discard_t* discard) {
  auto cint = cintForCosQDiscard(*discard);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_cosq_gport_discard_set(",
      makeParamStr(unit, gport, cosq, "&discard"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_cosq_gport_mapping_set(
    int unit,
    bcm_gport_t ing_port,
    bcm_cos_t priority,
    uint32_t flags,
    bcm_gport_t gport,
    bcm_cos_queue_t cosq) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_cosq_gport_mapping_set(",
      makeParamStr(unit, ing_port, priority, flags, gport, cosq),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_init(int unit) {
  writeCintLines(wrapFunc(to<string>("bcm_field_init(", unit, ")")));
  return 0;
}

int BcmCinter::bcm_field_group_create_id(
    int unit,
    bcm_field_qset_t groupQset,
    int pri,
    bcm_field_group_t group) {
  auto cint = cintForQset(groupQset);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_field_group_create_id(",
      makeParamStr(unit, "qset", pri, group),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_field_group_destroy(int unit, bcm_field_group_t group) {
  writeCintLines(wrapFunc(
      to<string>("bcm_field_group_destroy(", makeParamStr(unit, group), ")")));
  return 0;
}

int BcmCinter::bcm_field_entry_create(
    int unit,
    bcm_field_group_t group,
    bcm_field_entry_t* entry) {
  string entryVar = getNextFieldEntryVar();
  writeCintLine(to<string>("bcm_field_entry_t ", entryVar));
  { fieldEntryVars.wlock()->emplace(*entry, entryVar); }
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_create(",
      makeParamStr(unit, group, to<string>("&", entryVar)),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_entry_create_id(
    int unit,
    bcm_field_group_t group,
    bcm_field_entry_t entry) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_create(", makeParamStr(unit, group, entry), ")")));
  return 0;
}

int BcmCinter::bcm_field_entry_install(int unit, bcm_field_entry_t entry) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_install(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry)),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_entry_reinstall(int unit, bcm_field_entry_t entry) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_reinstall(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry)),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_entry_prio_set(
    int unit,
    bcm_field_entry_t entry,
    int prio) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_prio_set(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), prio),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_stat_create(
    int unit,
    bcm_field_group_t group,
    int nstat,
    bcm_field_stat_t* stat_arr,
    int* stat_id) {
  string statArrayCint, statArrayVar, statIdVar, statIdVarCint;
  statIdVar = getNextStatVar();
  { statIdVars.wlock()->emplace(*stat_id, statIdVar); }
  statIdVarCint = to<string>("int ", statIdVar);
  tie(statArrayCint, statArrayVar) = cintForStatArray(stat_arr, nstat);
  vector<string> cint = {statIdVarCint, statArrayCint};
  auto cintForFn = wrapFunc(to<string>(
      "bcm_field_stat_create(",
      makeParamStr(
          unit, group, nstat, statArrayVar, to<string>("&", statIdVar)),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_field_stat_destroy(int unit, int stat_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_stat_destroy(",
      makeParamStr(unit, getCintVar(statIdVars, stat_id)),
      ")")));
  { statIdVars.wlock()->erase(stat_id); }
  return 0;
}

int BcmCinter::bcm_field_entry_stat_attach(
    int unit,
    bcm_field_entry_t entry,
    int stat_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_stat_attach(",
      makeParamStr(
          unit,
          getCintVar(fieldEntryVars, entry),
          getCintVar(statIdVars, stat_id)),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_entry_stat_detach(
    int unit,
    bcm_field_entry_t entry,
    int stat_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_stat_detach(",
      makeParamStr(
          unit,
          getCintVar(fieldEntryVars, entry),
          getCintVar(statIdVars, stat_id)),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_action_add(
    int unit,
    bcm_field_entry_t entry,
    bcm_field_action_t action,
    uint32 param0,
    uint32 param1) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_action_add(",
      makeParamStr(
          unit, getCintVar(fieldEntryVars, entry), action, param0, param1),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_action_delete(
    int unit,
    bcm_field_entry_t entry,
    bcm_field_action_t action,
    uint32 param0,
    uint32 param1) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_action_delete(",
      makeParamStr(
          unit, getCintVar(fieldEntryVars, entry), action, param0, param1),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_DstIp6(
    int unit,
    bcm_field_entry_t entry,
    bcm_ip6_t data,
    bcm_ip6_t mask) {
  return bcmFieldQualifyIp6(unit, entry, data, mask, Dir::DST);
}

int BcmCinter::bcm_field_qualify_SrcIp6(
    int unit,
    bcm_field_entry_t entry,
    bcm_ip6_t data,
    bcm_ip6_t mask) {
  return bcmFieldQualifyIp6(unit, entry, data, mask, Dir::SRC);
}

int BcmCinter::bcmFieldQualifyIp6(
    int unit,
    bcm_field_entry_t entry,
    bcm_ip6_t data,
    bcm_ip6_t mask,
    Dir dir) {
  string cintV6, v6Var, cintMask, v6Mask;
  tie(cintV6, v6Var) = cintForIp6(data);
  tie(cintMask, v6Mask) = cintForIp6(mask);
  vector<string> cint = {cintV6, cintMask};
  auto cintForFn = wrapFunc(to<string>(
      dir == Dir::SRC ? "bcm_field_qualify_SrcIp6" : "bcm_field_qualify_DstIp6",
      "(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), v6Var, v6Mask),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_field_qualify_L4DstPort(
    int unit,
    bcm_field_entry_t entry,
    bcm_l4_port_t data,
    bcm_l4_port_t mask) {
  return bcmFieldQualifyL4Port(unit, entry, data, mask, Dir::DST);
}

int BcmCinter::bcm_field_qualify_L4SrcPort(
    int unit,
    bcm_field_entry_t entry,
    bcm_l4_port_t data,
    bcm_l4_port_t mask) {
  return bcmFieldQualifyL4Port(unit, entry, data, mask, Dir::SRC);
}

int BcmCinter::bcmFieldQualifyL4Port(
    int unit,
    bcm_field_entry_t entry,
    bcm_l4_port_t data,
    bcm_l4_port_t mask,
    Dir dir) {
  writeCintLines(wrapFunc(to<string>(
      dir == Dir::SRC ? "bcm_field_qualify_L4SrcPort"
                      : "bcm_field_qualify_L4DstPort",
      "(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_DstMac(
    int unit,
    bcm_field_entry_t entry,
    bcm_mac_t mac,
    bcm_mac_t macMask) {
  return bcmFieldQualifyMac(unit, entry, mac, macMask, Dir::DST);
}

int BcmCinter::bcm_field_qualify_SrcMac(
    int unit,
    bcm_field_entry_t entry,
    bcm_mac_t mac,
    bcm_mac_t macMask) {
  return bcmFieldQualifyMac(unit, entry, mac, macMask, Dir::SRC);
}

int BcmCinter::bcmFieldQualifyMac(
    int unit,
    bcm_field_entry_t entry,
    bcm_mac_t mac,
    bcm_mac_t macMask,
    Dir dir) {
  string cintMac, macVar, cintMask, macMaskVar;
  tie(cintMac, macVar) = cintForMac(mac);
  tie(cintMask, macMaskVar) = cintForMac(macMask);
  vector<string> cint = {cintMac, cintMask};
  auto cintForFn = wrapFunc(to<string>(
      dir == Dir::SRC ? "bcm_field_qualify_SrcMac" : "bcm_field_qualify_DstMac",
      "(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), macVar, macMaskVar),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_field_qualify_DstClassL2(
    int unit,
    bcm_field_entry_t entry,
    uint32 data,
    uint32 mask) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_DstClassL2",
      "(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_DstClassL3(
    int unit,
    bcm_field_entry_t entry,
    uint32 data,
    uint32 mask) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_DstClassL3",
      "(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_Ttl(
    int unit,
    bcm_field_entry_t entry,
    uint8 data,
    uint8 mask) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_Ttl(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_IpType(
    int unit,
    bcm_field_entry_t entry,
    bcm_field_IpType_t type) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_IpType(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), type),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_IpProtocol(
    int unit,
    bcm_field_entry_t entry,
    uint8 data,
    uint8 mask) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_IpProtocol(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_DSCP(
    int unit,
    bcm_field_entry_t entry,
    uint8 data,
    uint8 mask) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_DSCP(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_TcpControl(
    int unit,
    bcm_field_entry_t entry,
    uint8 data,
    uint8 mask) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_TcpControl(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_SrcPort(
    int unit,
    bcm_field_entry_t entry,
    bcm_module_t data_modid,
    bcm_module_t mask_modid,
    bcm_port_t data_port,
    bcm_port_t mask_port) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_SrcPort(",
      makeParamStr(
          unit,
          getCintVar(fieldEntryVars, entry),
          data_modid,
          mask_modid,
          data_port,
          mask_port),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_DstPort(
    int unit,
    bcm_field_entry_t entry,
    bcm_module_t data_modid,
    bcm_module_t mask_modid,
    bcm_port_t data_port,
    bcm_port_t mask_port) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_DstPort(",
      makeParamStr(
          unit,
          getCintVar(fieldEntryVars, entry),
          data_modid,
          mask_modid,
          data_port,
          mask_port),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_IpFrag(
    int unit,
    bcm_field_entry_t entry,
    bcm_field_IpFrag_t frag_info) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_IpFrag(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), frag_info),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_qualify_IcmpTypeCode(
    int unit,
    bcm_field_entry_t entry,
    uint16 data,
    uint16 mask) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_IcmpTypeCode(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), data, mask),
      ")")));
  return 0;
};

int BcmCinter::bcm_field_qualify_RangeCheck(
    int unit,
    bcm_field_entry_t entry,
    bcm_field_range_t range,
    int invert) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_qualify_RangeCheck(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), range, invert),
      ")")));
  return 0;
}

int BcmCinter::bcm_field_entry_destroy(int unit, bcm_field_entry_t entry) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_field_entry_destroy(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry)),
      ")")));
  { fieldEntryVars.wlock()->erase(entry); }
  return 0;
}

vector<string> BcmCinter::cintForTrunkInfo(const bcm_trunk_info_t& trunk_info) {
  vector<string> cintLines = {
      "bcm_trunk_info_t_init(&trunk_info)",

      to<string>("trunk_info.psc=", trunk_info.psc),
      to<string>("trunk_info.dlf_index=", trunk_info.dlf_index),
      to<string>("trunk_info.mc_index=", trunk_info.mc_index),
      to<string>("trunk_info.ipmc_index=", trunk_info.ipmc_index)};
  return cintLines;
}

vector<string> BcmCinter::cintForTrunkMember(
    const bcm_trunk_member_t& trunk_member) {
  vector<string> cintLines = {
      "bcm_trunk_member_t_init(&trunk_member)",
      to<string>("trunk_member.flags=", trunk_member.flags),
      to<string>("trunk_member.gport=", trunk_member.gport)};
  return cintLines;
}

pair<string, vector<string>> BcmCinter::cintForTrunkMemberArray(
    const bcm_trunk_member_t* trunkMemberArray,
    int member_max) {
  vector<string> cintLines;
  auto memberArrayName = getNextTmpTrunkMemberArray();
  cintLines.push_back(to<string>(
      "bcm_trunk_member_t ", memberArrayName, "[BCM_TRUNK_MAX_PORTCNT]"));
  for (int i = 0; i < member_max; ++i) {
    cintLines.push_back(
        to<string>("bcm_trunk_member_t_init(&", memberArrayName, "[", i, "])"));
    cintLines.push_back(to<string>(
        memberArrayName, "[", i, "].flags=", trunkMemberArray[i].flags));
    cintLines.push_back(to<string>(
        memberArrayName, "[", i, "].gport=", trunkMemberArray[i].gport));
  }
  memberArrayName = member_max > 0 ? memberArrayName : "NULL";
  return make_pair(memberArrayName, cintLines);
}

int BcmCinter::bcm_trunk_chip_info_get(
    int unit,
    bcm_trunk_chip_info_t* /* ta_info */) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_trunk_chip_info_get(", makeParamStr(unit, "&ta_info"), ")")));
  return 0;
}

int BcmCinter::bcm_trunk_init(int unit) {
  writeCintLines(
      wrapFunc(to<string>("bcm_trunk_init(", makeParamStr(unit), ")")));
  return 0;
}

int BcmCinter::bcm_trunk_destroy(int unit, bcm_trunk_t tid) {
  writeCintLines(
      wrapFunc(to<string>("bcm_trunk_destroy(", makeParamStr(unit, tid), ")")));
  return 0;
}

int BcmCinter::bcm_trunk_create(
    int unit,
    uint32 flags,
    bcm_trunk_t* /* tid */) {
  writeCintLines(wrapFunc(
      to<string>("bcm_trunk_create(", makeParamStr(unit, flags, "&tid"), ")")));
  return 0;
}

int BcmCinter::bcm_trunk_member_add(
    int unit,
    bcm_trunk_t tid,
    bcm_trunk_member_t* trunk_member) {
  auto cint = cintForTrunkMember(*trunk_member);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_trunk_member_add(", makeParamStr(unit, tid, "&trunk_member"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_trunk_member_delete(
    int unit,
    bcm_trunk_t tid,
    bcm_trunk_member_t* member) {
  auto cint = cintForTrunkMember(*member);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_trunk_member_delete(",
      makeParamStr(unit, tid, "&trunk_member"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_trunk_set(
    int unit,
    bcm_trunk_t tid,
    bcm_trunk_info_t* trunk_info,
    int member_max,
    bcm_trunk_member_t* member_array) {
  auto cintTrunkInfo = cintForTrunkInfo(*trunk_info);
  string trunkMemberVar = "NULL";
  vector<string> cintTrunkMemberArray;
  if (member_max > 0) {
    tie(trunkMemberVar, cintTrunkMemberArray) =
        cintForTrunkMemberArray(member_array, member_max);
  }
  auto cintForFn = wrapFunc(to<string>(
      "bcm_trunk_set(",
      makeParamStr(unit, tid, "&trunk_info", member_max, trunkMemberVar),
      ")"));
  cintTrunkInfo.insert(
      cintTrunkInfo.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  cintTrunkMemberArray.insert(
      cintTrunkMemberArray.end(),
      make_move_iterator(cintTrunkInfo.begin()),
      make_move_iterator(cintTrunkInfo.end()));
  writeCintLines(std::move(cintTrunkMemberArray));
  return 0;
}

int BcmCinter::bcm_port_loopback_set(int unit, bcm_port_t port, uint32 value) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_loopback_set(", makeParamStr(unit, port, value), ")")));
  return 0;
}

int BcmCinter::bcm_mirror_init(int unit) {
  writeCintLines(
      wrapFunc(to<string>("bcm_mirror_init(", makeParamStr(unit), ")")));
  return 0;
}

int BcmCinter::bcm_mirror_mode_set(int unit, int mode) {
  writeCintLines(wrapFunc(
      to<string>("bcm_mirror_mode_set(", makeParamStr(unit, mode), ")")));
  return 0;
}

int BcmCinter::bcm_mirror_destination_create(
    int unit,
    bcm_mirror_destination_t* mirror_dest) {
  auto cint = cintForMirrorDestination(mirror_dest);
  auto cintForFn = wrapFunc(to<string>(to<string>(
      "bcm_mirror_destination_create(",
      makeParamStr(unit, "&mirror_dest"),
      ")")));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  string destIdVar = getNextMirrorDestIdVar();
  mirrorDestIdVars.wlock()->emplace(mirror_dest->mirror_dest_id, destIdVar);
  cint.push_back(
      to<string>("bcm_gport_t ", destIdVar, " = mirror_dest.mirror_dest_id"));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_mirror_destination_destroy(
    int unit,
    bcm_gport_t mirror_dest_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_mirror_destination_destroy(",
      makeParamStr(unit, getCintVar(mirrorDestIdVars, mirror_dest_id)),
      ")")));
  return 0;
}

int BcmCinter::bcm_mirror_port_dest_add(
    int unit,
    bcm_port_t port,
    uint32 flags,
    bcm_gport_t mirror_dest_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_mirror_port_dest_add(",
      makeParamStr(
          unit, port, flags, getCintVar(mirrorDestIdVars, mirror_dest_id)),
      ")")));
  return 0;
}

int BcmCinter::bcm_mirror_port_dest_delete(
    int unit,
    bcm_port_t port,
    uint32 flags,
    bcm_gport_t mirror_dest_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_mirror_port_dest_delete(",
      makeParamStr(
          unit, port, flags, getCintVar(mirrorDestIdVars, mirror_dest_id)),
      ")")));
  return 0;
}

int BcmCinter::bcm_l3_egress_ecmp_add(
    int unit,
    bcm_l3_egress_ecmp_t* ecmp,
    bcm_if_t intf) {
  auto cint = cintForL3Ecmp(*ecmp);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_l3_egress_ecmp_add(",
      makeParamStr(unit, "&ecmp", getCintVar(l3IntfIdVars, intf)),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_egress_ecmp_delete(
    int unit,
    bcm_l3_egress_ecmp_t* ecmp,
    bcm_if_t intf) {
  auto cint = cintForL3Ecmp(*ecmp);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_l3_egress_ecmp_delete(",
      makeParamStr(unit, "&ecmp", getCintVar(l3IntfIdVars, intf)),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_mpls_init(int unit) {
  writeCintLines(
      wrapFunc(to<string>("bcm_mpls_init(", makeParamStr(unit), ")")));
  return 0;
}

int BcmCinter::bcm_mpls_tunnel_switch_add(
    int unit,
    bcm_mpls_tunnel_switch_t* info) {
  auto cint = cintForMplsTunnelSwitch(*info);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_mpls_tunnel_switch_add(", makeParamStr(unit, "&switch_info"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_mpls_tunnel_switch_delete(
    int unit,
    bcm_mpls_tunnel_switch_t* info) {
  auto cint = cintForMplsTunnelSwitch(*info);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_mpls_tunnel_switch_delete(",
      makeParamStr(unit, "&switch_info"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_mpls_tunnel_initiator_set(
    int unit,
    bcm_if_t intf,
    int num_labels,
    bcm_mpls_egress_label_t* label_array) {
  auto cint = cintForLabelsArray(num_labels, label_array);
  cint.push_back(to<string>(
      "bcm_mpls_tunnel_initiator_set(",
      makeParamStr(
          unit, getCintVar(l3IntfIdVars, intf), num_labels, "label_array)")));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_mpls_tunnel_initiator_clear(int unit, bcm_if_t intf) {
  writeCintLine(to<string>(
      "bcm_mpls_tunnel_initiator_clear(",
      makeParamStr(unit, getCintVar(l3IntfIdVars, intf)),
      ")"));
  return 0;
}

int BcmCinter::bcm_l3_route_delete_by_interface(
    int unit,
    bcm_l3_route_t* l3_route) {
  auto cint = cintForL3Route(*l3_route);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_l3_route_delete_by_interface(",
      makeParamStr(unit, "&l3_route"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_route_delete(int unit, bcm_l3_route_t* l3_route) {
  auto cint = cintForL3Route(*l3_route);
  auto cintForFn = wrapFunc(
      to<string>("bcm_l3_route_delete(", makeParamStr(unit, "&l3_route"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_route_add(int unit, bcm_l3_route_t* l3_route) {
  auto cint = cintForL3Route(*l3_route);
  auto cintForFn = wrapFunc(
      to<string>("bcm_l3_route_add(", makeParamStr(unit, "&l3_route"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_host_delete_by_interface(
    int unit,
    bcm_l3_host_t* l3_host) {
  auto cint = cintForL3Host(*l3_host);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_l3_host_delete_by_interface(", makeParamStr(unit, "&l3_host"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_host_add(int unit, bcm_l3_host_t* l3_host) {
  auto cint = cintForL3Host(*l3_host);
  auto cintForFn = wrapFunc(
      to<string>("bcm_l3_host_add(", makeParamStr(unit, "&l3_host"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_host_delete(int unit, bcm_l3_host_t* l3_host) {
  auto cint = cintForL3Host(*l3_host);
  auto cintForFn = wrapFunc(
      to<string>("bcm_l3_host_delete(", makeParamStr(unit, "&l3_host"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_stat_custom_add(
    int unit,
    bcm_port_t port,
    bcm_stat_val_t type,
    bcm_custom_stat_trigger_t trigger) {
  auto cint = wrapFunc(to<string>(
      "bcm_stat_custom_add(", makeParamStr(unit, port, type, trigger), ")"));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_intf_create(int unit, bcm_l3_intf_t* l3_intf) {
  auto cint = cintForL3Intf(*l3_intf);
  auto cintForFn = wrapFunc(
      to<string>("bcm_l3_intf_create(", makeParamStr(unit, "&l3_intf"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_intf_delete(int unit, bcm_l3_intf_t* l3_intf) {
  auto cint = cintForL3Intf(*l3_intf);
  auto cintForFn = wrapFunc(
      to<string>("bcm_l3_intf_delete(", makeParamStr(unit, "&l3_intf"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

vector<string> BcmCinter::cintForL3Egress(const bcm_l3_egress_t* l3_egress) {
  vector<string> cintLines;
  string cintMac, macVar;
  tie(cintMac, macVar) = cintForMac(l3_egress->mac_addr);
  cintLines = {
      cintMac,
      "bcm_l3_egress_t_init(&l3_egress)",
      to<string>("l3_egress.flags = ", l3_egress->flags),
      to<string>("l3_egress.mac_addr = ", macVar),
      to<string>(
          "l3_egress.intf = ", getCintVar(l3IntfIdVars, l3_egress->intf)),
      to<string>("l3_egress.vlan = ", l3_egress->vlan),
      to<string>("l3_egress.module = ", l3_egress->module),
      to<string>("l3_egress.port = ", l3_egress->port),
      to<string>("l3_egress.trunk = ", l3_egress->trunk),
      to<string>("l3_egress.qos_map_id = ", l3_egress->qos_map_id),
      to<string>("l3_egress.mpls_qos_map_id = ", l3_egress->mpls_qos_map_id),
      to<string>("l3_egress.mpls_flags = ", l3_egress->mpls_flags),
      to<string>("l3_egress.mpls_label = ", l3_egress->mpls_label)};
  return cintLines;
}

int BcmCinter::bcm_l3_egress_create(
    int unit,
    uint32 flags,
    bcm_l3_egress_t* egr,
    bcm_if_t* if_id) {
  vector<string> cint =
      cintForL3Egress(reinterpret_cast<bcm_l3_egress_t*>(egr));
  if ((flags & BCM_L3_REPLACE) || (flags & BCM_L3_WITH_ID)) {
    auto funcCint = wrapFunc(to<string>(
        "bcm_l3_egress_create(",
        makeParamStr(
            unit,
            flags,
            "&l3_egress",
            to<string>("&", getCintVar(l3IntfIdVars, *if_id))),
        ")"));
    cint.insert(
        cint.end(),
        make_move_iterator(funcCint.begin()),
        make_move_iterator(funcCint.end()));
  } else {
    auto intfVar = getNextL3IntfIdVar();
    {
      l3IntfIdVars.wlock()->emplace(
          reinterpret_cast<bcm_if_t&>(*if_id), intfVar);
    }
    cint.push_back(to<string>("bcm_if_t ", intfVar));
    auto funcCint = wrapFunc(to<string>(
        "bcm_l3_egress_create(",
        makeParamStr(unit, flags, "&l3_egress", to<string>("&", intfVar)),
        ")"));
    cint.insert(
        cint.end(),
        make_move_iterator(funcCint.begin()),
        make_move_iterator(funcCint.end()));
  }
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_vlan_create(int unit, bcm_vlan_t vid) {
  writeCintLines(
      wrapFunc(to<string>("bcm_vlan_create(", makeParamStr(unit, vid), ")")));
  return 0;
}

pair<string, vector<string>> BcmCinter::cintForPortBitmap(bcm_pbmp_t pbmp) {
  auto pbmpVar = getNextTmpPortBitmapVar();
  vector<string> pbmpCint;
  for (int i = 0; i < _SHR_PBMP_WORD_MAX; i++) {
    pbmpCint.push_back(to<string>("pbmp.pbits[", i, "] = ", pbmp.pbits[i]));
  }
  pbmpCint.push_back(to<string>("bcm_pbmp_t ", pbmpVar, " = (auto) pbmp"));
  return make_pair(pbmpVar, pbmpCint);
}

int BcmCinter::bcm_field_qualify_InPorts(
    int unit,
    bcm_field_entry_t entry,
    bcm_pbmp_t data,
    bcm_pbmp_t mask) {
  string pbmp, pbmpMask;
  vector<string> pbmpCint, pbmpMaskCint;
  tie(pbmp, pbmpCint) = cintForPortBitmap(data);
  tie(pbmpMask, pbmpMaskCint) = cintForPortBitmap(mask);
  auto funcCint = wrapFunc(to<string>(
      "bcm_field_qualify_InPorts(",
      makeParamStr(unit, getCintVar(fieldEntryVars, entry), pbmp, pbmpMask),
      ")"));
  vector<string> cint{};
  cint.insert(
      cint.end(),
      make_move_iterator(pbmpCint.begin()),
      make_move_iterator(pbmpCint.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(pbmpMaskCint.begin()),
      make_move_iterator(pbmpMaskCint.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_vlan_port_add(
    int unit,
    bcm_vlan_t vid,
    bcm_pbmp_t pbmp,
    bcm_pbmp_t ubmp) {
  string pbmpVar, ubmpVar;
  vector<string> pbmpCint, ubmpCint;
  tie(pbmpVar, pbmpCint) = cintForPortBitmap(pbmp);
  tie(ubmpVar, ubmpCint) = cintForPortBitmap(ubmp);
  auto funcCint = wrapFunc(to<string>(
      "bcm_vlan_port_add(", makeParamStr(unit, vid, pbmpVar, ubmpVar), ")"));
  vector<string> cint{};
  cint.insert(
      cint.end(),
      make_move_iterator(pbmpCint.begin()),
      make_move_iterator(pbmpCint.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(ubmpCint.begin()),
      make_move_iterator(ubmpCint.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_vlan_port_remove(int unit, bcm_vlan_t vid, bcm_pbmp_t pbmp) {
  string pbmpVar;
  vector<string> pbmpCint;
  tie(pbmpVar, pbmpCint) = cintForPortBitmap(pbmp);
  auto funcCint = wrapFunc(to<string>(
      "bcm_vlan_port_remove(", makeParamStr(unit, vid, pbmpVar), ")"));
  vector<string> cint{};
  cint.insert(
      cint.end(),
      make_move_iterator(pbmpCint.begin()),
      make_move_iterator(pbmpCint.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_port_vlan_member_set(
    int unit,
    bcm_port_t port,
    uint32 flags) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_vlan_member_set(", makeParamStr(unit, port, flags), ")")));
  return 0;
}

int BcmCinter::bcm_port_untagged_vlan_set(
    int unit,
    bcm_port_t port,
    bcm_vlan_t vid) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_untagged_vlan_set(", makeParamStr(unit, port, vid), ")")));
  return 0;
}

int BcmCinter::bcm_port_enable_set(int unit, bcm_port_t port, int enable) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_enable_set(", makeParamStr(unit, port, enable), ")")));
  return 0;
}

int BcmCinter::bcm_port_control_set(
    int unit,
    bcm_port_t port,
    bcm_port_control_t type,
    int value) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_control_set(", makeParamStr(unit, port, type, value), ")")));
  return 0;
}

int BcmCinter::bcm_port_interface_set(
    int unit,
    bcm_port_t port,
    bcm_port_if_t intf) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_interface_set(", makeParamStr(unit, port, intf), ")")));
  return 0;
}

int BcmCinter::bcm_l3_egress_destroy(int unit, bcm_if_t intf) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_l3_egress_destroy(",
      makeParamStr(unit, getCintVar(l3IntfIdVars, intf)),
      ")")));
  { l3IntfIdVars.wlock()->erase(intf); }
  return 0;
}

int BcmCinter::bcm_qos_map_create(int unit, uint32 flags, int* map_id) {
  string qosMapVar = getNextQosMapVar();
  { qosMapIdVars.wlock()->emplace(*map_id, qosMapVar); }
  vector<string> cint = {to<string>("int ", qosMapVar)};
  auto funcCint = wrapFunc(to<string>(
      "bcm_qos_map_create(",
      makeParamStr(unit, flags, to<string>("&", qosMapVar)),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_qos_map_destroy(int unit, int map_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_qos_map_destroy(",
      makeParamStr(unit, getCintVar(qosMapIdVars, map_id)),
      ")")));
  { qosMapIdVars.wlock()->erase(map_id); }
  return 0;
}

vector<std::string> BcmCinter::cintForQosMapEntry(
    uint32 flags,
    const bcm_qos_map_t* qosMapEntry) const {
  if (flags & BCM_QOS_MAP_L3) {
    return {
        "bcm_qos_map_t_init(&qos_map_entry)",
        to<string>("qos_map_entry.dscp = ", qosMapEntry->dscp),
        to<string>("qos_map_entry.int_pri = ", qosMapEntry->int_pri),
    };
  } else {
    return {
        "bcm_qos_map_t_init(&qos_map_entry)",
        to<string>("qos_map_entry.exp = ", qosMapEntry->exp),
        to<string>("qos_map_entry.int_pri = ", qosMapEntry->int_pri),
    };
  }
}

int BcmCinter::bcm_qos_map_add(
    int unit,
    uint32 flags,
    bcm_qos_map_t* map,
    int map_id) {
  auto cint = cintForQosMapEntry(flags, map);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_qos_map_add(",
      makeParamStr(
          unit, flags, "&qos_map_entry", getCintVar(qosMapIdVars, map_id)),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_qos_map_delete(
    int unit,
    uint32 flags,
    bcm_qos_map_t* map,
    int map_id) {
  auto cint = cintForQosMapEntry(flags, map);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_qos_map_delete(",
      makeParamStr(
          unit, flags, "&qos_map_entry", getCintVar(qosMapIdVars, map_id)),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_qos_port_map_set(
    int unit,
    bcm_gport_t gport,
    int ing_map,
    int egr_map) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_qos_port_map_set(",
      makeParamStr(
          unit,
          gport,
          getCintVar(qosMapIdVars, ing_map),
          getCintVar(qosMapIdVars, egr_map)),
      ")")));
  return 0;
}

int BcmCinter::bcm_port_dscp_map_mode_set(int unit, bcm_port_t port, int mode) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_dscp_map_mode_set(", makeParamStr(unit, port, mode), ")")));
  return 0;
}

int BcmCinter::bcm_l3_egress_ecmp_create(
    int unit,
    bcm_l3_egress_ecmp_t* ecmp,
    int intf_count,
    bcm_if_t* intf_array) {
  auto cint =
      cintForL3Ecmp(reinterpret_cast<const bcm_l3_egress_ecmp_t&>(*ecmp));
  auto cintForPaths = cintForPathsArray(intf_array, intf_count);
  auto cintForFn = wrapFunc(to<string>(
      "bcm_l3_egress_ecmp_create(",
      makeParamStr(unit, "&ecmp", intf_count, "&intf_array[0]"),
      ")"));
  auto l3IntfIdVar = getNextL3IntfIdVar();
  { l3IntfIdVars.wlock()->emplace(ecmp->ecmp_intf, l3IntfIdVar); }
  cint.push_back(to<string>("bcm_if_t ", l3IntfIdVar, " = ", ecmp->ecmp_intf));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForPaths.begin()),
      make_move_iterator(cintForPaths.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForFn.begin()),
      make_move_iterator(cintForFn.end()));
  writeCintLines(std::move(cint));
  return 0;
}

std::vector<std::string> BcmCinter::cintForPathsArray(
    const bcm_if_t* paths,
    int path_count) {
  vector<string> cintLines;
  for (auto i = 0; i < path_count; i++) {
    cintLines.push_back(
        to<string>("intf_array[", i, "]=", getCintVar(l3IntfIdVars, paths[i])));
  }
  return cintLines;
}

std::pair<std::string, std::vector<std::string>> BcmCinter::cintForReasons(
    bcm_rx_reasons_t reasons) {
  auto reasonsVar = getNextTmpReasonsVar();
  vector<string> reasonsCint = {};
  for (int i = 0; i < sizeof(_shr_rx_reasons_t) / sizeof(SHR_BITDCL); i++) {
    reasonsCint.push_back(
        to<string>("reasons.pbits[", i, "] = ", reasons.pbits[i]));
  }
  reasonsCint.push_back(
      to<string>("bcm_rx_reasons_t ", reasonsVar, " = (auto) reasons"));
  return make_pair(reasonsVar, reasonsCint);
}

int BcmCinter::bcm_rx_cosq_mapping_set(
    int unit,
    int index,
    bcm_rx_reasons_t reasons,
    bcm_rx_reasons_t reasons_mask,
    uint8 int_prio,
    uint8 int_prio_mask,
    uint32 packet_type,
    uint32 packet_type_mask,
    bcm_cos_queue_t cosq) {
  string reasonsVar, reasonsMaskVar;
  vector<string> cint, reasonsCint, reasonsMaskCint;
  tie(reasonsVar, reasonsCint) = cintForReasons(reasons);
  tie(reasonsMaskVar, reasonsMaskCint) = cintForReasons(reasons_mask);
  auto funcCint = wrapFunc(to<string>(
      "bcm_rx_cosq_mapping_set(",
      makeParamStr(
          unit,
          index,
          reasonsVar,
          reasonsMaskVar,
          int_prio,
          int_prio_mask,
          packet_type,
          packet_type_mask,
          cosq),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(reasonsCint.begin()),
      make_move_iterator(reasonsCint.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(reasonsMaskCint.begin()),
      make_move_iterator(reasonsMaskCint.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_rx_cosq_mapping_delete(int unit, int index) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_rx_cosq_mapping_delete(", makeParamStr(unit, index), ")")));
  return 0;
}

int BcmCinter::bcm_pkt_alloc(
    int /*unit*/,
    int /*size*/,
    uint32 /*flags*/,
    bcm_pkt_t** /*pkt_buf*/) {
  return 0;
}

int BcmCinter::bcm_tx(int unit, bcm_pkt_t* tx_pkt, void* /*cookie*/) {
  if (!FLAGS_gen_tx_cint) {
    return 0;
  }
  vector<string> cint{};
  string pbmpVar, ubmpVar;
  vector<string> pbmpCint, ubmpCint;

  auto allocFuncCint = wrapFunc(to<string>(
      "bcm_pkt_alloc(",
      makeParamStr(unit, tx_pkt->pkt_data->len, tx_pkt->flags, "&pkt"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(allocFuncCint.begin()),
      make_move_iterator(allocFuncCint.end()));

  tie(pbmpVar, pbmpCint) = cintForPortBitmap(tx_pkt->tx_pbmp);
  cint.insert(
      cint.end(),
      make_move_iterator(pbmpCint.begin()),
      make_move_iterator(pbmpCint.end()));
  cint.push_back(to<string>("pkt->tx_pbmp = ", pbmpVar));

  tie(ubmpVar, ubmpCint) = cintForPortBitmap(tx_pkt->tx_upbmp);
  cint.insert(
      cint.end(),
      make_move_iterator(ubmpCint.begin()),
      make_move_iterator(ubmpCint.end()));
  cint.push_back(to<string>("pkt->tx_upbmp = ", ubmpVar));

  for (int i = 0; i < tx_pkt->pkt_data->len; i++) {
    cint.push_back(to<string>(
        "pkt->pkt_data->data[", i, "] = ", tx_pkt->pkt_data->data[i]));
  }

  auto txFuncCint =
      wrapFunc(to<string>("bcm_tx(", makeParamStr(unit, "pkt", "NULL"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(txFuncCint.begin()),
      make_move_iterator(txFuncCint.end()));

  auto freeFuncCint =
      wrapFunc(to<string>("bcm_pkt_free(", makeParamStr(unit, "pkt"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(freeFuncCint.begin()),
      make_move_iterator(freeFuncCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_pkt_free(int /*unit*/, bcm_pkt_t* /*pkt*/) {
  return 0;
}

int BcmCinter::bcm_port_stat_enable_set(
    int unit,
    bcm_gport_t port,
    int enable) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_stat_enable_set(", makeParamStr(unit, port, enable), ")")));
  return 0;
}

int BcmCinter::bcm_stat_clear(int unit, bcm_port_t port) {
  writeCintLines(
      wrapFunc(to<string>("bcm_stat_clear(", makeParamStr(unit, port), ")")));
  return 0;
}

int BcmCinter::bcm_cosq_bst_stat_clear(
    int unit,
    bcm_gport_t gport,
    bcm_cos_queue_t cosq,
    bcm_bst_stat_id_t bid) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_cosq_bst_stat_clear(", makeParamStr(unit, gport, cosq, bid), ")")));
  return 0;
}

int BcmCinter::bcm_vlan_control_port_set(
    int unit,
    int port,
    bcm_vlan_control_port_t type,
    int arg) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_vlan_control_port_set(", makeParamStr(unit, port, type, arg), ")")));
  return 0;
}

int BcmCinter::bcm_cosq_mapping_set(
    int unit,
    bcm_cos_t priority,
    bcm_cos_queue_t cosq) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_cosq_mapping_set(", makeParamStr(unit, priority, cosq), ")")));
  return 0;
}

int BcmCinter::bcm_switch_control_set(
    int unit,
    bcm_switch_control_t type,
    int arg) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_switch_control_set(", makeParamStr(unit, type, arg), ")")));
  return 0;
}

int BcmCinter::bcm_l2_age_timer_set(int unit, int age_seconds) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_l2_age_timer_set(", makeParamStr(unit, age_seconds), ")")));
  return 0;
}

int BcmCinter::bcm_port_pause_set(
    int unit,
    bcm_port_t port,
    int pause_tx,
    int pause_rx) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_pause_set(",
      makeParamStr(unit, port, pause_tx, pause_rx),
      ")")));
  return 0;
}

int BcmCinter::bcm_port_sample_rate_set(
    int unit,
    bcm_port_t port,
    int ingress_rate,
    int egress_rate) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_sample_rate_set(",
      makeParamStr(unit, port, ingress_rate, egress_rate),
      ")")));
  return 0;
}

vector<string> BcmCinter::cintForL2Station(bcm_l2_station_t* station) {
  string cintMac, macVar, cintMacMask, macMaskVar;
  vector<string> cintLines;
  tie(cintMac, macVar) = cintForMac(station->dst_mac);
  tie(cintMacMask, macMaskVar) = cintForMac(station->dst_mac_mask);
  cintLines.push_back(cintMac);
  cintLines.push_back(cintMacMask);
  vector<string> l2StationCint = {
      "bcm_l2_station_t_init(&station)",
      to<string>("station.flags = ", station->flags),
      to<string>("station.dst_mac = ", macVar),
      to<string>("station.dst_mac_mask = ", macMaskVar),
      to<string>("station.vlan = ", station->vlan),
      to<string>("station.vlan_mask = ", station->vlan_mask),
      to<string>("station.src_port = ", station->src_port),
      to<string>("station.src_port_mask = ", station->src_port_mask),
  };
  cintLines.insert(
      cintLines.end(),
      make_move_iterator(l2StationCint.begin()),
      make_move_iterator(l2StationCint.end()));
  return cintLines;
}

int BcmCinter::bcm_l2_station_add(
    int unit,
    int* station_id,
    bcm_l2_station_t* station) {
  vector<string> cintLines;
  auto l2StationCint = cintForL2Station(station);
  cintLines.insert(
      cintLines.end(),
      make_move_iterator(l2StationCint.begin()),
      make_move_iterator(l2StationCint.end()));
  cintLines.push_back(
      to<string>("station.flags = ", station->flags | BCM_L2_STATION_WITH_ID));
  cintLines.push_back(to<string>("station_id = ", *station_id));
  auto funcCint = wrapFunc(to<string>(
      "bcm_l2_station_add(",
      makeParamStr(unit, "&station_id", "&station"),
      ")"));
  cintLines.insert(
      cintLines.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(cintLines);
  return 0;
}

int BcmCinter::bcm_vlan_default_set(int unit, bcm_vlan_t vid) {
  writeCintLines(wrapFunc(
      to<string>("bcm_vlan_default_set(", makeParamStr(unit, vid), ")")));
  return 0;
}

int BcmCinter::bcm_port_phy_tx_set(
    int unit,
    bcm_port_t port,
    bcm_port_phy_tx_t* tx) {
  vector<string> cintLines;
  cintLines.push_back(to<string>("bcm_port_phy_tx_t_init(&tx)"));
  cintLines.push_back(to<string>("tx.pre2 = ", tx->pre2));
  cintLines.push_back(to<string>("tx.pre = ", tx->pre));
  cintLines.push_back(to<string>("tx.main = ", tx->main));
  cintLines.push_back(to<string>("tx.post = ", tx->post));
  cintLines.push_back(to<string>("tx.post2 = ", tx->post2));
  cintLines.push_back(to<string>("tx.post3 = ", tx->post3));
  cintLines.push_back(to<string>("tx.tx_tap_mode = ", tx->tx_tap_mode));
  cintLines.push_back(to<string>("tx.signalling_mode = ", tx->signalling_mode));

  auto funcCint = wrapFunc(
      to<string>("bcm_port_phy_tx_set(", makeParamStr(unit, port, "&tx"), ")"));
  cintLines.insert(
      cintLines.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(cintLines);
  return 0;
}

int BcmCinter::bcm_port_phy_tx_get(
    int unit,
    bcm_port_t port,
    [[maybe_unused]] bcm_port_phy_tx_t* tx) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_phy_tx_get(", makeParamStr(unit, port, "&tx"), ")")));
  return 0;
}

int BcmCinter::bcm_port_phy_control_set(
    int unit,
    bcm_port_t port,
    bcm_port_phy_control_t type,
    uint32 value) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_phy_control_set(",
      makeParamStr(unit, port, type, value),
      ")")));
  return 0;
}

int BcmCinter::bcm_port_speed_set(int unit, bcm_port_t port, int speed) {
  writeCintLines(wrapFunc(
      to<string>("bcm_port_speed_set(", makeParamStr(unit, port, speed), ")")));
  return 0;
}

int BcmCinter::bcm_l3_egress_ecmp_destroy(
    int unit,
    bcm_l3_egress_ecmp_t* ecmp) {
  vector<string> cint =
      cintForL3Ecmp(reinterpret_cast<bcm_l3_egress_ecmp_t&>(*ecmp));
  auto funcCint = wrapFunc(to<string>(
      "bcm_l3_egress_ecmp_destroy(", makeParamStr(unit, "&ecmp"), ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  { l3IntfIdVars.wlock()->erase(ecmp->ecmp_intf); }
  return 0;
}

int BcmCinter::bcm_stg_stp_set(
    int unit,
    bcm_stg_t stg,
    bcm_port_t port,
    int stp_state) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_stg_stp_set(", makeParamStr(unit, stg, port, stp_state), ")")));
  return 0;
}

int BcmCinter::bcm_stg_vlan_add(int unit, bcm_stg_t stg, bcm_vlan_t vid) {
  writeCintLines(wrapFunc(
      to<string>("bcm_stg_vlan_add(", makeParamStr(unit, stg, vid), ")")));
  return 0;
}

int BcmCinter::bcm_port_learn_set(int unit, bcm_port_t port, uint32 flags) {
  writeCintLines(wrapFunc(
      to<string>("bcm_port_learn_set(", makeParamStr(unit, port, flags), ")")));
  return 0;
}

int BcmCinter::bcm_vlan_gport_delete_all(int unit, bcm_vlan_t vlan) {
  writeCintLines(wrapFunc(
      to<string>("bcm_vlan_gport_delete_all(", makeParamStr(unit, vlan), ")")));
  return 0;
}

int BcmCinter::bcm_l3_egress_multipath_create(
    int unit,
    uint32 flags,
    int intf_count,
    bcm_if_t* intf_array,
    bcm_if_t* mpintf) {
  vector<string> cint{};
  auto l3IntfIdVar = getNextL3IntfIdVar();
  {
    l3IntfIdVars.wlock()->emplace(
        reinterpret_cast<bcm_if_t&>(*mpintf), l3IntfIdVar);
  }
  cint.push_back(to<string>("bcm_if_t ", l3IntfIdVar));
  auto cintForPaths = cintForPathsArray(intf_array, intf_count);
  auto funcCint = wrapFunc(to<string>(
      "bcm_l3_egress_multipath_create(",
      makeParamStr(
          unit,
          flags,
          intf_count,
          "&intf_array[0]",
          to<string>("&", l3IntfIdVar)),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(cintForPaths.begin()),
      make_move_iterator(cintForPaths.end()));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_l3_egress_multipath_add(
    int unit,
    bcm_if_t mpintf,
    bcm_if_t intf) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_l3_egress_multipath_add(",
      makeParamStr(
          unit,
          getCintVar(l3IntfIdVars, mpintf),
          getCintVar(l3IntfIdVars, intf)),
      ")")));
  return 0;
}

int BcmCinter::bcm_l3_egress_multipath_delete(
    int unit,
    bcm_if_t mpintf,
    bcm_if_t intf) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_l3_egress_multipath_delete(",
      makeParamStr(
          unit,
          getCintVar(l3IntfIdVars, mpintf),
          getCintVar(l3IntfIdVars, intf)),
      ")")));
  return 0;
}

int BcmCinter::bcm_l3_egress_multipath_destroy(int unit, bcm_if_t mpintf) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_l3_egress_multipath_destroy(",
      makeParamStr(unit, getCintVar(l3IntfIdVars, mpintf)),
      ")")));
  { l3IntfIdVars.wlock()->erase(mpintf); }
  return 0;
}

int BcmCinter::bcm_port_pause_sym_set(int unit, bcm_port_t port, int pause) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_port_pause_sym_set(", makeParamStr(unit, port, pause), ")")));
  return 0;
}

int BcmCinter::bcm_vlan_destroy_all(int unit) {
  writeCintLines(
      wrapFunc(to<string>("bcm_vlan_destroy_all(", makeParamStr(unit), ")")));
  return 0;
}

int BcmCinter::bcm_vlan_destroy(int unit, bcm_vlan_t vid) {
  writeCintLines(
      wrapFunc(to<string>("bcm_vlan_destroy(", makeParamStr(unit, vid), ")")));
  return 0;
}

int BcmCinter::bcm_l2_station_delete(int unit, int station_id) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_l2_station_delete(",
      makeParamStr(unit, getCintVar(l2StationIdVars, station_id)),
      ")")));
  { l2StationIdVars.wlock()->erase(station_id); }
  return 0;
}

vector<string> BcmCinter::cintForCosqBstProfile(
    bcm_cosq_bst_profile_t* profile) {
  return {"bcm_cosq_bst_profile_t_init(&cosq_bst_profile)",
          to<string>("cosq_bst_profile.byte = ", profile->byte)};
}

int BcmCinter::bcm_cosq_bst_profile_set(
    int unit,
    bcm_gport_t gport,
    bcm_cos_queue_t cosq,
    bcm_bst_stat_id_t bid,
    bcm_cosq_bst_profile_t* profile) {
  auto cint = cintForCosqBstProfile(profile);
  auto funcCint = wrapFunc(to<string>(
      "bcm_cosq_bst_profile_set(",
      makeParamStr(unit, gport, cosq, bid, "&cosq_bst_profile"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

// BCM port resource APIs
string BcmCinter::cintForInitPortResource(string name) {
  return to<string>("bcm_port_resource_t_init(&", name, ")");
}

vector<string> BcmCinter::cintForPortResource(
    string name,
    bcm_port_resource_t* resource) {
  return {
      cintForInitPortResource(name),
      to<string>(name, ".flags = ", resource->flags),
      to<string>(name, ".port = ", resource->port),
      to<string>(name, ".physical_port = ", resource->physical_port),
      to<string>(name, ".speed = ", resource->speed),
      to<string>(name, ".lanes = ", resource->lanes),
      to<string>(name, ".encap = ", resource->encap),
      to<string>(name, ".fec_type = ", resource->fec_type),
      to<string>(name, ".phy_lane_config = ", resource->phy_lane_config),
      to<string>(name, ".link_training = ", resource->link_training),
      to<string>(name, ".roe_compression = ", resource->roe_compression),
  };
}

vector<string> BcmCinter::cintForPortResources(
    int nport,
    bcm_port_resource_t* resource) {
  vector<string> output;
  for (int i = 0; i < nport; i++) {
    auto resourceCint =
        cintForPortResource(to<string>("resources[", i, "]"), &resource[i]);
    for (const auto& line : resourceCint) {
      output.push_back(line);
    }
  }
  return output;
}

int BcmCinter::bcm_port_resource_speed_get(
    int unit,
    bcm_gport_t port,
    bcm_port_resource_t* /* resource */) {
  auto cint = std::vector{cintForInitPortResource("resource")};
  auto funcCint = wrapFunc(to<string>(
      "bcm_port_resource_speed_get(",
      makeParamStr(unit, port, "&resource"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_port_resource_speed_set(
    int unit,
    bcm_gport_t port,
    bcm_port_resource_t* resource) {
  auto cint = cintForPortResource("resource", resource);
  auto funcCint = wrapFunc(to<string>(
      "bcm_port_resource_speed_set(",
      makeParamStr(unit, port, "&resource"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_port_resource_multi_set(
    int unit,
    int nport,
    bcm_port_resource_t* resource) {
  auto cint = cintForPortResources(nport, resource);
  auto funcCint = wrapFunc(to<string>(
      "bcm_port_resource_multi_set(",
      makeParamStr(unit, nport, "&resources"),
      ")"));
  cint.insert(
      cint.end(),
      make_move_iterator(funcCint.begin()),
      make_move_iterator(funcCint.end()));
  writeCintLines(std::move(cint));
  return 0;
}

int BcmCinter::bcm_switch_control_port_set(
    int unit,
    bcm_port_t port,
    bcm_switch_control_t type,
    int arg) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_switch_control_port_set(",
      makeParamStr(unit, port, type, arg),
      ")")));
  return 0;
}

int BcmCinter::bcm_l2_addr_delete_by_port(
    int unit,
    bcm_module_t mod,
    bcm_port_t port,
    uint32 flags) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_l2_addr_delete_by_port(",
      makeParamStr(unit, mod, port, flags),
      ")")));
  return 0;
}

int BcmCinter::bcm_linkscan_enable_set(int unit, int us) {
  writeCintLines(wrapFunc(
      to<string>("bcm_linkscan_enable_set(", makeParamStr(unit, us), ")")));
  return 0;
}

int BcmCinter::bcm_linkscan_mode_set(int unit, bcm_port_t port, int mode) {
  writeCintLines(wrapFunc(to<string>(
      "bcm_linkscan_mode_set(", makeParamStr(unit, port, mode), ")")));
  return 0;
}

int BcmCinter::sh_process_command(int unit, char* cmd) {
  auto funcCint =
      wrapFunc(to<string>("sh_process_command(", unit, ", \"", cmd, "\")"));
  vector<string> cintLine = {funcCint};
  writeCintLines(cintLine);
  return 0;
}

} // namespace facebook::fboss
