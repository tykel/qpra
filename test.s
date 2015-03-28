; Draw a single sprite on the screen.

.bank rom_fixed

init:       mv a, $80           ; sprite Enable bit
            mv.b [$ea00], a     ; update sprite 0 reg.
            mv a, $01           ; tile index 1
            mv.b [$ea03], a     ; update sprite 0 reg.
            mv a, $0101         ; x = 1, y = 1
            mv [$eb00], a       ; update group 0 pos.
            mv a, 0             ; use palette 0
            mv [$eb81], a       ; update sprite palette reg.
            mv a, $dc           ; color 'white' in global palette
            mv.b [$e901], a     ; update palette 0, entry 1

            mv a, $00
            
loop:       mv [$eb00], a
            inc a
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            jp loop

.bank tile_swap 0

.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,

.db $11,$11,$11,$11,
.db $11,$00,$11,$11,
.db $11,$00,$11,$11,
.db $11,$00,$11,$11,
.db $11,$11,$00,$11,
.db $11,$11,$00,$11,
.db $11,$11,$00,$11,
.db $11,$11,$11,$11,

