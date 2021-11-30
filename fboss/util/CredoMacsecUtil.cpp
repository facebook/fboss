// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include "fboss/util/CredoMacsecUtil.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

using namespace facebook::fboss;
using namespace facebook::fboss::mka;
using namespace apache::thrift;

namespace {
void printSaStatsHelper(
    MacsecSaStats& saStats,
    std::string portName,
    bool ingress) {
  printf("Printing SA stats for %s\n", portName.c_str());
  printf("Direction: %s\n", ingress ? "ingress" : "egress");
  printf(
      "  SA direction: %s\n",
      *saStats.directionIngress_ref() ? "Ingress" : "Egress");
  printf("  OctetsEncrypted: %ld\n", *saStats.octetsEncrypted_ref());
  printf("  OctetsProtected: %ld\n", *saStats.octetsProtected_ref());

  if (*saStats.directionIngress_ref()) {
    printf("  InUncheckedPacktes: %ld\n", *saStats.inUncheckedPkts_ref());
    printf("  InDelayedPacktes: %ld\n", *saStats.inDelayedPkts_ref());
    printf("  InLatePacktes: %ld\n", *saStats.inLatePkts_ref());
    printf("  InInvalidPacktes: %ld\n", *saStats.inInvalidPkts_ref());
    printf("  InNotValidPacktes: %ld\n", *saStats.inNotValidPkts_ref());
    printf("  InNoSaPacktes: %ld\n", *saStats.inNoSaPkts_ref());
    printf("  InUnusedSaPacktes: %ld\n", *saStats.inUnusedSaPkts_ref());
    printf("  InOkPacktes: %ld\n", *saStats.inOkPkts_ref());
  } else {
    printf("  OutEncryptedPacktes: %ld\n", *saStats.outEncryptedPkts_ref());
    printf("  OutProtectedPacktes: %ld\n", *saStats.outProtectedPkts_ref());
  }
}

void printPortStatsHelper(
    MacsecPortStats& portStats,
    std::string portName,
    bool ingress) {
  printf("Printing Port stats for %s\n", portName.c_str());
  printf("Direction: %s\n", ingress ? "ingress" : "egress");
  printf("  PreMacsecDropPackets: %ld\n", *portStats.preMacsecDropPkts_ref());
  printf("  ControlPackets: %ld\n", *portStats.controlPkts_ref());
  printf("  DataPackets: %ld\n", *portStats.dataPkts_ref());
  if (ingress) {
    printf("  DecryptedOctets: %ld\n", *portStats.octetsEncrypted_ref());
    printf(
        "  inBadOrNoMacsecTagDroppedPkts: %ld\n",
        *portStats.inBadOrNoMacsecTagDroppedPkts_ref());
    printf("  inNoSciDroppedPkts: %ld\n", *portStats.inNoSciDroppedPkts_ref());
    printf("  inUnknownSciPkts: %ld\n", *portStats.inUnknownSciPkts_ref());
    printf(
        "  inOverrunDroppedPkts: %ld\n", *portStats.inOverrunDroppedPkts_ref());
    printf("  inDelayedPkts: %ld\n", *portStats.inDelayedPkts_ref());
    printf("  inLateDroppedPkts: %ld\n", *portStats.inLateDroppedPkts_ref());
    printf(
        "  inNotValidDroppedPkts: %ld\n",
        *portStats.inNotValidDroppedPkts_ref());
    printf("  inInvalidPkts: %ld\n", *portStats.inInvalidPkts_ref());
    printf("  inNoSaDroppedPkts: %ld\n", *portStats.inNoSaDroppedPkts_ref());
    printf("  inUnusedSaPkts: %ld\n", *portStats.inUnusedSaPkts_ref());
  } else {
    printf("  EncryptedOctets: %ld\n", *portStats.octetsEncrypted_ref());
    printf(
        "  outTooLongDroppedPkts: %ld\n",
        *portStats.outTooLongDroppedPkts_ref());
  }
}

void printFlowStatsHelper(
    MacsecFlowStats& flowStats,
    std::string portName,
    bool ingress) {
  printf("Printing Flow stats for %s\n", portName.c_str());
  printf("Direction: %s\n", ingress ? "ingress" : "egress");
  printf(
      "  Flow direction: %s\n",
      *flowStats.directionIngress_ref() ? "Ingress" : "Egress");
  printf(
      "  UnicastUncontrolledPackets: %ld\n",
      *flowStats.ucastUncontrolledPkts_ref());
  printf(
      "  UnicastControlledPackets: %ld\n",
      *flowStats.ucastControlledPkts_ref());
  printf(
      "  MulticastUncontrolledPackets: %ld\n",
      *flowStats.mcastUncontrolledPkts_ref());
  printf(
      "  MulticastControlledPackets: %ld\n",
      *flowStats.mcastControlledPkts_ref());
  printf(
      "  BroadcastUncontrolledPackets: %ld\n",
      *flowStats.bcastUncontrolledPkts_ref());
  printf(
      "  BroadcastControlledPackets: %ld\n",
      *flowStats.bcastControlledPkts_ref());
  printf("  ControlPackets: %ld\n", *flowStats.controlPkts_ref());
  printf("  UntaggedPackets: %ld\n", *flowStats.untaggedPkts_ref());
  printf("  OtherErroredPackets: %ld\n", *flowStats.otherErrPkts_ref());
  printf("  OctetsUncontrolled: %ld\n", *flowStats.octetsUncontrolled_ref());
  printf("  OctetsControlled: %ld\n", *flowStats.octetsControlled_ref());
  if (*flowStats.directionIngress_ref()) {
    printf(
        "  InTaggedControlledPackets: %ld\n",
        *flowStats.inTaggedControlledPkts_ref());
    printf("  InUntaggedPackets: %ld\n", *flowStats.untaggedPkts_ref());
    printf("  InBadTagPackets: %ld\n", *flowStats.inBadTagPkts_ref());
    printf("  NoSciPackets: %ld\n", *flowStats.noSciPkts_ref());
    printf("  UnknownSciPackets: %ld\n", *flowStats.unknownSciPkts_ref());
    printf("  OverrunPkts: %ld\n", *flowStats.overrunPkts_ref());
  } else {
    printf("  OutCommonOctets: %ld\n", *flowStats.outCommonOctets_ref());
    printf("  OutTooLongPackets: %ld\n", *flowStats.outTooLongPkts_ref());
  }
}

} // namespace

