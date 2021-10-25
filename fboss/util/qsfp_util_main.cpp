// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"
#include "fboss/util/wedge_qsfp_util.h"
#include "folly/gen/Base.h"

#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <sysexits.h>
#include <iostream>
#include <iterator>

using namespace facebook::fboss;
using std::make_pair;
using std::chrono::seconds;
using std::chrono::steady_clock;

std::vector<FlagCommand> kCommands = {
    {"pause_remediation", {}},
    {"get_remediation_until_time", {}},
    {"read_reg", {"offset", "length", "page", "i2c_address"}},
    {"write_reg", {"offset", "length", "page", "i2c_address"}},
    {"clear_low_power", {}},
    {"set_low_power", {}},
    {"tx_enable", {}},
    {"tx_disable", {}},
    {"set_40g", {}},
    {"set_100g", {}},
    {"app_sel", {}},
    {"cdr_enable", {}},
    {"cdr_disable", {}},
    {"qsfp_hard_reset", {}},
    {"electrical_loopback", {}},
    {"optical_loopback", {}},
    {"clear_loopback", {}},
    {"update_module_firmware", {"firmware_filename"}},
    {"cdb_command", {}},
    {"get_module_fw_info", {}},
    {"update_bulk_module_fw",
     {"port_range", "firmware_filename", "module_type", "fw_version"}},
    {"batch_ops", {"batchfile"}},
};

