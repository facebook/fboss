import ctypes
from dataclasses import dataclass, field
from enum import Enum
from typing import List, Optional

from dataclasses_json import dataclass_json


class fbiob_i2c_data(ctypes.Structure):
    _fields_ = [
        ("bus_freq_hz", ctypes.c_uint32),
        ("num_channels", ctypes.c_uint32),
    ]


class spi_dev_info(ctypes.Structure):
    _fields_ = [
        ("modalias", ctypes.c_char * 255),
        ("max_speed_hz", ctypes.c_uint32),
        ("chip_select", ctypes.c_uint16),
    ]


class fbiob_spi_data(ctypes.Structure):
    _fields_ = [
        ("num_spidevs", ctypes.c_uint),
        ("spidevs", spi_dev_info * 1),  # FBIOB_SPIDEV_MAX is 1
    ]


class fbiob_led_data(ctypes.Structure):
    _fields_ = [
        ("port_num", ctypes.c_int),
        ("led_idx", ctypes.c_int),
    ]


class fbiob_xcvr_data(ctypes.Structure):
    _fields_ = [
        ("port_num", ctypes.c_uint32),
    ]


class fbiob_fan_data(ctypes.Structure):
    _fields_ = [
        ("num_fans", ctypes.c_uint32),
    ]


class fbiob_aux_id(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char * 255),
        ("id", ctypes.c_uint32),
    ]


class fbiob_aux_data(ctypes.Structure):
    class _u(ctypes.Union):
        _fields_ = [
            ("fan_data", fbiob_fan_data),
            ("i2c_data", fbiob_i2c_data),
            ("led_data", fbiob_led_data),
            ("spi_data", fbiob_spi_data),
            ("xcvr_data", fbiob_xcvr_data),
        ]

    _fields_ = [
        ("id", fbiob_aux_id),
        ("csr_offset", ctypes.c_uint32),
        ("iobuf_offset", ctypes.c_uint32),
        ("u", _u),
    ]


@dataclass_json
@dataclass
class I2cInfo:
    busFreqHz: int | None = None
    numChannels: int | None = None


@dataclass_json
@dataclass
class SpiConfig:
    modalias: str
    chipSelect: int
    maxSpeedHz: int


@dataclass_json
@dataclass
class SpiInfo:
    numSpidevs: int
    spiDevices: list[SpiConfig]


@dataclass_json
@dataclass
class LedInfo:
    portNumber: int
    ledId: int


@dataclass_json
@dataclass
class XcvrInfo:
    portNumber: int


@dataclass_json
@dataclass
class FanInfo:
    numFans: int


class AuxDeviceType(Enum):
    FAN = "FAN"
    I2C = "I2C"
    SPI = "SPI"
    LED = "LED"
    XCVR = "XCVR"


@dataclass_json
@dataclass
class AuxDevice:
    type: AuxDeviceType
    deviceName: str
    csrOffset: str | None = ""
    iobufOffset: str | None = ""
    fanInfo: FanInfo | None = None
    i2cInfo: I2cInfo | None = None
    spiInfo: SpiInfo | None = None
    ledInfo: LedInfo | None = None
    xcvrInfo: XcvrInfo | None = None


@dataclass_json
@dataclass
class I2CDumpData:
    start: str
    end: str
    expected: list[str]


@dataclass_json
@dataclass
class I2CGetData:
    reg: str
    expected: str


@dataclass_json
@dataclass
class I2CSetData:
    reg: str
    value: str


@dataclass_json
@dataclass
class I2CTestData:
    i2cDumpData: list[I2CDumpData] = field(default_factory=list)
    i2cGetData: list[I2CGetData] = field(default_factory=list)
    i2cSetData: list[I2CSetData] = field(default_factory=list)


@dataclass_json
@dataclass
class GpioLineInfo:
    name: str
    direction: str
    getValue: int | None = None


@dataclass_json
@dataclass
class GpioTestData:
    numLines: int
    lines: list[GpioLineInfo]


@dataclass_json
@dataclass
class HwmonTestData:
    expectedFeatures: list[str] = field(default_factory=list)


@dataclass_json
@dataclass
class WatchdogTestData:
    numWatchdogs: int