namespace facebook {
namespace fboss {

DEFINE_bool(phy_port_map, false, "Print Phy Port map for this platform");
DEFINE_bool(add_sa_rx, false, "Add SA rx, use with --sa_config, --sc_config");
DEFINE_string(sa_config, "", "SA config JSON file");
DEFINE_string(sc_config, "", "SC config JSON file");
DEFINE_bool(add_sa_tx, false, "Add SA tx, use with --sa_config");
DEFINE_bool(
    phy_link_info,
    false,
    "Print link info for a port in a phy, use with --port");
DEFINE_string(port, "", "Switch port");
DEFINE_bool(
    delete_sa_rx,
    false,
    "Delete SA rx, use with --sa_config, --sc_config");
DEFINE_bool(delete_sa_tx, false, "Delete SA tx, use with --sa_config");
DEFINE_bool(
    delete_all_sc,
    false,
    "Delete all SA, SC on a Phy, use with --port");
DEFINE_bool(
    get_all_sc_info,
    false,
    "Get all SC, SA info for a Phy, use with --port");
DEFINE_bool(
    get_port_stats,
    false,
    "Get Port stats, use with --port --ingress/--egress");
DEFINE_bool(ingress, false, "Ingress direction for port/SA/Flow stats");
DEFINE_bool(egress, false, "Egress direction for port/SA/Flow stats");
DEFINE_bool(
    get_flow_stats,
    false,
    "Get flow stats, use with --port --ingress/--egress");
DEFINE_bool(
    get_sa_stats,
    false,
    "Get SA stats, use with --port --ingress/--egress");
DEFINE_bool(
    get_allport_stats,
    false,
    "Get all port stats in this system, optionally use --port");
DEFINE_bool(get_sdk_state, false, "Get entire SAI state, use with --filename");
DEFINE_string(filename, "", "File name");

constexpr bool kReadFromHw = true;
/*
 * getMacsecSaFromJson()
 * Reads MacSec SA information from JSON file to MKASak structure.
 * sa-json-file:
 *  {
 *       "sci": {
 *               "macAddress": "<mac-address>",
 *               "port": <port-id>
 *       },
 *       "l2Port": "<l2-port-string>",
 *       "assocNum": <association-number>,
 *       "keyHex": "<key-hex-value>",
 *       "keyIdHex": "<Key-id-hex-value>",
 *       "primary": <true/false>
 *  }
 */
bool CredoMacsecUtil::getMacsecSaFromJson(std::string saJsonFile, MKASak& sak) {
  bool retVal = false;

  if (std::string saJsonStr; folly::readFile(saJsonFile.c_str(), saJsonStr)) {
    sak = apache::thrift::SimpleJSONSerializer::deserialize<MKASak>(saJsonStr);
    retVal = true;
  } else {
    printf("SA config could not be read\n");
  }

  return retVal;
}

/*
 * getMacsecScFromJson()
 * Reads MacSec SC information from JSON file to MKASci structure.
 * sc-json-file:
 * {
 *       "macAddress": "<mac-address>",
 *       "port": <port-id>
 * }
 */
bool CredoMacsecUtil::getMacsecScFromJson(std::string scJsonFile, MKASci& sci) {
  bool retVal = false;

  if (std::string scJsonStr; folly::readFile(scJsonFile.c_str(), scJsonStr)) {
    sci = apache::thrift::SimpleJSONSerializer::deserialize<MKASci>(scJsonStr);
    retVal = true;
  } else {
    printf("SC config could not be read\n");
  }

  return retVal;
}

/*
 * printPhyPortMap()
 * Print the Phy port mapping information from the macsec handling process.
 * This function will make the thrift call to the qsfp_service (or phy_service)
 * process to get this phy port mapping information. ie:
 * PORT    SLOT    MDIO    PHYID   NAME
 *  1       2       1       0      eth4/2/1
 *  3       0       0       0      eth2/1/1
 *  5       3       4       0      eth5/5/1
 */
void CredoMacsecUtil::printPhyPortMap(QsfpServiceAsyncClient* fbMacsecHandler) {
  MacsecPortPhyMap macsecPortPhyMap{};
  fbMacsecHandler->sync_macsecGetPhyPortInfo(macsecPortPhyMap);

  printf("Printing the Phy port info map:\n");
  printf("PORT    SLOT    MDIO    PHYID   NAME\n");
  for (auto it : *macsecPortPhyMap.macsecPortPhyMap_ref()) {
    int port = it.first;
    int slot = *it.second.slotId_ref();
    int mdio = *it.second.mdioId_ref();
    int phy = *it.second.phyId_ref();

    printf(
        "%4d    %4d    %4d    %4d    eth%d/%d/1\n",
        port,
        slot,
        mdio,
        phy,
        slot,
        mdio + 1);
  }
}

/*
 * addSaRx()
 * Adds Macsec SA Rx on a given Phy. The syntax is:
 * credo_macsec_util --add_sa_rx --sa_config <sa-json-file> --sc_config
 * <sc-json-file> sa-json-file:
 *  {
 *       "sci": {
 *               "macAddress": "<mac-address>",
 *               "port": <port-id>
 *       },
 *       "l2Port": "<l2-port-string>",
 *       "assocNum": <association-number>,
 *       "keyHex": "<key-hex-value>",
 *       "keyIdHex": "<Key-id-hex-value>",
 *       "primary": <true/false>
 *  }
 * sc-json-file:
 * {
 *       "macAddress": "<mac-address>",
 *       "port": <port-id>
 * }
 */
void CredoMacsecUtil::addSaRx(QsfpServiceAsyncClient* fbMacsecHandler) {
  MKASak sak;
  MKASci sci;

  if (FLAGS_sa_config.empty() || FLAGS_sc_config.empty()) {
    printf("SA and SC config requied\n");
    return;
  }

  bool rc = getMacsecSaFromJson(FLAGS_sa_config, sak);
  if (!rc) {
    printf("SA config could not be read from file\n");
    return;
  }

  rc = getMacsecScFromJson(FLAGS_sc_config, sci);
  if (!rc) {
    printf("SC config could not be read from file\n");
    return;
  }

  rc = fbMacsecHandler->sync_sakInstallRx(sak, sci);
  printf("sakInstallRx is %s\n", rc ? "Successful" : "Failed");
}

/*
 * addSaTx()
 * Adds Macsec SA Tx on a given Phy. The syntax is:
 * credo_macsec_util --add_sa_tx --sa_config <sa-json-file>
 * sa-json-file is described in above command
 */
void CredoMacsecUtil::addSaTx(QsfpServiceAsyncClient* fbMacsecHandler) {
  MKASak sak;

  if (FLAGS_sa_config.empty()) {
    printf("SA config required\n");
    return;
  }

  bool rc = getMacsecSaFromJson(FLAGS_sa_config, sak);
  if (!rc) {
    printf("SA config could not be read from file\n");
    return;
  }

  rc = fbMacsecHandler->sync_sakInstallTx(sak);
  printf("sakInstallTx is %s\n", rc ? "Successful" : "Failed");
}

/*
 * deleteSaRx()
 * Deletes Macsec SA Rx on a given Phy. The syntax is:
 * credo_macsec_util --delete_sa_rx --sa_config <sa-json-file> --sc_config
 * <sc-json-file> sa-json-file and sc-json files described in above command
 */
void CredoMacsecUtil::deleteSaRx(QsfpServiceAsyncClient* fbMacsecHandler) {
  MKASak sak;
  MKASci sci;

  if (FLAGS_sa_config.empty() || FLAGS_sc_config.empty()) {
    printf("SA and SC config requied\n");
    return;
  }

  bool rc = getMacsecSaFromJson(FLAGS_sa_config, sak);
  if (!rc) {
    printf("SA config could not be read from file\n");
    return;
  }

  rc = getMacsecScFromJson(FLAGS_sc_config, sci);
  if (!rc) {
    printf("SC config could not be read from file\n");
    return;
  }

  rc = fbMacsecHandler->sync_sakDeleteRx(sak, sci);
  printf("sakDeleteRx is %s\n", rc ? "Successful" : "Failed");
}

/*
 * deleteSaTx()
 * Adds Macsec SA Tx on a given Phy. The syntax is:
 * credo_macsec_util --delete_sa_tx --sa_config <sa-json-file>
 * sa-json-file is described in above command
 */
void CredoMacsecUtil::deleteSaTx(QsfpServiceAsyncClient* fbMacsecHandler) {
  MKASak sak;

  if (FLAGS_sa_config.empty()) {
    printf("SA config requied\n");
    return;
  }

  bool rc = getMacsecSaFromJson(FLAGS_sa_config, sak);
  if (!rc) {
    printf("SA config could not be read from file\n");
    return;
  }

  rc = fbMacsecHandler->sync_sakDelete(sak);
  printf("sakDelete is %s\n", rc ? "Successful" : "Failed");
}

void CredoMacsecUtil::deleteAllSc(QsfpServiceAsyncClient* fbMacsecHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }

