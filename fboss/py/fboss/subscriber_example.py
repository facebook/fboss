from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import argparse
import time

from fboss.pcap_subscriber import PcapSubscriber, PcapListener


class TestSubscriber(PcapSubscriber):
    def __init__(self, port):
        super().__init__(port)
        self.rxData = 0
        self.txData = 0

    def receiveRxPacket(self, packetdata):
        self.rxData += 1

    def receiveTxPacket(self, packetdata):
        self.txData += 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("remote", help="host to subcribe to")
    parser.add_argument("port", help="port to open a connection on", type=int)
    args = parser.parse_args()

    # create a subscriber
    sub = TestSubscriber(args.port)
    # create a server
    server = PcapListener(sub)
    # subscribe and open a connection
    server.open_connection(args.remote)
    time.sleep(5)
    print("Packets per second: {}".format((sub.rxData + sub.txData) / 5))
    server.server_thread.join()


if __name__ == '__main__':
    main()
