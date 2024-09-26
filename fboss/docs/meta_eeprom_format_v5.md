# **Meta FBOSS EEPROM Format v5**

All Field Replaceable Unit (FRU) ID EEPROMs must follow the format below. This
replaces the Meta FBOSS EEPROM Format V4. The major changes in this format are
adoption of big-endian format, and flexibility to program multiple MAC addresses
for different purposes.

## **Conventions:**

- All fields are **big endian**.
- String terminator `\0` is **not** included in the string field. The parser
  will determine the string size by reading the length field of TLV entries.

## **Header:**

All Meta EEPROMs must start with the 4-byte header below. This header is
compatible with the past versions.

| Offset (byte) | Length (bytes) | Value  | Description                |
| :-----------: | :------------: | ------ | -------------------------- |
|       0       |       2        | 0xFBFB | Magic word                 |
|       2       |       1        | 0x5    | Meta EEPROM Format Version |
|       3       |       1        | 0XFF   | Reserved for future use    |

## **Body:**

- The body must come after the 4-byte header.
- The body consists of the following well-defined Type-Length-Value (TLV)
  entries.
- There must not be any gap between the TLV entries.
- The `type` field is always **one byte**.
- The `length` field is always **one byte**. It contains the size of the
  corresponding `value` field.
- The CRC field must be the **last entry**. The other TLV entries can be in any
  order.
- The below table only lists the Mandatory fields from the Meta software teams.
  Please check with Meta Manufacturing & Quality team to ensure other Mandatory
  fields are taken care of.

| Type | Length (bytes) | Value                                                                                       | Mandatory (Y/N) | Description                                                                                                                                                  |
| ---- | -------------- | ------------------------------------------------------------------------------------------- | --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 0    | N/A            | N/A                                                                                         | N/A             | Reserved                                                                                                                                                     |
| 1    | Variable       | ASCII String                                                                                | Y               | Product Name                                                                                                                                                 |
| 2    | Variable       | ASCII String                                                                                | N               | Top Level 20- Product Part Number                                                                                                                            |
| 3    | 8              | ASCII String                                                                                | N               | System Assembly Part Number                                                                                                                                  |
| 4    | 12             | ASCII String                                                                                | N               | Meta PCBA Part Number                                                                                                                                        |
| 5    | 12             | ASCII String                                                                                | N               | Meta PCB Part Number                                                                                                                                         |
| 6    | Variable       | ASCII String                                                                                | N               | ODM/JDM PCBA Part Number                                                                                                                                     |
| 7    | Variable       | ASCII String                                                                                | N               | ODM/JDM PCBA Serial Number                                                                                                                                   |
| 8    | 1              | Unsigned Integer                                                                            | Y               | Product Production State                                                                                                                                     |
| 9    | 1              | Unsigned Integer                                                                            | Y               | Product Version                                                                                                                                              |
| 10   | 1              | Unsigned Integer                                                                            | Y               | Product Sub Version                                                                                                                                          |
| 11   | Variable       | ASCII String                                                                                | Y               | Product Serial Number                                                                                                                                        |
| 12   | Variable       | ASCII String                                                                                | N               | System Manufacturer                                                                                                                                          |
| 13   | 8              | ASCII String                                                                                | N               | System Manufacturing Date, in “YYYYMMDD” format.                                                                                                             |
| 14   | Variable       | ASCII String                                                                                | N               | PCB Manufacturer                                                                                                                                             |
| 15   | Variable       | ASCII String                                                                                | N               | Assembled at                                                                                                                                                 |
| 16   | Variable       | ASCII String                                                                                | N               | EEPROM location on Fabric                                                                                                                                    |
| 17   | 8              | 48 bits of Base MAC Address in the first six bytes. Unsigned Integer in the last two bytes. | N               | X86 CPU MAC Addresses. The first six bytes represent the Base MAC Address, and the last two bytes represent the number of MAC Addresses from the Base.       |
| 18   | 8              | 48 bits of Base MAC Address in the first six bytes. Unsigned Integer in the last two bytes. | N               | BMC MAC Addresses. The first six bytes represent the Base MAC Address, and the last two bytes represent the number of MAC Addresses from the Base.           |
| 19   | 8              | 48 bits of Base MAC Address in the first six bytes. Unsigned Integer in the last two bytes. | N               | Switch ASIC MAC Addresses. The first six bytes represent the Base MAC Address, and the last two bytes represent the number of MAC Addresses from the Base.   |
| 20   | 8              | 48 bits of Base MAC Address in the first six bytes. Unsigned Integer in the last two bytes. | N               | Meta Reserved MAC Addresses. The first six bytes represent the Base MAC Address, and the last two bytes represent the number of MAC Addresses from the Base. |
| 250  | 2              | Unsigned Integer                                                                            | Y               | CRC16. Refer to the “Checksum” section for details                                                                                                           |