@dataclass_json
@dataclass
class LedTestData:
    expectedColors: list[str]
    ledType: str | None = None
    ledId: int | None = None


@dataclass_json
@dataclass
class I2CDevice:
    channel: int
    deviceName: str
    address: str
    testData: I2CTestData | None = None
    hwmonTestData: HwmonTestData | None = None
    gpioTestData: GpioTestData | None = None
    watchdogTestData: WatchdogTestData | None = None
    ledTestData: list[LedTestData] | None = None


@dataclass_json
@dataclass
class I2CAdapter:
    auxDevice: AuxDevice
    i2cDevices: list[I2CDevice] = field(default_factory=list)


@dataclass_json
@dataclass
class LedCtrlInfo:
    auxDevice: AuxDevice
    ledTestData: LedTestData


@dataclass_json
@dataclass
class FpgaSpec:
    name: str
    vendorId: str
    deviceId: str
    subSystemVendorId: str
    subSystemDeviceId: str
    i2cAdapters: list[I2CAdapter] = field(default_factory=list)
    xcvrCtrls: list[AuxDevice] = field(default_factory=list)
    ledCtrls: list[LedCtrlInfo] = field(default_factory=list)
    auxDevices: list[AuxDevice] = field(default_factory=list)


def print_fields(obj, indent=0):
    for field_name, field_type in obj._fields_:
        value = getattr(obj, field_name)
        if issubclass(field_type, (ctypes.Structure, ctypes.Union)):
            print("  " * indent + field_name + ":")
            print_fields(value, indent + 1)
        elif issubclass(field_type, ctypes.Array) and issubclass(
            field_type._type_, ctypes.Structure
        ):
            print("  " * indent + field_name + ":")
            for i, item in enumerate(value):
                print("  " * (indent + 1) + f"[{i}]: ")
                print_fields(item, indent + 2)
        else:
            print("  " * indent + field_name + ": " + str(value))


def get_empty_aux_data() -> fbiob_aux_data:
    aux_data = fbiob_aux_data()
    return aux_data


def get_invalid_aux_data() -> fbiob_aux_data:
    aux_data = fbiob_aux_data()
    aux_data.csr_offset = ctypes.c_uint32(-1).value
    return aux_data


def get_aux_data(fpga: FpgaSpec, device: AuxDevice, id: int) -> fbiob_aux_data:
    aux_data = fbiob_aux_data()
    aux_data.id.name = device.deviceName.encode()
    aux_data.id.id = id
    if device.csrOffset:
        aux_data.csr_offset = int(device.csrOffset, 16)
    if device.iobufOffset:
        aux_data.iobuf_offset = int(device.iobufOffset, 16)
    if device.type == AuxDeviceType.FAN:
        assert device.fanInfo
        aux_data.u.fan_data.num_fans = device.fanInfo.numFans
    elif device.type == AuxDeviceType.I2C:
        assert device.i2cInfo
        if device.i2cInfo.busFreqHz:
            aux_data.u.i2c_data.bus_freq_hz = device.i2cInfo.busFreqHz
        if device.i2cInfo.numChannels:
            aux_data.u.i2c_data.num_channels = device.i2cInfo.numChannels
    elif device.type == AuxDeviceType.SPI:
        assert device.spiInfo
        spiInfo = device.spiInfo
        aux_data.u.spi_data.num_spidevs = spiInfo.numSpidevs
        for i in range(spiInfo.numSpidevs):
            spi_dev = spi_dev_info()
            spi_dev.modalias = spiInfo.spiDevices[i].modalias
            spi_dev.max_speed_hz = spiInfo.spiDevices[i].maxSpeedHz
            spi_dev.chip_select = device
    elif device.type == AuxDeviceType.LED:
        assert device.ledInfo
        aux_data.u.led_data.port_num = device.ledInfo.portNumber
        aux_data.u.led_data.led_idx = device.ledInfo.ledId
    elif device.type == AuxDeviceType.XCVR:
        assert device.xcvrInfo
        aux_data.u.led_data.port_num = device.xcvrInfo.portNumber
        aux_data.u.xcvr_data.port_num = device.xcvrInfo.portNumber
    return aux_data
