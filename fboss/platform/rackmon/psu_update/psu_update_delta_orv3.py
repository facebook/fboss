#!/usr/bin/env python3

import argparse
import json
import os
import os.path
import struct
import sys
import time
import traceback
from binascii import hexlify
from contextlib import ExitStack

from fboss.platform.rackmon.psu_update import hexfile
from fboss.platform.rackmon.psu_update.modbus_update_helper import (
    auto_int,
    bh,
    decode_modbus_address,
    get_parser,
    print_perc,
    suppress_monitoring,
)
from fboss.platform.rackmon.psu_update.pyrmd_thrift import ModbusException, RackmonInterface as rmd

from fboss.platform.rackmon.psu_update import delta_key

parser = get_parser()
parser.add_argument("--key", type=auto_int, default=delta_key.key, help="Sec key")


class StatusRegister:
    _fields_ = [
        # Byte 0
        "WAIT",
        "START_PROG_ACCEPTED",
        "START_PROG_DECLINED",
        "KEY_ACCEPTED",
        "KEY_DECLINED",
        "ERASE_BUSY",
        "ERASE_DONE",
        "ERASE_FAIL",
        # Byte 1
        "ADD_ACCEPTED",
        "ADD_DECLINED",
        "SEND_DATA_BUSY",
        "SEND_DATA_RDY",
        "SEND_DATA_FAIL",
        "VERIFY_CRC_BUSY",
        "CRC_VERIFIED",
        "CRC_WRONG",
        # Byte 2
        "FW_IMAGE_ACCEPTED",
        "FW_IMAGE_DECLINED",
        "RESERVED",
        "RESERVED",
        "DEV_UPD_BUSY",
        "DEV_UPD_RDY",
        "DEV_UPD_FAIL",
        "RESERVED",
        # Byte 3
        "RESERVED",
        "RESERVED",
        "RESERVED",
        "RESERVED",
        "REV_FLAG",
        "COMPATIBILITY_ERROR",
        "SEQUENCE_ERROR",
        "ERROR_DETECTED",
    ]

    def __init__(self, val):
        if isinstance(val, int):
            self.val = val
        elif isinstance(val, bytes) or isinstance(val, bytearray):
            (self.val,) = struct.unpack(">L", val)

    def __getitem__(self, name):
        return (self.val & (1 << self._fields_.index(name))) != 0

    def __str__(self):
        return str(
            [
                (name, (self.val & (1 << idx)) != 0)
                for idx, name in enumerate(self._fields_)
            ]
        )


class BadMEIResponse(ModbusException):
    pass


def get_status_reg(addr):
    addr_b, uaddr = decode_modbus_address(addr)
    req = addr_b + b"\x2B\x64\x22\x00\x00"
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    exp_resp = addr_b + b"\x2B\x71\x62\x00\x00"
    if len(resp) != 10 or resp[:6] != exp_resp:
        print("Bad status response: " + bh(resp))
        raise BadMEIResponse()
    return StatusRegister(resp[6:])


def wait_status(addr, bit_set=None, bit_cleared=None, delay=1.0, timeout=100.0):
    timeout_ms = int(timeout * 1000)
    delay_ms = int(delay * 1000)
    start = time.monotonic()
    for _ in range(0, timeout_ms, delay_ms):
        fstatus = get_status_reg(addr)
        if bit_set is not None and fstatus[bit_set]:
            return fstatus
        if bit_cleared is not None and not fstatus[bit_cleared]:
            return fstatus
        time.sleep(delay)
    dur = time.monotonic() - start
    print(
        "Waiting for set:",
        bit_set,
        " cleared:",
        bit_cleared,
        " timeout after(sec):",
        timeout,
        "waited (sec):",
        dur,
    )
    raise Exception(fstatus)


def get_challenge(addr):
    print("Send get seed")
    addr_b, uaddr = decode_modbus_address(addr)
    req = addr_b + b"\x2B\x64\x27\x00\x00"
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    exp_resp = addr_b + b"\x2B\x71\x67\x00\x00"
    if len(resp) != 10 or resp[:6] != exp_resp:
        print("Bad challenge response: " + bh(resp))
        raise BadMEIResponse()
    challenge = resp[6:]
    print("Got seed: " + bh(challenge))
    return challenge


def send_key(addr, key):
    print("Send key")
    addr_b, uaddr = decode_modbus_address(addr)
    req = addr_b + b"\x2B\x64\x27\x00\x01" + key
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    exp_resp = addr_b + b"\x2b\x71\x67\x00\x01\xff\xff\xff\xff"
    if resp != exp_resp:
        print("Bad key response: " + bh(resp))
        raise BadMEIResponse()
    print("Send key successful.")


def delta_seccalckey(challenge, key):
    lower = key & 0xFFFFFFFF
    upper = (key >> 32) & 0xFFFFFFFF
    (seed,) = struct.unpack(">L", challenge)
    for _ in range(32):
        if seed & 1 != 0:
            seed = seed ^ lower
        seed = (seed >> 1) & 0x7FFFFFFF
    seed = seed ^ upper
    return struct.pack(">L", seed)


def key_handshake(addr, key):
    challenge = get_challenge(addr)
    send_key(addr, delta_seccalckey(challenge, key))


def erase_flash(addr):
    print("Erasing flash... ")
    addr_b, uaddr = decode_modbus_address(addr)
    sys.stdout.flush()
    req = addr_b + b"\x2B\x64\x31\x00\x00\xFF\xFF\xFF\xFF"
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    exp_resp = addr_b + b"\x2B\x71\x71\xFF\xFF\xFF\xFF\xFF\xFF"
    if resp != exp_resp:
        print("Bad erase response: " + bh(resp))
        raise BadMEIResponse()
    time.sleep(1.5)
    fstatus = get_status_reg(addr)
    if fstatus["ERASE_DONE"]:
        print("Erase successful")
    else:
        print("Erase failed")
        raise Exception(str(fstatus))