  bool rc = fbMacsecHandler->sync_deleteAllSc(FLAGS_port);
  printf("All SA, SC deletion %s\n", rc ? "Successful" : "Failed");
}

void CredoMacsecUtil::printPhyLinkInfo(
    QsfpServiceAsyncClient* fbMacsecHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }

  PortOperState phyLink =
      fbMacsecHandler->sync_macsecGetPhyLinkInfo(FLAGS_port);
  printf(
      "Port %s phy line status: %s\n",
      FLAGS_port.c_str(),
      (apache::thrift::util::enumNameSafe(phyLink)).c_str());
}

void CredoMacsecUtil::getAllScInfo(QsfpServiceAsyncClient* fbMacsecHandler) {
  std::string allScInfo;
  std::vector<HwObjectType> in{HwObjectType::MACSEC};
  fbMacsecHandler->sync_listHwObjects(allScInfo, in, true);

  printf("Printing all SC info\n");
  printf("%s\n", allScInfo.c_str());
}

void CredoMacsecUtil::getPortStats(QsfpServiceAsyncClient* fbMacsecHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  if (!(FLAGS_ingress ^ FLAGS_egress)) {
    printf("One direction (--ingress or --egress) is required\n");
    return;
  }

  std::map<std::string, MacsecStats> allportStats;
  fbMacsecHandler->sync_getAllMacsecPortStats(allportStats, true);

  auto portMacsecStatsItr = allportStats.find(FLAGS_port);
  if (portMacsecStatsItr == allportStats.end()) {
    printf("Wrong port name specified: %s\n", FLAGS_port.c_str());
    return;
  }

  auto& portMacsecStats = portMacsecStatsItr->second;
  if (FLAGS_ingress) {
    auto& portStats = portMacsecStats.ingressPortStats_ref().value();
    printPortStatsHelper(portStats, FLAGS_port, FLAGS_ingress);
  } else {
    auto& portStats = portMacsecStats.egressPortStats_ref().value();
    printPortStatsHelper(portStats, FLAGS_port, FLAGS_ingress);
  }
}

