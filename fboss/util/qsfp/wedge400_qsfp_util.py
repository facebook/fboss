"""
This utility provides access to QSFP modules in the Wedge400 platform. All the
optics modules can be enabled/disabled through the controlling FPGA using this
utility
"""

import argparse
import mmap
import subprocess
from ctypes import c_uint32


class FpgaDevice:
    def __init__(self):
        pciBusStr = subprocess.check_output(
            "lspci -d 1d9b:7012 | sed -e 's/ .*//'", shell=True, text=True
        )
        pciBusList = pciBusStr.strip().split("\n")
        if len(pciBusList) != 2:
            print("The FPGA not detected correctly, exiting")
            exit()

        # Memory map for first FPGA
        resourceFile1 = "/sys/bus/pci/devices/0000:" + pciBusList[0] + "/resource0"
        print(resourceFile1)
        self.f1 = open(resourceFile1, "rb+")
        fpgaMap1 = mmap.mmap(
            self.f1.fileno(),
            0x8000,
            flags=mmap.MAP_SHARED,
            prot=mmap.PROT_WRITE | mmap.PROT_READ,
            offset=0,
        )
        self.memStart1 = (c_uint32 * 0x2000).from_buffer(fpgaMap1)

        # Memory map for second FPGA
        resourceFile2 = "/sys/bus/pci/devices/0000:" + pciBusList[1] + "/resource0"
        print(resourceFile2)
        self.f2 = open(resourceFile2, "rb+")
        fpgaMap2 = mmap.mmap(
            self.f2.fileno(),
            0x8000,
            flags=mmap.MAP_SHARED,
            prot=mmap.PROT_WRITE | mmap.PROT_READ,
            offset=0,
        )
        self.memStart2 = (c_uint32 * 0x2000).from_buffer(fpgaMap2)

    def __del__(self):
        self.f1.close()
        self.f2.close()

    def read_fpga(self, fpgaId, regOffset) -> c_uint32:
        offset = regOffset >> 2
        if fpgaId == 0:
            return self.memStart1[offset]
        else:
            return self.memStart2[offset]

    def write_fpga(self, fpgaId, regOffset, regVal):
        offset = regOffset >> 2
        if fpgaId == 0:
            self.memStart1[offset] = regVal
        else:
            self.memStart2[offset] = regVal


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--info", action="store_true", default=False)
    ap.add_argument("--enable", action="store_true", default=False)
    ap.add_argument("--disable", action="store_true", default=False)

    args = ap.parse_args()
    fpga = FpgaDevice()

    if args.info:
        regVal = fpga.read_fpga(0, 0x0070)
        print("Fpga1 Qsfp reset bitmap:", hex(regVal))
        regVal = fpga.read_fpga(0, 0x0078)
        print("Fpga1 Qsfp Low Power bitmap:", hex(regVal))

        regVal = fpga.read_fpga(1, 0x0070)
        print("Fpga2 Qsfp reset bitmap:", hex(regVal))
        regVal = fpga.read_fpga(1, 0x0078)
        print("Fpga2 Qsfp Low Power bitmap:", hex(regVal))


if __name__ == "__main__":
    main()