def set_write_address(psu_addr, flash_addr):
    addr_b, uaddr = decode_modbus_address(psu_addr)
    req = addr_b + b"\x2B\x64\x34\x00\x00" + struct.pack(">L", flash_addr)
    exp_resp = addr_b + b"\x2B\x71\x74\xFF\xFF\xFF\xFF\xFF\xFF"
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    if resp != exp_resp:
        print("Bad set write addr response: " + bh(resp))
        raise BadMEIResponse()
    wait_status(psu_addr, bit_set="ADD_ACCEPTED")


def write_data(addr, data):
    assert len(data) == 128
    addr_b, uaddr = decode_modbus_address(addr)
    req = addr_b + b"\x2B\x65\x36" + data
    exp_resp = addr_b + b"\x2B\x73\x76\xFF\xFF\xFF\xFF\xFF\xFF"
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    if resp != exp_resp:
        print("Bad write data response: " + bh(resp))
        raise BadMEIResponse()
    time.sleep(0.05)
    # Wait till SEND_DATA_RDY is set.
    fstatus = wait_status(addr, bit_cleared="SEND_DATA_BUSY", timeout=5, delay=0.05)
    if fstatus["SEND_DATA_BUSY"]:
        print("Write data busy after 5s")
        raise Exception(str(fstatus))
    # If send data rdy is set, return immediately, else wait for it
    if fstatus["SEND_DATA_RDY"]:
        return
    fstatus = wait_status(addr, bit_set="SEND_DATA_RDY", timeout=5, delay=0.05)
    if not fstatus["SEND_DATA_RDY"]:
        print("Write data failed")
        raise Exception(str(fstatus))


def verify_flash(addr):
    print("Verifying program...")
    addr_b, uaddr = decode_modbus_address(addr)
    req = addr_b + b"\x2B\x64\x31\x00\x01"
    exp_resp = addr_b + b"\x2B\x71\x71\xFF\xFF\xFF\xFF\xFF\xFF"
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    if resp != exp_resp:
        print("Bad write data response: " + bh(resp))
        raise BadMEIResponse()
    time.sleep(0.1)
    # Wait till VERIFY_CRC_BUSY is cleared.
    fstatus = wait_status(addr, bit_cleared="VERIFY_CRC_BUSY")
    if not fstatus["CRC_VERIFIED"]:
        raise Exception(str(fstatus))
    print("Verify of flash successful!")


def activate(addr):
    print("Activating Image...")
    addr_b, uaddr = decode_modbus_address(addr)
    req = addr_b + b"\x2B\x64\x2E\x00\x00"
    exp_resp = addr_b + b"\x2B\x71\x6E\xFF\xFF\xFF\xFF\xFF\xFF"
    resp = rmd.raw(req, expected=12, unique_addr=uaddr)
    if resp != exp_resp:
        print("Bad activate response: " + bh(resp))
        raise BadMEIResponse()
    print("Activate successful!")


def send_image(addr, fwimg):
    chunk_size = 128
    total_chunks = sum([len(s) for s in fwimg.segments]) / chunk_size
    sent_chunks = 0
    for s in fwimg.segments:
        segment_name = str(s)
        segment_size = len(s)
        if segment_size == 0:
            print("Ignoring empty segment:", segment_name)
            continue
        print("Sending " + segment_name)
        set_write_address(addr, s.start_address)
        for i in range(0, len(s), chunk_size):
            chunk = s.data[i : i + chunk_size]
            if len(chunk) < chunk_size:
                chunk = chunk + (b"\xFF" * (chunk_size - len(chunk)))
            sent_chunks += 1
            # dont fill the restapi log with junk
            print_perc(
                sent_chunks * 100.0 / total_chunks,
                "Sending chunk %d of %d..." % (sent_chunks, total_chunks),
            )
            write_data(addr, bytearray(chunk))
    print_perc(100.0, "Sending chunk %d of %d..." % (total_chunks, total_chunks))


def update_psu(addr, filename, key):
    print("Parsing Firmware")
    fwimg = hexfile.load(filename)
    key_handshake(addr, key)
    erase_flash(addr)
    send_image(addr, fwimg)
    verify_flash(addr)
    activate(addr)


def print_revision(addr):
    # For x86, read firmware revision from holding registers
    # PSU_FW_Revision is at register 0x30
    try:
        values = rmd.read(addr, 0x0030, 4, timeout=2000)
        # Convert register values to ASCII string
        fw_version = ""
        for val in values:
            fw_version += chr((val >> 8) & 0xFF)
            fw_version += chr(val & 0xFF)
        print("Version:", fw_version.strip('\x00'))
    except Exception as e:
        print("Could not read version:", e)


def main():
    args = parser.parse_args()
    if args.key is None:
        print("PSU Update Key is needed to upgrade this device")
        sys.exit(1)
    with suppress_monitoring(args.addr):
        try:
            print_revision(args.addr)
            update_psu(args.addr, args.file, args.key)
        except Exception as e:
            print("Firmware update failed %s" % str(e))
            print("Status register dump:")
            print(get_status_reg(args.addr.to_bytes(1, "big")))
            traceback.print_exc()
            sys.exit(1)
    print("Upgrade Success!")
    print_revision(args.addr)


if __name__ == "__main__":
    main()
