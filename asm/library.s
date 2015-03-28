; 
; library.s -- Library functions useful for khepra development
;
; A collection of reusable functions to simplify writing programs and games
; in khepra assembly.
;

; NOTE: The calling convention is *caller*-saves, with parameters passed on the
; stack.
; Registers a, b, c *can* be overwritten by subroutines.
; Registers d, e, *cannot* be overwritten.


;------------------------------------------------------------------------------
; Setup the initial interrupt vector.
; Usage: lib_setup_iv(audio, video, timer, user) 

lib_setup_iv:       mv c, s         ; Save the stack pointer
                    ind s           ; s += 2 to point to user 
                    mv a, [s]       ; Read user handler address
                    mv [$fffe], a   ; Write it in user handler entry
                    ind s           ; s += 2 to point to timer
                    mv a, [s]       ; Read timer handler address
                    mv [$fffc], a   ; Write it in timer handler entry
                    ind s           ; s += 2 to point to video
                    mv a, [s]       ; Read video handler address
                    mv [$fffa], a   ; Write it in video handler entry
                    ind s           ; s += 2 to point to audio
                    mv a, [s]       ; Read audio handler address
                    mv [$fff8], a   ; Write it in audio handler entry
                    mv s, c         ; Restore the stack pointer
                    rts

;------------------------------------------------------------------------------
; Enable sprite number index.
; Usage: lib_enable_sprite(index)

lib_enable_sprite:  mv c, s         ; Save the stack pointer
                    ind s           ; s += 2 to point to the first parameter
                    mv a, [s]       ; Read the index parameter
                    lsl a, 2        ; Multiply by 4 (size of sprite reg.)
                    add a, $ea00    ; Add sprite reg. vector base address
                    mv.b b, [a]     ; Read current sprite reg.
                    or b, $80       ; Set Enable bit
                    mv [a], b       ; Write back to sprite reg.
                    mv s, c         ; Restore the stack pointer
                    rts

;------------------------------------------------------------------------------
; Disable sprite number index.
; Usage: lib_disable_sprite(index)

lib_disable_sprite: mv c, s         ; Save the stack pointer
                    ind s           ; s += 2 to point to the first parameter
                    mv a, [s]       ; Read the index parameter
                    lsl a, 2        ; Multiply by 4 (size of sprite reg.)
                    add a, $ea00    ; Add sprite reg. vector base address
                    mv.b b, [a]     ; Read current sprite reg.
                    and b, $7f      ; Clear Enable bit
                    mv [a], b       ; Write back to sprite reg.
                    mv s, c         ; Restore the stack pointer
                    rts

;------------------------------------------------------------------------------
; Set the sprite palette index to pi.
; Usage: lib_set_sprite_pi(pi)

lib_set_sprite_pi:  mv c, s         ; Save the stack pointer
                    ind s           ; s += 2 to point to pi
                    mv a, [s]       ; Read pi
                    mv [$eb81], a   ; Write it in the sprite palette index reg.
                    mv s, c         ; Restore the stack pointer

;------------------------------------------------------------------------------
; Set the tilemap layer 1 palette index to pi.
; Usage: lib_set_layer1_pi(pi)

lib_set_layer1_pi:  mv c, s         ; Save the stack pointer
                    ind s           ; s += 2 to point to pi
                    mv a, [s]       ; Read pi
                    mv b, [$eb80]   ; Read current layer 1-2 palette index
                    and b, $0f      ; Clear current layer 1 palette index
                    lsl a, 4        ; Set right bits for pi
                    or a, b         ; Set pi on layer 1-2 palette index
                    mv [$eb80], a   ; Write it in the sprite palette index reg.
                    mv s, c         ; Restore the stack pointer
                    rts

;------------------------------------------------------------------------------
; Set the tilemap layer 1 palette index to pi.
; Usage: lib_set_layer2_pi(pi)

lib_set_layer1_pi:  mv c, s         ; Save the stack pointer
                    ind s           ; s += 2 to point to pi
                    mv a, [s]       ; Read pi
                    mv b, [$eb80]   ; Read current layer 1-2 palette index
                    and b, $f0      ; Clear current layer 2 palette index
                    and a, $0f      ; Set right bits for pi
                    or a, b         ; Set pu on layer 1-2 palette index
                    mv [$eb80], a   ; Write it in the sprite palette index reg.
                    mv s, c         ; Restore the stack pointer
                    rts

;------------------------------------------------------------------------------
; Functions to push various registers to the stack.
; No arguments.

push_a:             mv [s], a
                    ded s
                    rts

push_b:             mv [s], b
                    ded s
                    rts

push_c:             mv [s], c
                    ded s
                    rts

push_d:             mv [s], d
                    ded s
                    rts

push_e:             mv [s], e
                    ded s
                    rts

push_f:             mv [s], f
                    ded s
                    rts
                    
