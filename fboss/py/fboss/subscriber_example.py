from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import argparse

from fboss.pcap_subscriber import PcapSubscriber, PcapListener


class TestSubscriber(PcapSubscriber):
    def __init__(self, port):
        super().__init__(port)

    def receiveRxPacket(self, packetdata):
        print("got Rx Packet!")

    def receiveTxPacket(self, packetdata):
        print("got Tx Packet!")


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
    server.server_thread.join()


if __name__ == '__main__':
    main()
