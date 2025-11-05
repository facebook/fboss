#!/usr/bin/env python3
# Copyright 2021-present Facebook. All Rights Reserved.
"""
Thrift-based Modbus/rackmond library for x86 platforms
This is the Thrift equivalent of pyrmd.py for BMC platforms
"""

import struct
import time
from typing import Optional, Union, List

from thrift.transport import TSocket, TTransport
from thrift.protocol import TBinaryProtocol
from thrift.Thrift import TException

from rackmonsvc.rackmonsvc import RackmonCtrl
from rackmonsvc.rackmonsvc.ttypes import (
    RawCommandRequest,
    RawCommandResponse,
    RackmonStatusCode,
    ReadWordRegistersRequest,
    ReadWordRegistersResponse,
    WriteSingleRegisterRequest,
    PresetMultipleRegistersRequest,
    ReadFileRecordRequest,
    ReadFileRecordResponse,
    RackmonControlRequest,
    MonitorDataFilter,
    DeviceFilter,
    RegisterFilter,
)

class ModbusException(Exception):
    pass


class ModbusTimeout(ModbusException):
    def __init__(self):
        super().__init__("ERR_TIMEOUT")


class ModbusCRCError(ModbusException):
    def __init__(self):
        super().__init__("ERR_BAD_CRC")


class ModbusUnknownError(ModbusException):
    def __init__(self):
        super().__init__("ERR_IO_FAILURE")


class ModbusInvalidArgs(ModbusException):
    def __init__(self):
        super().__init__("ERR_INVALID_ARGS")


# Logging support (matches pyrmd.py)
modbuslog = None
logstart = None


def log(*args, **kwargs):
    if modbuslog:
        t = time.time() - logstart
        if all(args):
            pfx = "[{:4.02f}]".format(t)
        else:
            pfx = ""
        print(pfx, *args, file=modbuslog, **kwargs)


class RackmonThriftClient:
    """
    Thrift client for rackmon service.
    Maintains connection to the rackmon Thrift server.
    """

    def __init__(self, host='localhost', port=5973):
        self.host = host
        self.port = port
        self.transport = None
        self.client = None
        self._connect()

    def _connect(self):
        """Establish connection to Thrift server"""
        try:
            socket = TSocket.TSocket(self.host, self.port)
            socket.setTimeout(30000)  # 30 second timeout
            self.transport = TTransport.TBufferedTransport(socket)
            protocol = TBinaryProtocol.TBinaryProtocol(self.transport)
            self.client = RackmonCtrl.Client(protocol)
            self.transport.open()
        except TException as e:
            raise ModbusException(f"Failed to connect to rackmon at {self.host}:{self.port}: {e}")

    def _ensure_connected(self):
        """Reconnect if connection was lost"""
        if not self.transport or not self.transport.isOpen():
            self._connect()

    def close(self):
        """Close the connection"""
        if self.transport:
            self.transport.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    @staticmethod
    def _check_status(status: RackmonStatusCode):
        """Convert Thrift status code to exception"""
        if status == RackmonStatusCode.SUCCESS:
            return
        elif status == RackmonStatusCode.ERR_BAD_CRC:
            log("<- crc check failure")
            raise ModbusCRCError()
        elif status == RackmonStatusCode.ERR_TIMEOUT:
            log("<- timeout")
            raise ModbusTimeout()
        elif status == RackmonStatusCode.ERR_INVALID_ARGS:
            log("<- invalid args")
            raise ModbusInvalidArgs()
        else:
            log("<-", status)
            raise ModbusException(f"Error: {status}")

    def raw(
        self,
        raw_cmd: Union[bytes, bytearray],
        expected: int,
        timeout: int = 0,
        fullResp: bool = False,
        unique_addr: Optional[int] = None
    ) -> Union[bytes, List[int]]:
        """
        Send raw Modbus command.

        Args:
            raw_cmd: Raw Modbus bytes (including address and function code)
            expected: Expected response length in bytes (including CRC)
            timeout: Timeout in milliseconds (0 = default)
            fullResp: If True, return full response including CRC. If False, exclude CRC.
            unique_addr: Optional unique device address

        Returns:
            Response bytes (excluding CRC by default) as bytes if raw_cmd was bytes,
            or as list of ints if raw_cmd was not bytes
        """
        self._ensure_connected()

        # Build request
        req = RawCommandRequest()
        # Convert bytes to list of signed bytes (Thrift byte is -128 to 127)
        req.cmd = [b if b < 128 else b - 256 for b in raw_cmd]
        req.responseLength = expected

        if timeout > 0:
            req.timeout = timeout

        if unique_addr is not None:
            req.uniqueDevAddress = unique_addr

        log("-> {}".format([hex(b & 0xFF) for b in req.cmd]))

        # Execute
        try:
            resp = self.client.sendRawCommand(req)
        except TException as e:
            raise ModbusException(f"Thrift error: {e}")

        # Check status
        self._check_status(resp.status)

        log("<- {}".format([hex(b & 0xFF) for b in resp.data]))

        # resp.data already excludes CRC (as documented in Thrift interface)
        # If fullResp requested, we can't add CRC back (we don't have it)
        # For now, fullResp just returns what we have
        ret = resp.data

        # Convert to bytes if input was bytes
        if isinstance(raw_cmd, bytes):
            # Thrift byte is signed (-128 to 127), convert to unsigned
            return bytes((b & 0xFF) for b in ret)
        return ret


