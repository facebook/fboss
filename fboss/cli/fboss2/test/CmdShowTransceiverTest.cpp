// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/IPAddressV4.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"
#include "fboss/cli/fboss2/commands/show/transceiver/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;
using namespace facebook::neteng::fboss::bgp::thrift;

namespace facebook::fboss {

/*
 * Set up port test data
 */
std::map<int32_t, PortInfoThrift> createShowTransceiverTestPortEntries() {
  std::map<int32_t, PortInfoThrift> portMap;
  auto addPortEntry = [&portMap](int id) {
    PortInfoThrift portEntry;
    portEntry.portId() = id;
    portEntry.name() = "eth1/" + std::to_string(id) + "/1";

    portMap[id] = std::move(portEntry);
  };

  for (int i = 1; i < 7; i++) {
    addPortEntry(i);
  }
  return portMap;
}

std::map<int32_t, PortStatus> createPortStatusEntries() {
  std::map<int32_t, PortStatus> portStatusMap;
  auto addPortStatusEntry = [&portStatusMap](int id, bool isUp) {
    PortStatus portStatus;
    TransceiverIdxThrift tcvr;
    tcvr.transceiverId() = id;
    portStatus.transceiverIdx() = tcvr;
    portStatus.up() = isUp;

    portStatusMap[id] = std::move(portStatus);
  };

  addPortStatusEntry(1, true);
  addPortStatusEntry(2, true);
  addPortStatusEntry(3, false);
  addPortStatusEntry(4, false);
  addPortStatusEntry(5, true);
  addPortStatusEntry(6, true);

  return portStatusMap;
}

/*
 * Set up transceiver test data
 */
std::map<int32_t, TransceiverInfo> createShowTransceiverTestEntries() {
  std::map<int, TransceiverInfo> transceiverMap;

  // Lambda Helpers
  auto makeVendor =
      [](std::string name, std::string serialNumber, std::string partNumber) {
        Vendor vendor;
        vendor.name() = name;
        vendor.serialNumber() = serialNumber;
        vendor.partNumber() = partNumber;

        return vendor;
      };
  auto makeModuleForFirmware = [](std::string appFwVersion,
                                  std::string dspFwVersion) {
    ModuleStatus moduleStatus;
    FirmwareStatus fwStatus;
    fwStatus.version() = appFwVersion;
    fwStatus.dspFwVer() = dspFwVersion;
    moduleStatus.fwStatus() = fwStatus;

    return moduleStatus;
  };
  auto makeSensor = [](double temp, double voltage) {
    Sensor tempSensor;
    Sensor voltageSensor;
    tempSensor.value() = temp;
    voltageSensor.value() = voltage;

    GlobalSensors sensors;
    sensors.temp() = tempSensor;
    sensors.vcc() = voltageSensor;

    return sensors;
  };

  TransceiverInfo transceiverEntry1;
  transceiverEntry1.tcvrState()->vendor() = makeVendor("vendorOne", "aa", "1");
  transceiverEntry1.tcvrState()->status() = makeModuleForFirmware("1", "2");
  transceiverEntry1.tcvrState()->present() = true;
  transceiverEntry1.tcvrStats()->sensor() = makeSensor(50.0, 25.0);
  transceiverEntry1.tcvrStats()->channels() = {};

  TransceiverInfo transceiverEntry2;
  transceiverEntry2.tcvrState()->vendor() = makeVendor("vendorTwo", "b", "3");
  transceiverEntry2.tcvrState()->present() = true;
  transceiverEntry2.tcvrStats()->sensor() = makeSensor(40.0, 25.0);
  transceiverEntry2.tcvrStats()->channels() = {};

  TransceiverInfo transceiverEntry3;
  transceiverEntry3.tcvrState()->vendor() = makeVendor("vendorOne", "ab", "1");
  transceiverEntry3.tcvrState()->present() = false;
  transceiverEntry3.tcvrStats()->sensor() = makeSensor(0.0, 0.0);
  transceiverEntry3.tcvrStats()->channels() = {};

  TransceiverInfo transceiverEntry4;
  transceiverEntry4.tcvrState()->vendor() = makeVendor("vendorOne", "ac", "1");
  transceiverEntry4.tcvrState()->present() = false;
  transceiverEntry4.tcvrStats()->sensor() = makeSensor(0.0, 0.0);
  transceiverEntry4.tcvrStats()->channels() = {};

  TransceiverInfo transceiverEntry5;
  transceiverEntry5.tcvrState()->vendor() = makeVendor("vendorOne", "ad", "2");
  transceiverEntry5.tcvrState()->present() = true;
  transceiverEntry5.tcvrStats()->sensor() = makeSensor(0.0, 0.0);
  transceiverEntry5.tcvrStats()->channels() = {};

  TransceiverInfo transceiverEntry6;
  transceiverEntry6.tcvrState()->vendor() = makeVendor("vendorThree", "c", "4");
  transceiverEntry6.tcvrState()->status() = makeModuleForFirmware("3", "4");
  transceiverEntry6.tcvrState()->present() = true;
  transceiverEntry6.tcvrStats()->sensor() = makeSensor(30.0, 30.0);
  transceiverEntry6.tcvrStats()->channels() = {};

  transceiverMap[1] = transceiverEntry1;
  transceiverMap[2] = transceiverEntry2;
  transceiverMap[3] = transceiverEntry3;
  transceiverMap[4] = transceiverEntry4;
  transceiverMap[5] = transceiverEntry5;
  transceiverMap[6] = transceiverEntry6;

  return transceiverMap;
}

std::map<int32_t, std::string> createTransceiverValidationEntries() {
  return {
      {1, "nonValidatedVendorPartNumber"},
      {2, "missingVendor"},
      {5, "invalidEepromChecksums"},
      {6, ""}};
}

cli::ShowTransceiverModel createTransceiverModel() {
  auto makeDefaultTcvrDetail = [](std::string name, bool isUp, bool isPresent) {
    cli::TransceiverDetail detail;
    detail.name() = name;
    detail.isUp() = isUp;
    detail.isPresent() = isPresent;
    return detail;
  };
  auto setConfigAttributes = [](cli::TransceiverDetail& detail,
                                std::string vendorName,
                                std::string serialNumber,
                                std::string partNumber,
                                std::string appFwVer,
                                std::string dspFwVer) {
    detail.vendor() = vendorName;
    detail.serial() = serialNumber;
    detail.partNumber() = partNumber;
    detail.appFwVer() = appFwVer;
    detail.dspFwVer() = dspFwVer;
  };
  auto setValidationStatus = [](cli::TransceiverDetail& detail,
                                std::string validationStatus,
                                std::string notValidatedReason) {
    detail.validationStatus() = validationStatus;
    detail.notValidatedReason() = notValidatedReason;
  };
  auto setOperAttributes =
      [](cli::TransceiverDetail& detail, double temperature, double voltage) {
        detail.temperature() = temperature;
        detail.voltage() = voltage;
        detail.currentMA() = {};
        detail.txPower() = {};
        detail.rxPower() = {};
        detail.rxSnr() = {};
      };

  cli::ShowTransceiverModel model;

  auto entry1 = makeDefaultTcvrDetail("eth1/1/1", true, true);
  setConfigAttributes(entry1, "vendorOne", "aa", "1", "1", "2");
  setValidationStatus(entry1, "Not Validated", "nonValidatedVendorPartNumber");
  setOperAttributes(entry1, 50.0, 25.0);

  auto entry2 = makeDefaultTcvrDetail("eth1/2/1", true, true);
  setConfigAttributes(entry2, "vendorTwo", "b", "3", "", "");
  setValidationStatus(entry2, "Not Validated", "missingVendor");
  setOperAttributes(entry2, 40.0, 25.0);

  auto entry3 = makeDefaultTcvrDetail("eth1/3/1", false, false);
  setConfigAttributes(entry3, "vendorOne", "ab", "1", "", "");
  setValidationStatus(entry3, "--", "--");
  setOperAttributes(entry3, 0.0, 0.0);

  auto entry4 = makeDefaultTcvrDetail("eth1/4/1", false, false);
  setConfigAttributes(entry4, "vendorOne", "ac", "1", "", "");
  setValidationStatus(entry4, "--", "--");
  setOperAttributes(entry4, 0.0, 0.0);

  auto entry5 = makeDefaultTcvrDetail("eth1/5/1", true, true);
  setConfigAttributes(entry5, "vendorOne", "ad", "2", "", "");
  setValidationStatus(entry5, "Not Validated", "invalidEepromChecksums");
  setOperAttributes(entry5, 0.0, 0.0);

  auto entry6 = makeDefaultTcvrDetail("eth1/6/1", true, true);
  setConfigAttributes(entry6, "vendorThree", "c", "4", "3", "4");
  setValidationStatus(entry6, "Validated", "--");
  setOperAttributes(entry6, 30.0, 30.0);

  model.transceivers()->emplace("eth1/1/1", std::move(entry1));
  model.transceivers()->emplace("eth1/2/1", std::move(entry2));
  model.transceivers()->emplace("eth1/3/1", std::move(entry3));
  model.transceivers()->emplace("eth1/4/1", std::move(entry4));
  model.transceivers()->emplace("eth1/5/1", std::move(entry5));
  model.transceivers()->emplace("eth1/6/1", std::move(entry6));

  return model;
}

class CmdShowTransceiverTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> mockPortEntries;
  std::map<int32_t, PortStatus> mockPortStatusEntries;
  std::map<int32_t, facebook::fboss::TransceiverInfo> mockTransceiverEntries;
  std::map<int32_t, std::string> mockTransceiverValidationEntries;
  cli::ShowTransceiverModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockPortEntries = createShowTransceiverTestPortEntries();
    mockPortStatusEntries = createPortStatusEntries();
    mockTransceiverEntries = createShowTransceiverTestEntries();
    mockTransceiverValidationEntries = createTransceiverValidationEntries();
    normalizedModel = createTransceiverModel();
  }
};

