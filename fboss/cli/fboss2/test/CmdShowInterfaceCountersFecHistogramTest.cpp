// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/interface/counters/fec/histogram/CmdShowInterfaceCountersFecHistogram.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/histogram/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {

CdbDatapathSymErrHistogram makeSymErr(double mediaVal, double hostVal) {
  SymErrHistogramBin mediaBin;
  mediaBin.nbitSymbolErrorMax() = mediaVal;
  mediaBin.nbitSymbolErrorAvg() = mediaVal;
  mediaBin.nbitSymbolErrorCur() = mediaVal;

  SymErrHistogramBin hostBin;
  hostBin.nbitSymbolErrorMax() = hostVal;
  hostBin.nbitSymbolErrorAvg() = hostVal;
  hostBin.nbitSymbolErrorCur() = hostVal;

  CdbDatapathSymErrHistogram symErr;
  symErr.media() = std::map<int32_t, SymErrHistogramBin>{{0, mediaBin}};
  symErr.host() = std::map<int32_t, SymErrHistogramBin>{{0, hostBin}};
  return symErr;
}

} // namespace

class CmdShowInterfaceCountersFecHistogramTestFixture
    : public CmdHandlerTestBase {
 public:
  utils::LinkDirection lineDirection = utils::LinkDirection({"line"});
  utils::LinkDirection systemDirection = utils::LinkDirection({"system"});
};

TEST_F(
    CmdShowInterfaceCountersFecHistogramTestFixture,
    queryClientExplicitInterface) {
  setupMockedAgentServer();

  EXPECT_CALL(getQsfpService(), getPortMediaInterface(_)).Times(0);
  EXPECT_CALL(getQsfpService(), getSymbolErrorHistogram(_, _))
      .WillOnce([](CdbDatapathSymErrHistogram& symErr, auto&&) {
        symErr = makeSymErr(1, 4);
      });

  auto model = CmdShowInterfaceCountersFecHistogram().queryClient(
      localhost(), {"eth1/1/1"}, lineDirection);

  EXPECT_EQ(model.nBitCorrectedWords()->size(), 1);
  EXPECT_EQ(
      model.nBitCorrectedWords()->at("eth1/1/1").nBitCorrectedMax()->at(0), 1);
  EXPECT_EQ(
      model.nBitCorrectedWords()->at("eth1/1/1").nBitCorrectedAvg()->at(0), 1);
  EXPECT_EQ(
      model.nBitCorrectedWords()->at("eth1/1/1").nBitCorrectedCur()->at(0), 1);
}

TEST_F(
    CmdShowInterfaceCountersFecHistogramTestFixture,
    queryClientUsesHostSideBinsForSystemDirection) {
  setupMockedAgentServer();

  EXPECT_CALL(getQsfpService(), getSymbolErrorHistogram(_, _))
      .WillOnce([](CdbDatapathSymErrHistogram& symErr, auto&&) {
        symErr = makeSymErr(1, 4);
      });

  auto model = CmdShowInterfaceCountersFecHistogram().queryClient(
      localhost(), {"eth1/1/1"}, systemDirection);

  EXPECT_EQ(
      model.nBitCorrectedWords()->at("eth1/1/1").nBitCorrectedMax()->at(0), 4);
}

TEST_F(
    CmdShowInterfaceCountersFecHistogramTestFixture,
    queryClientFallsBackToAllInterfacesWhenNoneQueried) {
  setupMockedAgentServer();

  std::map<std::string, MediaInterfaceCode> mediaInterfaces = {
      {"eth1/1/1", MediaInterfaceCode::CWDM4_100G},
      {"eth1/2/1", MediaInterfaceCode::CWDM4_100G}};

  EXPECT_CALL(getQsfpService(), getPortMediaInterface(_))
      .WillOnce([&](std::map<std::string, MediaInterfaceCode>& out) {
        out = mediaInterfaces;
      });
  EXPECT_CALL(getQsfpService(), getSymbolErrorHistogram(_, _))
      .Times(2)
      .WillRepeatedly([](CdbDatapathSymErrHistogram& symErr, auto&&) {
        symErr = makeSymErr(2, 5);
      });

  auto model = CmdShowInterfaceCountersFecHistogram().queryClient(
      localhost(), {}, lineDirection);

  EXPECT_EQ(model.nBitCorrectedWords()->size(), 2);
  EXPECT_TRUE(
      model.nBitCorrectedWords()->find("eth1/1/1") !=
      model.nBitCorrectedWords()->end());
  EXPECT_TRUE(
      model.nBitCorrectedWords()->find("eth1/2/1") !=
      model.nBitCorrectedWords()->end());
}

