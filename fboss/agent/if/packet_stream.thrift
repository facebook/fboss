namespace cpp2 facebook.fboss
namespace py3 neteng.fboss

include "common/fb303/if/fb303.thrift"

struct Packet {
  1: i64 timestamp
  2: string l2Port
  3: binary buf
}

enum ErrorCode {
  INVALID_L2PORT = 1,
  CLIENT_NOT_CONNECTED = 2,
  STREAM_NOT_FOUND = 3,
  INTERNAL_ERROR = 4,
  PORT_NOT_REGISTERED = 5,
  INVALID_CLIENT = 6,
}

exception PacketException {
  1: ErrorCode code,
  2: string msg,
}

service PacketStream extends fb303.FacebookService {
  stream<Packet throws (1: PacketException ex)> connect(1: string clientId)
    throws (1: PacketException ex)
  void registerPort(1: string clientId, 2: string l2Port)
    throws (1: PacketException ex)
  void clearPort(1: string clientId, 2: string l2Port)
    throws (1: PacketException ex)
  void disconnect(1: string clientId) throws (1: PacketException ex)
}
