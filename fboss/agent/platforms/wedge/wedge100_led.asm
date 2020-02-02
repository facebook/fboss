; Wedge100 LED microprocessor assembly.
;
; See the Tomahawk Theory of Operations document for an explanation of
; the LED microprocessor behavior and assembly.

; Address locations in the data ram.
;
; The software will write one byte of status information for each of the 64
; ports managed by this microprocessor, at bytes 0xc0 - 0xff.
;
; For each of these bytes, we define a 3-bit RGB LED format:
;  0 (LSB) :  Red led on
;  1       :  Green led on
;  2       :  Blue
;
; This value defines the start byte where FBOSS will write port color info.
ADDR_SW_PORTS equ 0xc0

; Note that unlike Wedge40, we iterate over the ports backwards.  This
; is because the CPLD accesses leds in reverse order of the logical
; numbering. The CPLD takes care of flipping the ports back within a
; 24-bit block, but cannot change the order outside of 24 bits so we
; pass the cpld the ports in reverse order.
;
; This value defines the port our code loop will read first. Since we go
; backwards, this is the last port.
START_PORT equ 63


; The main entry point.
; The microprocessor starts here each cycle.
;
update:
    ld a, START_PORT

loop_start:
    ; We return to loop start once for each port.
    ; Register A should contain the port number.

    ; Load the software color bits into B
    ld b, ADDR_SW_PORTS
    add b, a
    ld b, (b)

    ; Call set_led, which packs the three color bits in register b in
    ; to the bitstream
    call set_led

    ; Check to see if we need to continue around the loop
    dec a
    jnc loop_start

    ; All done.  Send the 192 (64 * 3) bits of data we constructed.
    send 192

set_led:
    ; 3 bits per LED
    ;
    ; Note that the LSB is sent on the wire first.
    push b
    pack
    ror b
    push b
    pack
    ror b
    push b
    pack
    ret
