; Wedge LED microprocessor assembly.
;
; See the Trident2 Theory of Operations document for an explanation of the LED
; microprocessor behavior and assembly.

; Address locations in the data ram.
;
; The software will write one byte of status information for each of the 32
; ports managed by this microprocessor, at bytes 0xe0 - 0xff.
;
; For each of these bytes, the following bits are defined
;  0 (LSB) :  Link status (up/down)
;  1       :  Blink fast
;  2       :  Blink slow
ADDR_SW_PORTS equ 0xe0

; Since we only have two registers, we store the current port number in memory.
; (At the moment we actually don't actually need this, but it may be useful in
; the future if we want to re-use register A for something else.)
ADDR_PORT_NUMBER equ 0xd0

; We keep a cycle counter, just for helping control blink rates
; One LED update cycle is approximately 30ms.
ADDR_TICKER0 equ 0xd1
ADDR_TICKER1 equ 0xd2


; The main entry point.
; The microprocessor starts here each cycle.
update:
    ld a, 0
    ld (ADDR_PORT_NUMBER), a

    jmp inc_ticker
ticker_done:

loop_start:
    ; We return to loop start once for each port.
    ; Register A already contains the port number.
    inc (ADDR_PORT_NUMBER)

    ; Load the software status byte into B
    ld b, ADDR_SW_PORTS
    add b, a
    ld b, (b)

    ; The first bit controls whether the LED should be on at all or not.
    ; This gets sent out directly as the first of the 2 bits for this port
    ; in the LED stream.
    tst b, 0
    push cy
    pack

    ; Check if we want to be blinking fast or slow, or not at all.
    tst b, 1
    jc blink_fast
    tst b, 2
    jc blink_slow
    push 0
blink_done:
    pack

    ; Check to see if we need to continue around the loop
    ld a, (ADDR_PORT_NUMBER)
    cmp a, 32
    jnz loop_start

    ; All done.  Send the 64 bits of data we constructed.
    send 64

blink_fast:
    ; Blink using bit 4 of (ADDR_TICKER0)
    ; This is approximately once every 0.5 seconds
    ld b, (ADDR_TICKER0)
    tst b, 4
    push cy
    jmp blink_done

blink_slow:
    ; Blink using bit 6 of (ADDR_TICKER0)
    ; This is approximately once every 2 seconds
    ; Slow blinks are 1/4th the speed of fast blinks,
    ; to make it easier to distinguish.
    ld b, (ADDR_TICKER0)
    tst b, 6
    push cy
    jmp blink_done

inc_ticker:
    inc (ADDR_TICKER0)
    jc inc_ticker2
    jmp ticker_done
inc_ticker2:
    inc (ADDR_TICKER1)
    jmp ticker_done
