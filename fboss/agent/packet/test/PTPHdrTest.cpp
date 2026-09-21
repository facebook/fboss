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

  EXPECT_EQ(static_cast<PTPMessageType>(ptpType), ptpHdr.getPtpType());
  EXPECT_EQ(static_cast<PTPVersion>(ptpVersion), ptpHdr.getPtpVersion());
  EXPECT_EQ(0x1234, ptpHdr.getCorrectionField());
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

  EXPECT_EQ(ptpHdr.getPtpType(), static_cast<PTPMessageType>(0x01));
  EXPECT_EQ(ptpHdr.getPtpVersion(), static_cast<PTPVersion>(0x01));
  EXPECT_EQ(ptpHdr.getCorrectionField(), 0x12345678);
  EXPECT_EQ(ptpHdr.getPtpMessageLength(), 0x2c);
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

  EXPECT_EQ(ptpHdr.getPtpType(), ptpHdr1.getPtpType());
  EXPECT_EQ(ptpHdr.getPtpVersion(), ptpHdr1.getPtpVersion());
  EXPECT_EQ(ptpHdr.getCorrectionField(), ptpHdr1.getCorrectionField());
  EXPECT_EQ(PTP_DELAY_REQUEST_MSG_SIZE, ptpHdr1.getPtpMessageLength());
}
