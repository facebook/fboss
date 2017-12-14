#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from neteng.fboss.ctrl import FbossCtrl
from neteng.fboss.qsfp import QsfpService
from neteng.fboss.pcap_pubsub import PcapPushSubscriber
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TSocket import TSocket
from thrift.Thrift import TApplicationException


class FbossAgentClient(FbossCtrl.Client):
    DEFAULT_PORT = 5909

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

    #
    # The getPortStats() thrift API was unfortunately renamed to getPortInfo().
    # Here's a hacky workaround that tries to do the right thing regardless of
    # whether the switch we are talking to supports getPortStats() or
    # getPortInfo().
    #

    def getPortStats(self, *args, **kwargs):
        return self.getPortInfo(*args, **kwargs)

    def getAllPortStats(self, *args, **kwargs):
        return self.getAllPortInfo(*args, **kwargs)

    def getPortInfo(self, *args, **kwargs):
        try:
            return FbossCtrl.Client.getPortInfo(self, *args, **kwargs)
        except TApplicationException as ex:
            if 'Method name getPortInfo not found' in str(ex):
                return FbossCtrl.Client.getPortStats(self, *args, **kwargs)
            raise

    def getAllPortInfo(self, *args, **kwargs):
        try:
            return FbossCtrl.Client.getAllPortInfo(self, *args, **kwargs)
        except TApplicationException as ex:
            if 'Method name getAllPortInfo not found' in str(ex):
                return FbossCtrl.Client.getAllPortStats(self, *args, **kwargs)
            raise


class PcapPushSubClient(PcapPushSubscriber.Client):
    DEFAULT_PORT = 5911

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
        PcapPushSubscriber.Client.__init__(self, self._protocol)

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