TEST_F(
    CmdShowInterfaceCountersFecHistogramTestFixture,
    queryClientSkipsInterfacesThatThrowWhenEnumeratingAll) {
  setupMockedAgentServer();

  std::map<std::string, MediaInterfaceCode> mediaInterfaces = {
      {"eth1/1/1", MediaInterfaceCode::CWDM4_100G},
      {"eth1/2/1", MediaInterfaceCode::CWDM4_100G}};

  EXPECT_CALL(getQsfpService(), getPortMediaInterface(_))
      .WillOnce([&](std::map<std::string, MediaInterfaceCode>& out) {
        out = mediaInterfaces;
      });
  EXPECT_CALL(getQsfpService(), getSymbolErrorHistogram(_, _))
      .WillRepeatedly(
          [](CdbDatapathSymErrHistogram& symErr, const auto& portName) {
            if (*portName == "eth1/1/1") {
              throw thrift::FbossBaseError("CDB command unsupported");
            }
            symErr = makeSymErr(3, 6);
          });

  auto model = CmdShowInterfaceCountersFecHistogram().queryClient(
      localhost(), {}, lineDirection);

  EXPECT_EQ(model.nBitCorrectedWords()->size(), 1);
  EXPECT_TRUE(
      model.nBitCorrectedWords()->find("eth1/1/1") ==
      model.nBitCorrectedWords()->end());
  EXPECT_TRUE(
      model.nBitCorrectedWords()->find("eth1/2/1") !=
      model.nBitCorrectedWords()->end());
}

TEST_F(
    CmdShowInterfaceCountersFecHistogramTestFixture,
    queryClientReturnsEmptyModelWhenEnumerationThrows) {
  setupMockedAgentServer();

  EXPECT_CALL(getQsfpService(), getPortMediaInterface(_))
      .WillOnce(Throw(thrift::FbossBaseError("qsfp_service internal error")));
  EXPECT_CALL(getQsfpService(), getSymbolErrorHistogram(_, _)).Times(0);

  auto model = CmdShowInterfaceCountersFecHistogram().queryClient(
      localhost(), {}, lineDirection);

  EXPECT_EQ(model.nBitCorrectedWords()->size(), 0);
}

TEST_F(
    CmdShowInterfaceCountersFecHistogramTestFixture,
    queryClientReportsExplicitInterfaceThatThrows) {
  setupMockedAgentServer();

  EXPECT_CALL(getQsfpService(), getSymbolErrorHistogram(_, _))
      .WillOnce(Throw(thrift::FbossBaseError("no transceiver on port")));

  testing::internal::CaptureStderr();
  auto model = CmdShowInterfaceCountersFecHistogram().queryClient(
      localhost(), {"eth1/1/1"}, lineDirection);
  std::string stderrOutput = testing::internal::GetCapturedStderr();

  EXPECT_EQ(model.nBitCorrectedWords()->size(), 0);
  EXPECT_THAT(stderrOutput, HasSubstr("eth1/1/1"));
}

TEST_F(
    CmdShowInterfaceCountersFecHistogramTestFixture,
    queryClientReturnsEmptyModelWhenQsfpServiceUnreachable) {
  setupMockedAgentServer();
  mockedQsfpServer_.reset();

  auto model = CmdShowInterfaceCountersFecHistogram().queryClient(
      localhost(), {"eth1/1/1"}, lineDirection);

  EXPECT_EQ(model.nBitCorrectedWords()->size(), 0);
}

} // namespace facebook::fboss
