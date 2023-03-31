// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/product/CmdShowProductDetails.h"
#include "fboss/cli/fboss2/commands/show/product/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

ProductInfo createMockProductDetails() {
  ProductInfo productInfo;

  productInfo.product() = "TEST-PRODUCT";
  productInfo.oem() = "CLS";
  productInfo.serial() = "S854930657198T";
  productInfo.mgmtMac() = "05:31:12:79:08:45";
  productInfo.bmcMac() = "15:21:75:98:40:20";
  productInfo.macRangeStart() = "00:00:00:00:00:05";
  productInfo.macRangeSize() = 2;
  productInfo.assembledAt() = "CLS";
  productInfo.assetTag() = "5968201";
  productInfo.partNumber() = "15-395012";
  productInfo.productionState() = 5;
  productInfo.subVersion() = 1;
  productInfo.productVersion() = 2;
  productInfo.systemPartNumber() = "34-4121709-23";
  productInfo.mfgDate() = "02-12-18";
  productInfo.pcbManufacturer() = "WUS";
  productInfo.fbPcbaPartNumber() = "453-000028-05";
  productInfo.fbPcbPartNumber() = "455-000028-04";
  productInfo.odmPcbaPartNumber() = "G3498W092476";
  productInfo.odmPcbaSerial() = "J284950362756";
  productInfo.version() = 8;

  return productInfo;
}

cli::ShowProductDetailsModel createExpectedProductDetailsModel() {
  cli::ShowProductDetailsModel model;

  model.product() = "TEST-PRODUCT";
  model.oem() = "CLS";
  model.serial() = "S854930657198T";
  model.mgmtMac() = "05:31:12:79:08:45";
  model.bmcMac() = "15:21:75:98:40:20";
  model.macRangeStart() = "00:00:00:00:00:05";
  model.macRangeSize() = "2";
  model.assembledAt() = "CLS";
  model.assetTag() = "5968201";
  model.partNumber() = "15-395012";
  model.productionState() = "5";
  model.subVersion() = "1";
  model.productVersion() = "2";
  model.systemPartNumber() = "34-4121709-23";
  model.mfgDate() = "02-12-18";
  model.pcbManufacturer() = "WUS";
  model.fbPcbaPartNumber() = "453-000028-05";
  model.fbPcbPartNumber() = "455-000028-04";
  model.odmPcbaPartNumber() = "G3498W092476";
  model.odmPcbaSerial() = "J284950362756";
  model.version() = "8";

  return model;
}

class CmdShowProductDetailsTestFixture : public CmdHandlerTestBase {
 public:
  ProductInfo mockProductDetails;
  cli::ShowProductDetailsModel expectedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockProductDetails = createMockProductDetails();
    expectedModel = createExpectedProductDetailsModel();
  }
};

TEST_F(CmdShowProductDetailsTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getProductInfo(_))
      .WillOnce(
          Invoke([&](auto& productInfo) { productInfo = mockProductDetails; }));

  auto cmd = CmdShowProductDetails();
  auto model = cmd.queryClient(localhost());
  EXPECT_THRIFT_EQ(model, expectedModel);
}

TEST_F(CmdShowProductDetailsTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowProductDetails().printOutput(expectedModel, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      "Product: TEST-PRODUCT\n"
      "OEM: CLS\n"
      "Serial: S854930657198T\n"
      "Management MAC Address: 05:31:12:79:08:45\n"
      "BMC MAC Address: 15:21:75:98:40:20\n"
      "Extended MAC Start: 00:00:00:00:00:05\n"
      "Extended MAC Size: 2\n"
      "Assembled At: CLS\n"
      "Product Asset Tag: 5968201\n"
      "Product Part Number: 15-395012\n"
      "Product Production State: 5\n"
      "Product Sub-Version: 1\n"
      "Product Version: 2\n"
      "System Assembly Part Number: 34-4121709-23\n"
      "System Manufacturing Date: 02-12-18\n"
      "PCB Manufacturer: WUS\n"
      "Facebook PCBA Part Number: 453-000028-05\n"
      "Facebook PCB Part Number: 455-000028-04\n"
      "ODM PCBA Part Number: G3498W092476\n"
      "ODM PCBA Serial Number: J284950362756\n"
      "Version: 8\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
