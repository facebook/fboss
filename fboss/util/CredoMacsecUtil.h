// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/qsfp_service/lib/QsfpClient.h"

namespace facebook {
namespace fboss {

DECLARE_bool(phy_port_map);
DECLARE_bool(add_sa_rx);
DECLARE_string(sa_config);
DECLARE_string(sc_config);
DECLARE_bool(add_sa_tx);
DECLARE_bool(phy_link_info);
DECLARE_bool(phy_serdes_info);
DECLARE_string(port);
DECLARE_bool(delete_sa_rx);
DECLARE_bool(delete_sa_tx);
DECLARE_bool(delete_all_sc);
DECLARE_bool(get_all_sc_info);
DECLARE_bool(get_port_stats);
DECLARE_bool(ingress);
DECLARE_bool(egress);
DECLARE_bool(get_flow_stats);
DECLARE_bool(get_sa_stats);
DECLARE_bool(get_allport_stats);
DECLARE_bool(get_sdk_state);
DECLARE_string(filename);

class CredoMacsecUtil {
 public:
  CredoMacsecUtil() = default;
  ~CredoMacsecUtil() = default;
  bool getMacsecSaFromJson(
      std::string saJsonFile,
      facebook::fboss::mka::MKASak& sak);
  bool getMacsecScFromJson(
      std::string scJsonFile,
      facebook::fboss::mka::MKASci& sci);
  void printPhyPortMap(
      facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void addSaRx(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void addSaTx(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void deleteSaRx(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void deleteSaTx(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void printPhyLinkInfo(
      facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void printPhySerdesInfo(
      facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void getAllScInfo(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void getPortStats(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void getFlowStats(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void getSaStats(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void deleteAllSc(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void getAllPortStats(
      facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);
  void getSdkState(facebook::fboss::QsfpServiceAsyncClient* fbMacsecHandler);

 private:
  // Forbidden copy constructor and assignment operator
  CredoMacsecUtil(CredoMacsecUtil const&) = delete;
  CredoMacsecUtil& operator=(CredoMacsecUtil const&) = delete;
};

} // namespace fboss
} // namespace facebook
