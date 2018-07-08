; Graphical demo to showcase Khepra's abilities.

.bank rom_fixed

init:       mv a, v_handler0    ; load the initial video IRQ handler address
            mv [$fffa], a       ; store it in the interrupt vector
            mv c, 0
            mv d, 0

loop:       jp loop             ; loop until the video IRQ

v_handler0: mv a, $85           ; set Enable bit and H-Double bit
            mv.b [$ea00], a     ; update sprite 0 reg.
            mv.b [$ea04], a     ; update sprite 1 reg.
            mv.b [$ea08], a     ; update sprite 2 reg.
            mv.b [$ea0c], a     ; update sprite 3 reg.
            mv a, $00           ; set sprite group to 0
            mv.b [$ea01], a     ; update sprite 0 reg.
            mv.b [$ea05], a     ; update sprite 1 reg.
            mv.b [$ea09], a     ; update sprite 2 reg.
            mv.b [$ea0d], a     ; update sprite 3 reg.
            mv a, $88           ; set x and y sprite group offsets to 0
            mv.b [$ea02], a     ; update sprite 0 reg.
            mv a, $a8           ; set x and y sprite group offsets to 0
            mv.b [$ea06], a     ; update sprite 1 reg.
            mv a, $8a           ; set x and y sprite group offsets to 0
            mv.b [$ea0a], a     ; update sprite 2 reg.
            mv a, $aa           ; set x and y sprite group offsets to 0
            mv.b [$ea0e], a     ; update sprite 3 reg.
            mv a, $01           ; tile index 1
            mv.b [$ea03], a     ; update sprite 0 reg.
            mv a, $02           ; tile index 2
            mv.b [$ea07], a     ; update sprite 1 reg.
            mv a, $03           ; tile index 3
            mv.b [$ea0b], a     ; update sprite 2 reg.
            mv a, $04           ; tile index 4
            mv.b [$ea0f], a     ; update sprite 3 reg.
            
            mv a, 0             ; use palette 0
            mv [$eb40], a       ; update layer 1/2 palette reg.
            mv [$eb81], a       ; update sprite palette reg.
            mv a, $7020         ; y = 96
            mv [$eb00], a       ; update group 0 pos.
            mv a, $b4           ; grey
            mv.b [$e900], a     ; update palette 0, entry 0
            mv a, $00           ; black
            mv.b [$e901], a     ; update palette 0, entry 1
            mv a, $46           ; light green
            mv.b [$e902], a     ; update palette 0, entry 2
            mv a, $43           ; dark green
            mv.b [$e903], a     ; update palette 0, entry 3
            mv a, $27           ; red
            mv.b [$e904], a     ; update palette 0, entry 4
            mv a, $dd           ; white
            mv.b [$e90f], a     ; update palette 0, entry f
            mv a, v_handler1    ; load the "real" video IRQ handler address
            mv [$fffa], a       ; store it in the interrupt vector
            rti

v_handler2: rti

v_handler1: inc c
            and c, 255
            mv d, c
            add d, sin_lut
            mv.b d, [d]
            add d, 24 
            mv e, c
            add e, 64
            and e, 255
            add e, sin_lut
            mv e, [e]
            add e, 24
            lsl e, 8
            or d, e
            lsl d, 1
            mv [$eb00], d       ; update group 0 pos.
            rti

