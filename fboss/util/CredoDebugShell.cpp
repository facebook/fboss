// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

extern "C" {
extern int debug_main();
}

using namespace std;

constexpr std::string_view kCredoDebugIntfSocketIp = "127.0.0.1";
constexpr uint16_t kCredoDebugIntfSocketPort(8299);

/*
 * This program connects to a socket interface for predefined address/port where
 * other side is Credo library libsaiowl.a (a part of qsfp_service) listening.
 * It send the command to invoke debug shell functionality to libsaiowl. Then it
 * invokes command line debug shell utility where user can provide
 * commands/options. These commands gets gets sent to libsaiowl over socket
 * interface and response is printed back. Quitting the shell closes socket
 * connection. Note: The qsfp_service with Credo SAI library (libsaiowl) should
 * be running for this utility to work.
 */
int main(int argc, char* argv[]) {
  int sockfd = 0;
  struct sockaddr_in peer;
  std::string msg("debug_shell");

  if (argc > 1 &&
      (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    printf(
        "Use command \'fboss2 show interface phymap\'"
        " to get the PHY Slice Id for a port\n");
    return 0;
  }

  // Create a socket interface for communication
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("Socket create error");
    exit(-1);
  }

  // Connect to loopback address and port for communicating with Credo library
  // running in different process
  peer.sin_addr.s_addr = inet_addr(kCredoDebugIntfSocketIp.data());
  peer.sin_family = AF_INET;
  peer.sin_port = htons(kCredoDebugIntfSocketPort);

  if (connect(sockfd, (struct sockaddr*)&peer, sizeof(peer)) < 0) {
    printf("Socket connect error");
    exit(-1);
  }

  // Send message to Credo library to invoke the debug functionality
  if (send(sockfd, msg.c_str(), msg.length(), 0) < 0) {
    printf("Socket send error");
    exit(-1);
  }
  sleep(2);

  // Start the interactive debug shell in this process
  debug_main();

  // Close the socket after shell exits
  close(sockfd);
  return 0;
}
