# The MIT License (MIT)
# =====================
#
# Copyright (c) 2014 Ryan Sturmer
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# License URL: https://github.com/ryansturmer/hexfile/blob/master/LICENSE.md
# Code URL: https://github.com/ryansturmer/hexfile/blob/master/hexfile/core.py

import itertools

def short(msb,lsb):
    return (msb<<8) | lsb

def long(b1, b2, b3, b4):
    return (b1<<24) | (b2<<16) | (b3<<8) | b4

class HexFile(object):
    def __init__(self, segments, eip, cs, ip):
        self.segments = segments
        self.eip = eip
        self.cs = cs
        self.ip = ip

    def __getitem__(self, val):
        if isinstance(val, slice):
            address = val.start
        else:
            address = val

        for segment in self.segments:
            if address in segment:
                return segment[val]

        raise IndexError('No segment contains address 0x%x' % address)

    def __len__(self):
        return sum(map(len, self.segments))

    @property
    def size(self):
        return len(self)

    def __iter__(self):
        return itertools.chain(*self.segments)

    @staticmethod
    def load(filename):
        segments = []
        eip = None
        cs = None
        ip = None

        with open(filename) as fp:
            lines = fp.readlines()

        extended_linear_address = 0
        extended_segment_address = 0
        current_address = 0
        end_of_file = False

        lineno = 0
        for line in lines:
            lineno += 1
            line = line.strip();
            if not line.startswith(':'):
                continue

            if end_of_file:
                raise Exception("Record found after end of file on line %d" % lineno)

            bytes = [int(line[i:i+2], 16) for i in range(1,len(line), 2)]
            byte_count = bytes[0]
            address = short(*bytes[1:3])
            record_type = bytes[3]
            checksum = bytes[-1]
            data = bytes[4:-1]
            computed_checksum = ((1 << 8)-(sum(bytes[:-1]) & 0xff)) & 0xff

            if(computed_checksum != checksum):
                raise Exception("Record checksum doesn't match on line %d" % lineno)

            if record_type == 0:
                if byte_count == len(data):
                    current_address = (address + extended_linear_address + extended_segment_address) & 0xffffffff
                    have_segment = False
                    for segment in segments:
                        if segment.end_address == current_address:
                            segment.data.extend(data)
                            have_segment = True
                            break
                    if not have_segment:
                        segments.append(Segment(current_address, data))
                else:
                    raise Exception("Data record reported size does not match actual size on line %d" % lineno)
            elif record_type == 1:
                end_of_file = True
            elif record_type == 2:
                if byte_count != 2 or len(data) != 2:
                    raise Exception("Byte count misreported in extended segment address record on line %d" % lineno)
                extended_segment_address = short(*data) << 4
            elif record_type == 3:
                if byte_count != 4 or len(data) != 4:
                    raise Exception("Byte count misreported in start segment address record on line %d" % lineno)
                cs = short(*data[0:2])
                ip = short(*data[2:4])
            elif record_type == 4:
                if byte_count != 2 or len(data) != 2:
                    raise Exception("Byte count misreported in extended linear address record on line %d" % lineno)
                extended_linear_address = short(*data) << 16
            elif record_type == 5:
                if byte_count != 4 or len(data) != 4:
                    raise Exception("Byte count misreported in start linear address record on line %d" % lineno)
                eip = long(*data)
            else:
                raise Exception("Unknown record type: %s" % record_type)
        return HexFile(segments, eip, cs, ip)

    def pretty_string(self, stride=16):
        retval = []
        for segment in self.segments:
            retval.append('Segment @ 0x%08x (%d bytes)' % (segment.start_address, segment.size))
            retval.append(segment.pretty_string(stride=stride))
            retval.append('')
        if self.eip is not None:
            retval.append('EIP 0x%x' % self.eip)
        if self.cs is not None:
            retval.append('CS 0x%x' % self.cs)
        if self.ip is not None:
            retval.append('IP 0x%x' % self.ip)
        return '\n'.join(retval)

def load(filename):
    return HexFile.load(filename)

class Segment(object):
    def __init__(self, start_address, data = None):
        self.start_address = start_address
        self.data = data or []

    def pretty_string(self, stride=16):
        retval = []
        addresses = self.addresses
        ranges = [addresses[i:i+stride] for i in range(0, self.size, stride)]
        for r in ranges:
            retval.append('%08x ' % r[0] + ' '.join(['%02x' % self[addr] for addr in r]))
        return '\n'.join(retval)

    def __str__(self):
        return '<%d byte segment @ 0x%08x>' % (self.size, self.start_address)
    def __repr__(self):
        return str(self)

    @property
    def end_address(self):
        return self.start_address + len(self.data)

    @property
    def size(self):
        return len(self.data)

    def __contains__(self, address):
        return address >= self.start_address and address < self.end_address

    def __getitem__(self, address):
        if isinstance(address, slice):
            if address.start not in self or address.stop-1 not in self:
                raise IndexError('Address out of range for this segment')
            else:
                d = self.data[address.start-self.start_address:address.stop-self.start_address:address.step]
                start_address = address.start + self.start_address
                return Segment(start_address, d)
        else:
            if not address in self:
                raise IndexError("Address 0x%x is not in this segment" % address)
            return self.data[address-self.start_address]

    @property
    def addresses(self):
        return range(self.start_address, self.end_address)

    def __len__(self):
        return len(self.data)

    def __iter__(self):
        return iter(zip(self.addresses,self.data))