class RackmonInterface:
    """
    Stateless interface to rackmon (creates new connection per call).
    Compatible with pyrmd.py API for easier migration.
    """

    _client: Optional[RackmonThriftClient] = None
    _host = 'localhost'
    _port = 5973

    @classmethod
    def configure(cls, host='localhost', port=5973):
        """Configure connection parameters"""
        cls._host = host
        cls._port = port
        if cls._client:
            cls._client.close()
            cls._client = None

    @classmethod
    def _get_client(cls) -> RackmonThriftClient:
        """Get or create persistent client"""
        if cls._client is None:
            cls._client = RackmonThriftClient(cls._host, cls._port)
        return cls._client

    @classmethod
    def raw(
        cls,
        raw_cmd: Union[bytes, bytearray],
        expected: int,
        timeout: int = 0,
        fullResp: bool = False,
        unique_addr: Optional[int] = None
    ) -> Union[bytes, List[int]]:
        """
        Send raw Modbus command (compatible with pyrmd.py).

        Args:
            raw_cmd: Raw Modbus bytes
            expected: Expected response length
            timeout: Timeout in ms
            fullResp: Include CRC in response
            unique_addr: Optional unique device address

        Returns:
            Response bytes
        """
        client = cls._get_client()
        return client.raw(raw_cmd, expected, timeout, fullResp, unique_addr)

    @classmethod
    def pause(cls):
        """Pause rackmon monitoring"""
        client = cls._get_client()
        client._ensure_connected()
        try:
            status = client.client.controlRackmond(RackmonControlRequest.PAUSE_RACKMOND)
            client._check_status(status)
        except TException as e:
            raise ModbusException(f"Thrift error: {e}")

    @classmethod
    def resume(cls):
        """Resume rackmon monitoring"""
        client = cls._get_client()
        client._ensure_connected()
        try:
            status = client.client.controlRackmond(RackmonControlRequest.RESUME_RACKMOND)
            client._check_status(status)
        except TException as e:
            raise ModbusException(f"Thrift error: {e}")

    @classmethod
    def rescan(cls):
        """Force rackmon to rescan devices"""
        client = cls._get_client()
        client._ensure_connected()
        try:
            status = client.client.controlRackmond(RackmonControlRequest.RESCAN)
            client._check_status(status)
        except TException as e:
            raise ModbusException(f"Thrift error: {e}")

    @classmethod
    def list(cls):
        """List all Modbus devices"""
        client = cls._get_client()
        client._ensure_connected()
        try:
            devices = client.client.listModbusDevices()
            return devices
        except TException as e:
            raise ModbusException(f"Thrift error: {e}")

    @classmethod
    def read(cls, addr: int, reg: int, length: int = 1, timeout: int = 0) -> List[int]:
        """
        Read holding registers.

        Args:
            addr: Device address
            reg: Register address
            length: Number of registers to read
            timeout: Timeout in ms

        Returns:
            List of register values
        """
        client = cls._get_client()
        client._ensure_connected()

        req = ReadWordRegistersRequest()
        req.devAddress = addr
        req.regAddress = reg
        req.numRegisters = length
        if timeout > 0:
            req.timeout = timeout

        try:
            resp = client.client.readHoldingRegisters(req)
            client._check_status(resp.status)
            return resp.regValues
        except TException as e:
            raise ModbusException(f"Thrift error: {e}")

    @classmethod
    def write(cls, addr: int, reg: int, data: Union[int, List[int]], timeout: int = 0):
        """
        Write register(s).

        Args:
            addr: Device address
            reg: Register address
            data: Single value (int) or list of values
            timeout: Timeout in ms
        """
        client = cls._get_client()
        client._ensure_connected()

        try:
            if isinstance(data, int):
                # Write single register
                req = WriteSingleRegisterRequest()
                req.devAddress = addr
                req.regAddress = reg
                req.regValue = data
                if timeout > 0:
                    req.timeout = timeout
                status = client.client.writeSingleRegister(req)
            else:
                # Write multiple registers
                req = PresetMultipleRegistersRequest()
                req.devAddress = addr
                req.regAddress = reg
                req.regValue = data
                if timeout > 0:
                    req.timeout = timeout
                status = client.client.presetMultipleRegisters(req)

            client._check_status(status)
        except TException as e:
            raise ModbusException(f"Thrift error: {e}")


# For backward compatibility, create an alias
RackmonAsyncInterface = RackmonInterface  # Thrift doesn't need separate async
