; Draw a single sprite on the screen.

.bank rom_fixed

init:       mv a, $84           ; sprite Enable bit and H-doubling
            mv.b [$ea00], a     ; update sprite 0 reg.
            mv a, $01           ; tile index 1
            mv.b [$ea03], a     ; update sprite 0 reg.
            mv a, 0             ; use palette 0
            mv [$eb81], a       ; update sprite palette reg.
            mv a, $03           ; random color
            mv.b [$e900], a     ; update palette 0, entry 0
            mv a, $89           ; color 'white' in global palette
            mv.b [$e901], a     ; update palette 0, entry 1
            mv a, $0101         ; x = 1, y = 1

loop:       mv [$eb00], a       ; update group 0 pos.
            inc a               ; increment group x position
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            jp loop             ; loop back to start

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