;------------------------------------------------------------------------------
sin_lut:    
.db 32,33,34,34,
.db 35,36,37,37,
.db 38,39,40,41,
.db 41,42,43,44,
.db 44,45,46,46,
.db 47,48,48,49,
.db 50,50,51,52,
.db 52,53,53,54,
.db 55,55,56,56,
.db 57,57,58,58,
.db 59,59,59,60,
.db 60,61,61,61,
.db 62,62,62,62,
.db 63,63,63,63,
.db 63,64,64,64,
.db 64,64,64,64,
.db 64,64,64,64,
.db 64,64,64,64,
.db 63,63,63,63,
.db 63,62,62,62,
.db 62,61,61,61,
.db 60,60,59,59,
.db 59,58,58,57,
.db 57,56,56,55,
.db 55,54,53,53,
.db 52,52,51,50,
.db 50,49,48,48,
.db 47,46,46,45,
.db 44,44,43,42,
.db 41,41,40,39,
.db 38,37,37,36,
.db 35,34,34,33,
.db 32,31,30,30,
.db 29,28,27,27,
.db 26,25,24,23,
.db 23,22,21,20,
.db 20,19,18,18,
.db 17,16,16,15,
.db 14,14,13,12,
.db 12,11,11,10,
.db 9,9,8,8,
.db 7,7,6,6,
.db 5,5,5,4,
.db 4,3,3,3,
.db 2,2,2,2,
.db 1,1,1,1,
.db 1,0,0,0,
.db 0,0,0,0,
.db 0,0,0,0,
.db 0,0,0,0,
.db 1,1,1,1,
.db 1,2,2,2,
.db 2,3,3,3,
.db 4,4,5,5,
.db 5,6,6,7,
.db 7,8,8,9,
.db 9,10,11,11,
.db 12,12,13,14,
.db 14,15,16,16,
.db 17,18,18,19,
.db 20,20,21,22,
.db 23,23,24,25,
.db 26,27,27,28,
.db 29,30,30,31,

;------------------------------------------------------------------------------
.bank tile_swap 0

; 0 Background tile
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $0f,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$00,$00,$0f,
.db $00,$00,$00,$00,
.db $00,$00,$00,$00,
.db $00,$f0,$00,$00,

; Player tiles
; 1 Top left
.db $00,$00,$00,$01,
.db $00,$00,$00,$11,
.db $00,$00,$00,$11,
.db $00,$00,$00,$1f,
.db $00,$00,$11,$13,
.db $00,$01,$22,$22,
.db $00,$01,$23,$22,
.db $00,$12,$23,$32,
; 2 Top right
.db $10,$00,$00,$00,
.db $11,$00,$00,$00,
.db $f1,$00,$00,$00,
.db $f1,$00,$00,$00,
.db $33,$11,$00,$00,
.db $22,$22,$10,$00,
.db $22,$23,$10,$00,
.db $22,$13,$31,$00,
; 3 Bottom left
.db $00,$12,$11,$32,
.db $00,$14,$11,$11,
.db $00,$1f,$11,$44,
.db $00,$11,$11,$44,
.db $00,$00,$01,$f1,
.db $00,$00,$01,$41,
.db $00,$00,$01,$f1,
.db $00,$00,$01,$11,
; 4 Bottom right
.db $22,$11,$31,$00,
.db $11,$11,$41,$00,
.db $44,$41,$f1,$00,
.db $11,$41,$11,$00,
.db $14,$f1,$00,$00,
.db $14,$10,$00,$00,
.db $1f,$10,$00,$00,
.db $11,$11,$00,$00,

; Test tiles
.db $01,$11,$11,$10,
.db $11,$11,$11,$11,
.db $11,$01,$10,$11,
.db $11,$01,$10,$11,
.db $11,$11,$11,$11,
.db $10,$11,$11,$01,
.db $11,$00,$00,$11,
.db $01,$11,$11,$10,

.db $10,$00,$00,$00,
.db $11,$00,$00,$00,
.db $11,$10,$00,$00,
.db $11,$11,$00,$00,
.db $11,$11,$10,$00,
.db $11,$11,$11,$00,
.db $11,$11,$11,$10,
.db $11,$11,$11,$11,

.db $01,$01,$01,$01,
.db $10,$10,$10,$10,
.db $01,$01,$01,$01,
.db $10,$10,$10,$10,
.db $01,$01,$01,$01,
.db $10,$10,$10,$10,
.db $01,$01,$01,$01,
.db $10,$10,$10,$10,

