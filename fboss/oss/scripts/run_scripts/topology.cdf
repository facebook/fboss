comp {
    NAME sw0
    REMOTE yes
    LAUNCH_SDK no
    WD $BCMSIM_ROOT/run/$USER/sw0/
    chip {
        NAME switch0
        TYPE BCM78900
        SCID 10
        XTERM no
        PORT 1..258 LINK=eth PHY=ge
    }
}
