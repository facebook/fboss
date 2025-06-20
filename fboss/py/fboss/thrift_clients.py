#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

# pyre-unsafe

from neteng.fboss.ctrl import FbossCtrl
from neteng.fboss.qsfp import QsfpService
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TSocket import TSocket


#################
# This class wraps the autogenerated thrift class.
# All of the functions in fboss/agent/if/ctrl.thrift get inherited into
# here.
#
# For example:
#     with PlainTextFbossAgentClientDontUseInFb(host) as client:
#        print(client.getAllPortInfo())    # dump all of the port stats


class PlainTextFbossAgentClientDontUseInFb(FbossCtrl.Client):
    DEFAULT_PORT = 5909
    DEFAULT_HW_PORT = 5931

    def __init__(self, host, port=None, timeout=5.0):
        self.host = host
        if port is None:
            port = self.DEFAULT_PORT

        self._socket = TSocket(host, port)
        # TSocket.setTimeout() takes a value in milliseconds
        self._socket.setTimeout(timeout * 1000)
        self._transport = THeaderTransport(self._socket)
        self._protocol = THeaderProtocol(self._transport)

        self._transport.open()
        FbossCtrl.Client.__init__(self, self._protocol)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._transport.close()


class QsfpServiceClient(QsfpService.Client):
    DEFAULT_PORT = 5910
    DEFAULT_TIMEOUT = 10.0

    # we ignore the value of port
    def __init__(self, host, port=None, timeout=None):
        # In a box with all 32 QSFP ports populated, it takes about 7.5s right
        # now to read all 32 QSFP ports. So, put the defaut timeout to 10s.
        self.host = host

        timeout = timeout or self.DEFAULT_TIMEOUT
        self._socket = TSocket(host, self.DEFAULT_PORT)
        # TSocket.setTimeout() takes a value in milliseconds
        self._socket.setTimeout(timeout * 1000)
        self._transport = THeaderTransport(self._socket)
        self._protocol = THeaderProtocol(self._transport)

        self._transport.open()
        QsfpService.Client.__init__(self, self._protocol)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._transport.close()
