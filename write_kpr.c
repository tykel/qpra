#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    FILE *file;
    uint8_t data[4];
    char name[16] = "Khepra test ROM";
    char desc[32] = "Simple dummy ROM with a NOP";

    file = fopen("test.kpr", "wb");
    // Write magic
    *(char *)&data[0] = 'K';
    *(char *)&data[1] = 'H';
    *(char *)&data[2] = 'P';
    *(char *)&data[3] = 'R';
    fwrite(data, 4, 1, file);
    // Write file size
    *(uint32_t *)data = 68 + 1;
    fwrite(data, 4, 1, file);
    // Write (dummy) checksum
    *(uint32_t *)data = 0;
    fwrite(data, 4, 1, file);
    // Write ROM bank numbers
    *(uint32_t *)data = 0x01010101;
    fwrite(data, 4, 1, file);
    // Write reserved bank
    *(uint32_t *)data = 0;
    fwrite(data, 4, 1, file);
    // Write name
    fwrite(name, 16, 1, file);
    // Write description
    fwrite(desc, 32, 1, file);

    // Write NOP
    *(uint8_t *)data = 0;
    fwrite(data, 1, 1, file);
    
    fclose(file);

    return 0;
}
