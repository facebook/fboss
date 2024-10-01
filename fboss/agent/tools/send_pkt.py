#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import binascii
import re
import sys

from fboss.py.fboss.thrift_clients import (
    PlainTextFbossAgentClientDontUseInFb as PlainTextFbossAgentClient,
)


def load_hex_file(path):
    lines = []
    with open(path) as f:
        for line_num, line in enumerate(f):
            if line.startswith("#"):
                continue
            line = re.sub(r"[\t\n ]+", "", line)
            try:
                raw_line = binascii.a2b_hex(line)
            except Exception as ex:
                raise Exception("error parsing line {}: {}".format(line_num, ex))
            lines.append(raw_line)
    return b"".join(lines)


def load_raw_file(path):
    with open(path) as f:
        return f.read()


def main(get_client):
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "-s",
        "--sim-host",
        default="::1",
        help="The thrift host the simulator is listening on",
    )
    ap.add_argument(
        "-p",
        "--sim-port",
        type=int,
        default=None,
        help="The thrift port the simulator is listening on",
    )
    ap.add_argument(
        "-P", "--port", type=int, default=1, help="The port to send the packet to"
    )
    ap.add_argument(
        "-V", "--vlan", type=int, default=1, help="The VLAN to send the packet to"
    )
    ap.add_argument("-f", "--hex-file", help="A file containing hex packet data")
    ap.add_argument("-r", "--raw-file", help="A file containing raw packet data")
    args = ap.parse_args()

    if args.hex_file and args.raw_file:
        ap.error("only one of --hex-file and --raw-file may be specified")
    if args.hex_file:
        data = load_hex_file(args.hex_file)
    elif args.raw_file:
        data = load_raw_file(args.raw_file)
    else:
        ap.error("must specify either --hex-file or --raw-file")

    with get_client(args.sim_host, args.sim_port) as client:
        client.sendPkt(args.port, args.vlan, data)


if __name__ == "__main__":
    rc = main(PlainTextFbossAgentClient)
    sys.exit(rc)