void listCommands() {
  std::cerr
      << "Commands in this tool are flags so you can combine them "
         "and for example set_40g and tx_enable on all ports specified.\n\n"
         "Valid commands and their flags:\n";

  std::cerr << "Command:\n    <None>: Print port details. Prints port "
               "summary for all ports if no port specified.\nFlags:\n\n";

  std::copy(
      std::begin(kCommands),
      std::end(kCommands),
      std::ostream_iterator<FlagCommand>(std::cerr, "\n"));
}

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  folly::EventBase evb;

  if (FLAGS_list_commands) {
    listCommands();
    return EX_OK;
  }

  if (FLAGS_set_100g && FLAGS_set_40g) {
    fprintf(stderr, "Cannot set both 40g and 100g\n");
    return EX_USAGE;
  }
  if (FLAGS_cdr_enable && FLAGS_cdr_disable) {
    fprintf(stderr, "Cannot set and clear the CDR bits\n");
    return EX_USAGE;
  }
  if (FLAGS_clear_low_power && FLAGS_set_low_power) {
    fprintf(stderr, "Cannot set and clear lp mode\n");
    return EX_USAGE;
  }

  if (FLAGS_pause_remediation) {
    try {
      auto client = getQsfpClient(evb);
      client->sync_pauseRemediation(FLAGS_pause_remediation);
      return EX_OK;
    } catch (const std::exception& ex) {
      fprintf(
          stderr, "error pausing remediation of qsfp_service: %s\n", ex.what());
      return EX_SOFTWARE;
    }
  }

  if (FLAGS_get_remediation_until_time) {
    try {
      doGetRemediationUntilTime(evb);
      return EX_OK;
    } catch (const std::exception& ex) {
      fprintf(
          stderr,
          "error getting remediationUntil time from qsfp_service: %s\n",
          ex.what());
      return EX_SOFTWARE;
    }
  }

  std::vector<unsigned int> ports;
  bool good = true;
  std::unique_ptr<WedgeManager> wedgeManager = nullptr;
  if (argc == 1) {
    wedgeManager = createWedgeManager();
    folly::gen::range(0, wedgeManager->getNumQsfpModules()) |
        folly::gen::appendTo(ports);
  } else {
    for (int n = 1; n < argc; ++n) {
      unsigned int portNum;
      auto& portStr = argv[n];
      try {
        if (argv[n][0] == 'x' && argv[n][1] == 'e') {
          portNum = 1 + folly::to<unsigned int>(argv[n] + 2);
        } else if (isalpha(portStr[0])) {
          if (!wedgeManager) {
            wedgeManager = createWedgeManager();
          }
          portNum = wedgeManager->getPortNameToModuleMap().at(portStr) + 1;
        } else {
          portNum = folly::to<unsigned int>(argv[n]);
        }
        ports.push_back(portNum);
      } catch (const std::exception& ex) {
        fprintf(
            stderr,
            "error: invalid port name/number \"%s\": %s\n",
            argv[n],
            ex.what());
        good = false;
      }
    }
  }
  if (!good) {
    return EX_USAGE;
  }
  auto busAndError = getTransceiverAPI();
  if (busAndError.second) {
    return busAndError.second;
  }
  auto bus = std::move(busAndError.first);

  bool printInfo =
      !(FLAGS_clear_low_power || FLAGS_tx_disable || FLAGS_tx_enable ||
        FLAGS_set_100g || FLAGS_set_40g || FLAGS_cdr_enable ||
        FLAGS_cdr_disable || FLAGS_set_low_power || FLAGS_qsfp_hard_reset ||
        FLAGS_electrical_loopback || FLAGS_optical_loopback ||
        FLAGS_clear_loopback || FLAGS_read_reg || FLAGS_write_reg ||
        FLAGS_update_module_firmware || FLAGS_get_module_fw_info ||
        FLAGS_app_sel || FLAGS_cdb_command || FLAGS_update_bulk_module_fw ||
        FLAGS_vdm_info);

  if (FLAGS_direct_i2c || !printInfo) {
    try {
      tryOpenBus(bus.get());
    } catch (const std::exception& ex) {
      fprintf(stderr, "error: unable to open device: %s\n", ex.what());
      return EX_IOERR;
    }
  } else {
    try {
      std::vector<int32_t> idx = zeroBasedPortIds(ports);
      if (FLAGS_client_parser) {
        auto domDataUnionMap = fetchDataFromQsfpService(idx, evb);
        for (auto& tcvrId : idx) {
          auto iter = domDataUnionMap.find(tcvrId);
          if (iter == domDataUnionMap.end()) {
            fprintf(stderr, "Port %d is not present.\n", tcvrId + 1);
          } else {
            printPortDetail(iter->second, iter->first + 1);
          }
        }
      } else {
        auto transceiverInfo = fetchInfoFromQsfpService(idx, evb);
        for (auto& i : idx) {
          auto iter = transceiverInfo.find(i);
          if (iter == transceiverInfo.end()) {
            fprintf(
                stderr, "qsfp_service didn't return data for Port %d\n", i + 1);
          } else {
            printPortDetailService(
                iter->second, iter->first + 1, FLAGS_verbose);
          }
        }
      }
      return EX_OK;
    } catch (const std::exception& e) {
      fprintf(stderr, "Exception talking to qsfp_service: %s\n", e.what());
      return EX_SOFTWARE;
    }
  }

  if (FLAGS_read_reg) {
    return doReadReg(
        bus.get(), ports, FLAGS_offset, FLAGS_length, FLAGS_page, evb);
  }

  if (FLAGS_write_reg) {
    return doWriteReg(
        bus.get(), ports, FLAGS_offset, FLAGS_page, FLAGS_data, evb);
  }

  if (FLAGS_batch_ops) {
    return doBatchOps(bus.get(), ports, FLAGS_batchfile, evb);
  }

  int retcode = EX_OK;
  for (unsigned int portNum : ports) {
    if (FLAGS_clear_low_power && overrideLowPower(bus.get(), portNum, false)) {
      printf("QSFP %d: cleared low power flags\n", portNum);
    }
    if (FLAGS_set_low_power && overrideLowPower(bus.get(), portNum, true)) {
      printf("QSFP %d: set low power flags\n", portNum);
    }
    if (FLAGS_tx_disable && setTxDisable(bus.get(), portNum, true)) {
      printf(
          "QSFP %d: disabled TX on %s channels\n",
          portNum,
          (FLAGS_channel >= 1 && FLAGS_channel <= 8) ? "some" : "all");
    }
    if (FLAGS_tx_enable && setTxDisable(bus.get(), portNum, false)) {
      printf(
          "QSFP %d: enabled TX on %s channels\n",
          portNum,
          (FLAGS_channel >= 1 && FLAGS_channel <= 8) ? "some" : "all");
    }

    if (FLAGS_set_40g && rateSelect(bus.get(), portNum, 0x0)) {
      printf("QSFP %d: set to optimize for 10G channels\n", portNum);
    }
    if (FLAGS_set_100g && rateSelect(bus.get(), portNum, 0xaa)) {
      printf("QSFP %d: set to optimize for 25G channels\n", portNum);
    }
    if (FLAGS_app_sel && appSel(bus.get(), portNum, FLAGS_app_sel)) {
      printf("QSFP %d: set to application %d\n", portNum, FLAGS_app_sel);
    }

    if (FLAGS_cdr_enable && setCdr(bus.get(), portNum, 0xff)) {
      printf("QSFP %d: CDR enabled\n", portNum);
    }

    if (FLAGS_cdr_disable && setCdr(bus.get(), portNum, 0x00)) {
      printf("QSFP %d: CDR disabled\n", portNum);
    }

    if (FLAGS_qsfp_hard_reset && doQsfpHardReset(bus.get(), portNum)) {
      printf("QSFP %d: Hard reset done\n", portNum);
    }

    if (FLAGS_electrical_loopback) {
      if (getModuleType(bus.get(), portNum) !=
          TransceiverManagementInterface::CMIS) {
        if (doMiniphotonLoopback(bus.get(), portNum, electricalLoopback)) {
          printf(
              "QSFP %d: done setting module to electrical loopback.\n",
              portNum);
        }
      } else {
        cmisHostInputLoopback(bus.get(), portNum, electricalLoopback);
      }
    }

    if (FLAGS_optical_loopback) {
      if (getModuleType(bus.get(), portNum) !=
          TransceiverManagementInterface::CMIS) {
        if (doMiniphotonLoopback(bus.get(), portNum, opticalLoopback)) {
          printf(
              "QSFP %d: done setting module to optical loopback.\n", portNum);
        }
      } else {
        cmisMediaInputLoopback(bus.get(), portNum, opticalLoopback);
      }
    }

    if (FLAGS_clear_loopback) {
      if (getModuleType(bus.get(), portNum) !=
          TransceiverManagementInterface::CMIS) {
        if (doMiniphotonLoopback(bus.get(), portNum, noLoopback)) {
          printf("QSFP %d: done clear module to loopback.\n", portNum);
        }
      } else {
        cmisHostInputLoopback(bus.get(), portNum, noLoopback);
        cmisMediaInputLoopback(bus.get(), portNum, noLoopback);
      }
    }

    if (FLAGS_vdm_info && printVdmInfo(bus.get(), portNum)) {
      printf("QSFP %d: printed module vdm info\n", portNum);
      return EX_OK;
    }

    if (FLAGS_direct_i2c && printInfo) {
      try {
        // Get the port details from the direct i2c read and then print out the
        // i2c info from module
        printPortDetail(fetchDataFromLocalI2CBus(bus.get(), portNum), portNum);
      } catch (const I2cError& ex) {
        // This generally means the QSFP module is not present.
        fprintf(stderr, "Port %d: not present: %s\n", portNum, ex.what());
        retcode = EX_SOFTWARE;
      } catch (const std::exception& ex) {
        fprintf(stderr, "error parsing QSFP data %u: %s\n", portNum, ex.what());
        retcode = EX_SOFTWARE;
      }
    }

    if (FLAGS_update_module_firmware) {
      printf("This action may bring down the port and interrupt the traffic\n");
      if (FLAGS_firmware_filename.empty()) {
        fprintf(
            stderr,
            "QSFP %d: Fail to upgrade firmware. Specify firmware using --firmware_filename\n",
            portNum);
      } else {
        cliModulefirmwareUpgrade(bus.get(), portNum, FLAGS_firmware_filename);
      }
    }

    if (FLAGS_cdb_command) {
      if (getModuleType(bus.get(), portNum) !=
          TransceiverManagementInterface::CMIS) {
        printf("This command is applicable to CMIS module only\n");
      } else {
        doCdbCommand(bus.get(), portNum);
      }
    }
  }

  if (FLAGS_get_module_fw_info) {
    if (ports.size() < 1) {
      fprintf(
          stderr,
          "Pl specify 1 module or 2 modules for the range: <ModuleA> <moduleB>\n");
    } else if (ports.size() == 1) {
      get_module_fw_info(bus.get(), ports[0], ports[0]);
    } else {
      get_module_fw_info(bus.get(), ports[0], ports[1]);
    }
  }

  if (FLAGS_update_bulk_module_fw) {
    if (FLAGS_port_range.empty()) {
      fprintf(stderr, "Pl specify the port range ie: 1,3,5-8\n");
      return EX_USAGE;
    }
    if (FLAGS_firmware_filename.empty()) {
      fprintf(
          stderr, "Pl specify firmware filename using --firmware_filename\n");
      return EX_USAGE;
    }
    if (FLAGS_module_type.empty()) {
      fprintf(
          stderr,
          "Pl specify module type using --module_type (ie: finisar-200g)\n");
      return EX_USAGE;
    }
    if (FLAGS_fw_version.empty()) {
      fprintf(
          stderr,
          "Pl specify firmware version using --fw_version (ie: 7.8 or ca.f8)\n");
      return EX_USAGE;
    }

    cliModulefirmwareUpgrade(
        bus.get(), FLAGS_port_range, FLAGS_firmware_filename);
  }

  return retcode;
}
