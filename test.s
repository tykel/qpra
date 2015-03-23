.bank rom_fixed

init:   mv a, 1
test:   jz loop
        mv a, 0
        jp test
loop:   jp loop
