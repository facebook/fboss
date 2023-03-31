// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/product/CmdShowProduct.h"
#include "fboss/cli/fboss2/commands/show/product/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

ProductInfo createMockProductInfo() {
  ProductInfo productInfo;

  productInfo.product() = "TEST-PRODUCT";
  productInfo.oem() = "CLS";
  productInfo.serial() = "S854930657198T";

  return productInfo;
}

cli::ShowProductModel createExpectedProductModel() {
  cli::ShowProductModel model;

  model.product() = "TEST-PRODUCT";
  model.oem() = "CLS";
  model.serial() = "S854930657198T";

  return model;
}

class CmdShowProductTestFixture : public CmdHandlerTestBase {
 public:
  ProductInfo mockProductInfo;
  cli::ShowProductModel expectedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockProductInfo = createMockProductInfo();
    expectedModel = createExpectedProductModel();
  }
};

TEST_F(CmdShowProductTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getProductInfo(_))
      .WillOnce(
          Invoke([&](auto& productInfo) { productInfo = mockProductInfo; }));

  auto cmd = CmdShowProduct();
  auto model = cmd.queryClient(localhost());
  EXPECT_THRIFT_EQ(model, expectedModel);
}

TEST_F(CmdShowProductTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowProduct().printOutput(expectedModel, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      "Product: TEST-PRODUCT\n"
      "OEM: CLS\n"
      "Serial: S854930657198T\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
