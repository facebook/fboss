#include "fboss/agent/packet/PTPHeader.h"

#include <folly/io/Cursor.h>
#include <gtest/gtest.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

using namespace facebook::fboss;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;

TEST(PTPHdrTest, constructor) {
  uint8_t ptpType = static_cast<uint8_t>(PTPMessageType::PTP_DELAY_REQUEST);
  uint8_t ptpVersion = static_cast<uint8_t>(PTPVersion::PTP_V2);

  PTPHeader ptpHdr(ptpType, ptpVersion, 0x1234);

  EXPECT_EQ(ptpType, ptpHdr.ptpType_);
  EXPECT_EQ(ptpVersion, ptpHdr.ptpVersion_);
  EXPECT_EQ(0x1234, ptpHdr.ptpCorrectionField_);
}

TEST(PTPHdrTest, readPacket) {
  auto pkt = MockRxPacket::fromHex(
      // PTP Header
      "01" // PTP Type
      "01" // PTP Version
      "00 2c" // msg length
      "00 00 04 00"
      "00 00 00 00 12 34 56 78" // correction field
  );
  Cursor cursor(pkt->buf());
  PTPHeader ptpHdr(&cursor);

  EXPECT_EQ(ptpHdr.ptpType_, 0x01);
  EXPECT_EQ(ptpHdr.ptpVersion_, 0x01);
  EXPECT_EQ(ptpHdr.ptpCorrectionField_, 0x12345678);
  EXPECT_EQ(ptpHdr.ptpMessageLength_, 0x2c);
}

TEST(PTPHdrTest, readPacketWrongPtpType) {
  auto pkt = MockRxPacket::fromHex(
      // PTP Header
      "10" // PTP Type (illegal)
      "01" // PTP Version
      "00 2c" // msg length
      "00 00 04 00"
      "00 00 00 00 12 34 56 78" // correction field
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ PTPHeader ptpHdr(&cursor); }, HdrParseError);
}

TEST(PTPHdrTest, writePacket) {
  uint8_t ptpType = static_cast<uint8_t>(PTPMessageType::PTP_DELAY_REQUEST);
  uint8_t ptpVersion = static_cast<uint8_t>(PTPVersion::PTP_V2);

  IOBuf buf = IOBuf(IOBuf::CREATE, PTP_DELAY_REQUEST_MSG_SIZE);
  buf.append(PTP_DELAY_REQUEST_MSG_SIZE);
  PTPHeader ptpHdr(ptpType, ptpVersion, 0x1234);
  RWPrivateCursor cursor(&buf);
  ptpHdr.write(&cursor);

  Cursor receiver(&buf);
  PTPHeader ptpHdr1(&receiver);
  ;

  EXPECT_EQ(ptpHdr.ptpType_, ptpHdr1.ptpType_);
  EXPECT_EQ(ptpHdr.ptpVersion_, ptpHdr1.ptpVersion_);
  EXPECT_EQ(ptpHdr.ptpCorrectionField_, ptpHdr1.ptpCorrectionField_);
  EXPECT_EQ(PTP_DELAY_REQUEST_MSG_SIZE, ptpHdr1.ptpMessageLength_);
}