TEST_F(CmdShowTransceiverTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockPortEntries; }));
  EXPECT_CALL(getMockAgent(), getPortStatus(_, _))
      .WillOnce(Invoke(
          [&](auto& entries, auto) { entries = mockPortStatusEntries; }));

  EXPECT_CALL(getQsfpService(), getTransceiverInfo(_, _))
      .WillOnce(Invoke(
          [&](auto& entries, auto) { entries = mockTransceiverEntries; }));
  EXPECT_CALL(getQsfpService(), getTransceiverConfigValidationInfo(_, _, _))
      .WillOnce(Invoke([&](auto& entries, auto, auto) {
        entries = mockTransceiverValidationEntries;
      }));

  auto cmd = CmdShowTransceiver();
  CmdShowTransceiverTraits::ObjectArgType queriedEntries;
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

TEST_F(CmdShowTransceiverTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowTransceiver().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " Interface  Status  Present  CfgValidated   Reason                        Vendor       Serial  Part Number  FW App Version  FW DSP Version  Temperature (C)  Voltage (V)  Current (mA)  Tx Power (dBm)  Rx Power (dBm)  Rx SNR \n"
      "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " eth1/1/1   Up      Present  Not Validated  nonValidatedVendorPartNumber  vendorOne    aa      1            1               2               50.00            25.00                                                             \n"
      " eth1/2/1   Up      Present  Not Validated  missingVendor                 vendorTwo    b       3                                            40.00            25.00                                                             \n"
      " eth1/3/1   Down    Absent   --             --                            vendorOne    ab      1                                            0.00             0.00                                                              \n"
      " eth1/4/1   Down    Absent   --             --                            vendorOne    ac      1                                            0.00             0.00                                                              \n"
      " eth1/5/1   Up      Present  Not Validated  invalidEepromChecksums        vendorOne    ad      2                                            0.00             0.00                                                              \n"
      " eth1/6/1   Up      Present  Validated      --                            vendorThree  c       4            3               4               30.00            30.00                                                             \n\n";

  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