void CredoMacsecUtil::getFlowStats(QsfpServiceAsyncClient* fbMacsecHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  if ((!FLAGS_ingress && !FLAGS_egress) || (FLAGS_ingress && FLAGS_egress)) {
    printf("One direction (--ingress or --egress) is required\n");
    return;
  }

  std::map<std::string, MacsecStats> allportStats;
  fbMacsecHandler->sync_getAllMacsecPortStats(allportStats, true);

  auto portMacsecStatsItr = allportStats.find(FLAGS_port);
  if (portMacsecStatsItr == allportStats.end()) {
    printf("Wrong port name specified: %s\n", FLAGS_port.c_str());
    return;
  }

  auto& portMacsecStats = portMacsecStatsItr->second;
  if (FLAGS_ingress) {
    for (auto& flowStatsItr : portMacsecStats.ingressFlowStats_ref().value()) {
      auto& sci = flowStatsItr.sci_ref().value();
      auto& flowStats = flowStatsItr.flowStats_ref().value();
      printf(
          "Flow stats for SCI: %s.%d\n",
          sci.macAddress_ref().value().c_str(),
          sci.port_ref().value());
      printFlowStatsHelper(flowStats, FLAGS_port, FLAGS_ingress);
    }
  } else {
    for (auto& flowStatsItr : portMacsecStats.egressFlowStats_ref().value()) {
      auto& sci = flowStatsItr.sci_ref().value();
      auto& flowStats = flowStatsItr.flowStats_ref().value();
      printf(
          "Flow stats for SCI: %s.%d\n",
          sci.macAddress_ref().value().c_str(),
          sci.port_ref().value());
      printFlowStatsHelper(flowStats, FLAGS_port, FLAGS_ingress);
    }
  }
}