## **Checksum:**

We use the 16 bits CRC-CCITT-AUG specification with the following key
attributes:

- Width \= 16 bits
- Truncated polynomial \= 0x1021
- Initial value \= 0x1D0F
- Input data is NOT reflected
- Output CRC is NOT reflected
- No XOR is performed on the output CRC

All EEPROM content bytes (starting from Meta EEPROM header up to the CRC16 TLV,
but NOT including the CRC16 TLV field) are included for this CRC16 calculation.
Be aware there is a 16 bits argument for CRC16 calculation. For example, if
CRC16 Type field is located at offset 63, CRC16 calculation will include bytes
from offset 0 to offset 62, inclusively.

Check
https://github.com/facebook/fboss/blob/main/fboss/platform/weutil/Crc16CcittAug.cpp
for CRC16 CCITT AUG implementation.

The below list shows the CRC16 output values for corner cases of byte streams
and can be used as the reference to check the correctness of the algorithm.

| Byte Stream                                                   | Length | CRC16 (per CRC-CCITT) |
| :------------------------------------------------------------ | :----- | :-------------------- |
| NULL                                                          | 0      | 0x1D0F                |
| A                                                             | 1      | 0x9479                |
| 123456789                                                     | 9      | 0xE5CC                |
| A string of 256 upper case “A” characters with no line breaks | 256    | 0XE938                |

## **Example:**

**Raw EEPROM BLOB:**

```
[root@devserver ~]$ hexdump -C Juice=Juice
00000000  fb fb 05 ff 01 0d 46 49  52 53 54 5f 53 51 55 45  |......FIRST_SQUE|
00000010  45 5a 45 02 08 32 30 31  32 33 34 35 36 03 08 53  |EZE..20123456..S|
00000020  59 53 41 31 32 33 34 04  0c 50 43 42 41 31 32 33  |YSA1234..PCBA123|
00000030  34 35 36 37 20 05 0c 50  43 42 31 32 33 34 35 36  |4567 ..PCB123456|
00000040  37 38 20 06 0c 4d 59 4f  44 4d 31 32 33 34 35 36  |78 ..MYODM123456|
00000050  37 07 0d 4f 53 31 32 33  34 35 36 37 38 39 41 42  |7..OS123456789AB|
00000060  08 01 01 09 01 00 0a 01  01 0b 0d 50 53 31 32 33  |...........PS123|
00000070  34 35 36 37 38 39 30 41  0c 07 55 4e 41 5f 4d 41  |4567890A..UNA_MA|
00000080  53 0d 08 32 30 31 33 30  32 30 33 0e 05 54 45 52  |S..20130203..TER|
00000090  5a 4f 0f 09 4a 55 49 43  45 54 4f 52 59 10 07 42  |ZO..JUICETORY..B|
000000a0  55 44 4f 4b 41 4e 11 08  11 22 33 44 55 66 01 02  |UDOKAN..."3DUf..|
000000b0  12 08 12 34 56 78 9a bc  03 04 13 08 66 55 44 33  |...4Vx......fUD3|
000000c0  22 11 02 00 14 08 fe dc  ba 98 76 54 00 02 fa 02  |".........vT....|
000000d0  d5 c6 ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |'_..............|
000000e0  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00002000
```

**Output after parsing :**

```
[root@devserver ~]# ./weutil --path /v5_eeprom 2>/dev/null
Wedge EEPROM : /v5_eeprom
Product Name: FIRST_SQUEEZE
Product Part Number: 20123456
System Assembly Part Number: SYSA1234
Meta PCBA Part Number: PCBA1234567
Meta PCB Part Number: PCB12345678
ODM/JDM PCBA Part Number: MYODM1234567
ODM/JDM PCBA Serial Number: OS123456789AB
Product Production State: 1
Product Version: 0
Product Sub-Version: 1
Product Serial Number: PS1234567890A
System Manufacturer: UNA_MAS
System Manufacturing Date: 20130203
PCB Manufacturer: TERZO
Assembled at: JUICETORY
EEPROM location on Fabric: BUDOKAN
X86 CPU MAC Base: 11:22:33:44:55:66
X86 CPU MAC Address Size: 258
BMC MAC Base: 12:34:56:78:9a:bc
BMC MAC Address Size: 772
Switch ASIC MAC Base: 66:55:44:33:22:11
Switch ASIC MAC Address Size: 512
META Reserved MAC Base: fe:dc:ba:98:76:54
META Reserved MAC Address Size: 2
CRC16: 0xd5c6 (CRC Matched)
```

**Note:** In case the CRC programmed in the EEPROM mismatches the CRC calculated
by weutil, the last line printing will display the programmed CRC and expected
CRC.

```
CRC16: 0xa6b7 (CRC Mismatch. Expected 0xd5c6)
```
