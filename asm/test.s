; Draw a single sprite on the screen.

.bank rom_fixed

init:       mv a, v_handler0    ; load the initial video IRQ handler address
            mv [$fffa], a       ; store it in the interrupt vector

loop:       jp loop             ; loop until the video IRQ

v_handler0: mv a, $85           ; set Enable bit and H-Double bit
            mv.b [$ea00], a     ; update sprite 0 reg.
            mv.b [$ea04], a     ; update sprite 1 reg.
            mv.b [$ea08], a     ; update sprite 2 reg.
            mv a, $88           ; set x and y sprite group offsets to 0
            mv.b [$ea02], a     ; update sprite 0 reg.
            mv a, $a8           ; set x offset to 3, y to 0
            mv.b [$ea06], a     ; update sprite 1 reg.
            mv a, $c8           ; set x offset to -3, y to 0
            mv.b [$ea0a], a     ; update sprite 2 reg.
            mv a, $01           ; tile index 1
            mv.b [$ea03], a     ; update sprite 0 reg.
            mv.b [$ea07], a     ; update sprite 1 reg.
            mv.b [$ea0b], a     ; update sprite 2 reg.
            mv a, 0             ; use palette 0
            mv [$eb81], a       ; update sprite palette reg.
            mv a, $04           ; dark gray 
            mv.b [$e900], a     ; update palette 0, entry 0
            mv a, $dd           ; color 'white' in global palette
            mv.b [$e901], a     ; update palette 0, entry 1
            mv c, $80           ; x = 128
            mv d, $70           ; x = 112
            mv d, $60           ; x = 96 
            mv.b [$eb00], c
            mv.b [$eb01], d
            mv.b [$eb02], e
            mv a, v_handler1    ; load the "real" video IRQ handler address
            mv [$fffa], a       ; store it in the interrupt vector
            rti

v_handler1: mv.b [$eb00], c     ; update group 0 pos.
            mv.b [$eb01], d     ; update group 1 pos.
            mv.b [$eb02], e     ; update group 1 pos.
            inc c               ; increment group x position
            and c, $ff
            ;inc d               ; increment group y position
            ;and d, $ff
            ;mv.b a, [$ea06]
            ;mv b, a
            ;lsr b, 4
            ;inc b
            ;lsl b, 4
            ;and a, $0f
            ;or a, b
            ;mv.b [$ea06], a
            rti

.bank tile_swap 0

.db $10,$00,$00,$01,
.db $01,$00,$00,$10,
.db $00,$10,$01,$00,
.db $00,$01,$10,$00,
.db $00,$01,$10,$00,
.db $00,$10,$01,$00,
.db $01,$00,$00,$10,
.db $10,$00,$00,$01,

.db $11,$11,$11,$11,
.db $11,$11,$11,$11,
.db $11,$11,$11,$11,
.db $11,$11,$11,$11,
.db $11,$11,$11,$11,
.db $11,$11,$11,$11,
.db $11,$11,$11,$11,
.db $11,$11,$11,$11,