void CredoMacsecUtil::getSaStats(QsfpServiceAsyncClient* fbMacsecHandler) {
  if (FLAGS_port == "") {
    printf("Port name is required\n");
    return;
  }
  if ((!FLAGS_ingress && !FLAGS_egress) || (FLAGS_ingress && FLAGS_egress)) {
    printf("One direction (--ingress or --egress) is required\n");
    return;
  }

  std::map<std::string, MacsecStats> allportStats;
  fbMacsecHandler->sync_getAllMacsecPortStats(allportStats, true);

  auto portMacsecStatsItr = allportStats.find(FLAGS_port);
  if (portMacsecStatsItr == allportStats.end()) {
    printf("Wrong port name specified: %s\n", FLAGS_port.c_str());
    return;
  }

  auto& portMacsecStats = portMacsecStatsItr->second;
  if (FLAGS_ingress) {
    for (auto& saStatsItr :
         portMacsecStats.rxSecureAssociationStats_ref().value()) {
      auto& saId = saStatsItr.saId_ref().value();
      auto& saStats = saStatsItr.saStats_ref().value();
      printf(
          "SA stats for SCI: %s.%d, AN: %d\n",
          saId.sci_ref()->macAddress_ref().value().c_str(),
          saId.sci_ref()->port_ref().value(),
          saId.assocNum_ref().value());
      printSaStatsHelper(saStats, FLAGS_port, FLAGS_ingress);
    }
  } else {
    for (auto& saStatsItr :
         portMacsecStats.txSecureAssociationStats_ref().value()) {
      auto& saId = saStatsItr.saId_ref().value();
      auto& saStats = saStatsItr.saStats_ref().value();
      printf(
          "SA stats for SCI: %s.%d, AN: %d\n",
          saId.sci_ref()->macAddress_ref().value().c_str(),
          saId.sci_ref()->port_ref().value(),
          saId.assocNum_ref().value());
      printSaStatsHelper(saStats, FLAGS_port, FLAGS_ingress);
    }
  }
}

