; Draw a single sprite on the screen.

.bank rom_fixed

init:       mv a, v_handler0    ; load the initial video IRQ handler address
            mv [$fffa], a       ; store it in the interrupt vector

loop:       jp loop             ; loop until the video IRQ

v_handler0: mv a, $85           ; set Enable bit and H-Double bit
            mv.b [$ea00], a     ; update sprite 0 reg.
            mv a, $01           ; tile index 1
            mv.b [$ea03], a     ; update sprite 0 reg.
            mv a, 0             ; use palette 0
            mv [$eb81], a       ; update sprite palette reg.
            mv a, $03           ; random color
            mv.b [$e900], a     ; update palette 0, entry 0
            mv a, $89           ; color 'white' in global palette
            mv.b [$e901], a     ; update palette 0, entry 1
            mv c, $0101         ; x = 1, y = 1
            mv a, v_handler1    ; load the "real" video IRQ handler address
            mv [$fffa], a       ; store it in the interrupt vector
            rti

v_handler1: mv [$eb00], c       ; update group 0 pos.
            inc c               ; increment group x position
            lsl c, 8
            inc c
            rti

.bank tile_swap 0

.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $01,$00,$01,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$01,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$10,$00,$00,

.db $01,$11,$11,$10,
.db $11,$11,$11,$11,
.db $11,$01,$10,$11,
.db $11,$01,$10,$11,
.db $11,$11,$11,$11,
.db $10,$11,$11,$01,
.db $11,$00,$00,$11,
.db $01,$11,$11,$10,