void CredoMacsecUtil::getAllPortStats(QsfpServiceAsyncClient* fbMacsecHandler) {
  std::map<std::string, MacsecStats> allportStats;

  if (FLAGS_port == "") {
    fbMacsecHandler->sync_getAllMacsecPortStats(allportStats, true);
  } else {
    std::vector<std::string> ports;
    ports.push_back(FLAGS_port);
    fbMacsecHandler->sync_getMacsecPortStats(allportStats, ports, true);
  }

  for (auto& portStatsItr : allportStats) {
    auto portName = portStatsItr.first;
    printf("Printing stats for %s\n", portName.c_str());
    printPortStatsHelper(
        portStatsItr.second.ingressPortStats_ref().value(), portName, true);
    printPortStatsHelper(
        portStatsItr.second.egressPortStats_ref().value(), portName, false);
    for (auto& flowStatsItr :
         portStatsItr.second.ingressFlowStats_ref().value()) {
      printFlowStatsHelper(
          flowStatsItr.flowStats_ref().value(), portName, true);
    }
    for (auto& flowStatsItr :
         portStatsItr.second.egressFlowStats_ref().value()) {
      printFlowStatsHelper(
          flowStatsItr.flowStats_ref().value(), portName, false);
    }
    for (auto& saStatsItr :
         portStatsItr.second.rxSecureAssociationStats_ref().value()) {
      printSaStatsHelper(saStatsItr.saStats_ref().value(), portName, true);
    }
    for (auto& saStatsItr :
         portStatsItr.second.txSecureAssociationStats_ref().value()) {
      printSaStatsHelper(saStatsItr.saStats_ref().value(), portName, false);
    }
    printf("\n");
  }
}

void CredoMacsecUtil::getSdkState(QsfpServiceAsyncClient* fbMacsecHandler) {
  if (FLAGS_filename == "") {
    printf("File name is required\n");
    return;
  }

  bool rc = fbMacsecHandler->sync_getSdkState(FLAGS_filename);
  printf(
      "SAI state dump to file %s was %s",
      FLAGS_filename.c_str(),
      rc ? "Successful" : "Failed");
}

} // namespace fboss
} // namespace facebook
